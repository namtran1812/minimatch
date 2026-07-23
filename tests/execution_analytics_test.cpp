#include "minimatch/execution_analytics.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace minimatch {
namespace {

TEST(ExecutionAnalytics, ConvertsOmsFillsIntoTcaReport) {
  ParentOrder parent;
  parent.id = 42;
  parent.symbol = "BTC-USD";
  parent.side = RouteSide::Buy;
  parent.quantity = 100.0;

  RoutePlan plan;
  plan.legs.push_back(RouteLeg{
      .venue = "VENUE_A",
      .price = 100.0,
      .effective_price = 100.05,
      .quantity = 60.0,
  });

  plan.legs.push_back(RouteLeg{
      .venue = "VENUE_B",
      .price = 100.1,
      .effective_price = 100.15,
      .quantity = 40.0,
  });

  const std::vector<OmsExecutionReport> fills{
      OmsExecutionReport{
          .execution_report_id = 1,
          .parent_id = parent.id,
          .child_id = 11,
          .venue = "VENUE_A",
          .price = 100.2,
          .quantity = 60.0,
          .notional = 6012.0,
          .fee = 1.0,
          .timestamp_ns = 1000,
      },
      OmsExecutionReport{
          .execution_report_id = 2,
          .parent_id = parent.id,
          .child_id = 12,
          .venue = "VENUE_B",
          .price = 100.4,
          .quantity = 40.0,
          .notional = 4016.0,
          .fee = 1.0,
          .timestamp_ns = 2000,
      },
  };

  const ExecutionQuality result =
      ExecutionAnalytics{}.analyze(parent, plan, fills, 100.0, 100.1, 100.5);

  EXPECT_EQ(result.parent_order_id, 42);
  EXPECT_EQ(result.symbol, "BTC-USD");
  EXPECT_DOUBLE_EQ(result.requested_quantity, 100.0);
  EXPECT_DOUBLE_EQ(result.filled_quantity, 100.0);
  EXPECT_DOUBLE_EQ(result.unfilled_quantity, 0.0);
  EXPECT_DOUBLE_EQ(result.fill_rate, 1.0);

  EXPECT_NEAR(result.average_fill_price, 100.28, 1e-9);

  EXPECT_DOUBLE_EQ(result.total_fees, 2.0);
  EXPECT_EQ(result.venues.size(), 2U);
  EXPECT_GT(result.implementation_shortfall_bps, 0.0);
}

TEST(ExecutionAnalytics, IncludesOpportunityCostForUnfilledQuantity) {
  ParentOrder parent;
  parent.id = 7;
  parent.symbol = "ETH-USD";
  parent.side = RouteSide::Buy;
  parent.quantity = 100.0;

  RoutePlan plan;
  plan.legs.push_back(RouteLeg{
      .venue = "VENUE_A",
      .price = 100.0,
      .effective_price = 100.0,
      .quantity = 50.0,
  });

  const std::vector<OmsExecutionReport> fills{
      OmsExecutionReport{
          .execution_report_id = 1,
          .parent_id = parent.id,
          .child_id = 21,
          .venue = "VENUE_A",
          .price = 100.0,
          .quantity = 50.0,
          .notional = 5000.0,
          .fee = 0.0,
          .timestamp_ns = 1000,
      },
  };

  const ExecutionQuality result =
      ExecutionAnalytics{}.analyze(parent, plan, fills, 100.0, 100.0, 102.0);

  EXPECT_DOUBLE_EQ(result.filled_quantity, 50.0);
  EXPECT_DOUBLE_EQ(result.unfilled_quantity, 50.0);
  EXPECT_DOUBLE_EQ(result.fill_rate, 0.5);

  EXPECT_DOUBLE_EQ(result.opportunity_cost, 100.0);

  EXPECT_DOUBLE_EQ(result.implementation_shortfall, 100.0);
}

TEST(ExecutionAnalytics, PreservesMarkouts) {
  ParentOrder parent;
  parent.id = 9;
  parent.symbol = "SOL-USD";
  parent.side = RouteSide::Sell;
  parent.quantity = 10.0;

  RoutePlan plan;

  const std::vector<ExecutionMarkout> markouts{
      ExecutionMarkout{
          .horizon_ns = 1'000'000'000,
          .reference_price = 99.0,
          .markout_bps = 100.0,
      },
  };

  const ExecutionQuality result = ExecutionAnalytics{}.analyze(
      parent, plan, {}, 100.0, 100.0, 99.0, markouts);

  ASSERT_EQ(result.markouts.size(), 1U);
  EXPECT_EQ(result.markouts.front().horizon_ns, 1'000'000'000U);
  EXPECT_DOUBLE_EQ(result.markouts.front().reference_price, 99.0);
}

} // namespace
} // namespace minimatch
