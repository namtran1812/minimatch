#include "minimatch/tca.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <vector>

namespace {

using minimatch::ExecutionFill;
using minimatch::RouteLeg;
using minimatch::RoutePlan;
using minimatch::RouteRequest;
using minimatch::RouteSide;
using minimatch::analyze_execution;

RoutePlan plan_with_leg(
    const std::string& venue,
    double price,
    double quantity
) {
  RoutePlan plan;
  plan.complete = true;
  plan.requested_quantity = quantity;
  plan.routed_quantity = quantity;

  plan.legs.push_back(
      RouteLeg{
          venue,
          price,
          quantity,
          0.0,
          price,
          0.0,
          0.0,
          0.0,
          0
      }
  );

  return plan;
}

TEST(Tca, BuySlippageIsPositiveWhenPayingMore) {
  const RouteRequest request{
      RouteSide::Buy,
      10.0
  };

  const auto report =
      analyze_execution(
          request,
          plan_with_leg(
              "venue-a",
              100.0,
              10.0
          ),
          {
              ExecutionFill{
                  1,
                  "venue-a",
                  101.0,
                  10.0,
                  0.0,
                  1
              }
          },
          100.0,
          100.5,
          102.0
      );

  EXPECT_DOUBLE_EQ(
      report.average_fill_price,
      101.0
  );

  EXPECT_NEAR(
      report.arrival_slippage_bps,
      100.0,
      1e-9
  );

  EXPECT_DOUBLE_EQ(
      report.realized_cost,
      10.0
  );
}

TEST(Tca, SellSlippageIsPositiveWhenSellingLower) {
  const RouteRequest request{
      RouteSide::Sell,
      10.0
  };

  const auto report =
      analyze_execution(
          request,
          plan_with_leg(
              "venue-a",
              100.0,
              10.0
          ),
          {
              ExecutionFill{
                  1,
                  "venue-a",
                  99.0,
                  10.0,
                  0.0,
                  1
              }
          },
          100.0,
          99.5,
          98.0
      );

  EXPECT_NEAR(
      report.arrival_slippage_bps,
      100.0,
      1e-9
  );

  EXPECT_DOUBLE_EQ(
      report.realized_cost,
      10.0
  );
}

TEST(Tca, IncludesExplicitFees) {
  const RouteRequest request{
      RouteSide::Buy,
      5.0
  };

  const auto report =
      analyze_execution(
          request,
          plan_with_leg(
              "venue-a",
              100.0,
              5.0
          ),
          {
              ExecutionFill{
                  1,
                  "venue-a",
                  100.0,
                  5.0,
                  2.5,
                  1
              }
          },
          100.0,
          100.0,
          100.0
      );

  EXPECT_DOUBLE_EQ(
      report.fees,
      2.5
  );

  EXPECT_DOUBLE_EQ(
      report.realized_cost,
      2.5
  );

  EXPECT_DOUBLE_EQ(
      report.implementation_shortfall,
      2.5
  );
}

TEST(Tca, CalculatesOpportunityCostForUnfilledBuy) {
  const RouteRequest request{
      RouteSide::Buy,
      10.0
  };

  const auto report =
      analyze_execution(
          request,
          plan_with_leg(
              "venue-a",
              100.0,
              10.0
          ),
          {
              ExecutionFill{
                  1,
                  "venue-a",
                  100.0,
                  4.0,
                  0.0,
                  1
              }
          },
          100.0,
          101.0,
          102.0
      );

  EXPECT_DOUBLE_EQ(
      report.filled_quantity,
      4.0
  );

  EXPECT_DOUBLE_EQ(
      report.unfilled_quantity,
      6.0
  );

  EXPECT_DOUBLE_EQ(
      report.opportunity_cost,
      12.0
  );

  EXPECT_DOUBLE_EQ(
      report.implementation_shortfall,
      12.0
  );
}

TEST(Tca, AggregatesFillsByVenue) {
  const RouteRequest request{
      RouteSide::Buy,
      10.0
  };

  RoutePlan plan;
  plan.complete = true;
  plan.requested_quantity = 10.0;
  plan.routed_quantity = 10.0;

  plan.legs = {
      RouteLeg{
          "coinbase",
          100.0,
          4.0,
          0.0,
          100.0,
          0.0,
          0.0,
          0.0,
          0
      },
      RouteLeg{
          "kraken",
          101.0,
          6.0,
          0.0,
          101.0,
          0.0,
          0.0,
          0.0,
          0
      }
  };

  const auto report =
      analyze_execution(
          request,
          plan,
          {
              ExecutionFill{
                  1,
                  "coinbase",
                  100.0,
                  2.0,
                  0.2,
                  1
              },
              ExecutionFill{
                  1,
                  "coinbase",
                  101.0,
                  2.0,
                  0.2,
                  2
              },
              ExecutionFill{
                  1,
                  "kraken",
                  102.0,
                  6.0,
                  0.6,
                  3
              }
          },
          100.0,
          101.0,
          102.0
      );

  ASSERT_EQ(
      report.venues.size(),
      2U
  );

  EXPECT_EQ(
      report.venues[0].venue,
      "coinbase"
  );

  EXPECT_DOUBLE_EQ(
      report.venues[0].filled_quantity,
      4.0
  );

  EXPECT_DOUBLE_EQ(
      report.venues[0].average_fill_price,
      100.5
  );

  EXPECT_EQ(
      report.venues[1].venue,
      "kraken"
  );

  EXPECT_DOUBLE_EQ(
      report.venues[1].filled_quantity,
      6.0
  );
}

TEST(Tca, CalculatesRoutingRegretAgainstPlan) {
  const RouteRequest request{
      RouteSide::Buy,
      10.0
  };

  const auto report =
      analyze_execution(
          request,
          plan_with_leg(
              "venue-a",
              100.0,
              10.0
          ),
          {
              ExecutionFill{
                  1,
                  "venue-a",
                  101.0,
                  10.0,
                  0.0,
                  1
              }
          },
          100.0,
          100.0,
          101.0
      );

  EXPECT_DOUBLE_EQ(
      report.routing_regret,
      10.0
  );

  EXPECT_NEAR(
      report.routing_regret_bps,
      100.0,
      1e-9
  );
}

TEST(Tca, RejectsOverfills) {
  const RouteRequest request{
      RouteSide::Buy,
      5.0
  };

  EXPECT_THROW(
      analyze_execution(
          request,
          plan_with_leg(
              "venue-a",
              100.0,
              5.0
          ),
          {
              ExecutionFill{
                  1,
                  "venue-a",
                  100.0,
                  6.0,
                  0.0,
                  1
              }
          },
          100.0,
          100.0,
          100.0
      ),
      std::invalid_argument
  );
}

TEST(Tca, EmptyFillsProduceFullOpportunityCost) {
  const RouteRequest request{
      RouteSide::Buy,
      5.0
  };

  const auto report =
      analyze_execution(
          request,
          plan_with_leg(
              "venue-a",
              100.0,
              5.0
          ),
          {},
          100.0,
          101.0,
          102.0
      );

  EXPECT_DOUBLE_EQ(
      report.filled_quantity,
      0.0
  );

  EXPECT_DOUBLE_EQ(
      report.fill_rate,
      0.0
  );

  EXPECT_DOUBLE_EQ(
      report.opportunity_cost,
      10.0
  );
}

}  // namespace
