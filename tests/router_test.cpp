#include "minimatch/router.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

using minimatch::RouteRequest;
using minimatch::RouteSide;
using minimatch::VenueLevel;
using minimatch::VenueQuote;
using minimatch::build_route_plan;

VenueQuote quote(
    std::string venue,
    std::vector<VenueLevel> bids,
    std::vector<VenueLevel> asks,
    double fee_bps = 0.0,
    double latency_ms = 0.0,
    double latency_cost = 0.0,
    bool healthy = true
) {
  return VenueQuote{
      std::move(venue),
      std::move(bids),
      std::move(asks),
      fee_bps,
      latency_ms,
      latency_cost,
      healthy
  };
}

TEST(SmartOrderRouter, BuyUsesLowestEffectiveAskFirst) {
  const std::vector<VenueQuote> quotes{
      quote(
          "coinbase",
          {{99.0, 1.0}},
          {{100.0, 1.0}},
          10.0
      ),
      quote(
          "kraken",
          {{98.9, 1.0}},
          {{99.8, 1.0}},
          20.0
      ),
  };

  const auto plan = build_route_plan(
      RouteRequest{RouteSide::Buy, 1.0},
      quotes
  );

  ASSERT_EQ(plan.legs.size(), 1U);
  EXPECT_EQ(plan.legs[0].venue, "kraken");
  EXPECT_TRUE(plan.complete);
}

TEST(SmartOrderRouter, ConsumesMultipleLevels) {
  const std::vector<VenueQuote> quotes{
      quote(
          "coinbase",
          {{99.0, 1.0}},
          {
              {100.0, 0.25},
              {100.1, 0.50},
              {100.2, 1.00}
          }
      )
  };

  const auto plan = build_route_plan(
      RouteRequest{RouteSide::Buy, 0.60},
      quotes
  );

  ASSERT_EQ(plan.legs.size(), 2U);

  EXPECT_EQ(plan.legs[0].level_index, 0U);
  EXPECT_DOUBLE_EQ(plan.legs[0].quantity, 0.25);

  EXPECT_EQ(plan.legs[1].level_index, 1U);
  EXPECT_DOUBLE_EQ(plan.legs[1].quantity, 0.35);

  EXPECT_TRUE(plan.complete);
  EXPECT_DOUBLE_EQ(plan.routed_quantity, 0.60);
}

TEST(SmartOrderRouter, SplitsAcrossVenuesAndLevels) {
  const std::vector<VenueQuote> quotes{
      quote(
          "coinbase",
          {{99.0, 1.0}},
          {
              {100.0, 0.20},
              {100.3, 0.50}
          }
      ),
      quote(
          "kraken",
          {{99.0, 1.0}},
          {
              {100.1, 0.30},
              {100.2, 0.50}
          }
      ),
  };

  const auto plan = build_route_plan(
      RouteRequest{RouteSide::Buy, 0.75},
      quotes
  );

  ASSERT_EQ(plan.legs.size(), 3U);

  EXPECT_EQ(plan.legs[0].venue, "coinbase");
  EXPECT_DOUBLE_EQ(plan.legs[0].price, 100.0);

  EXPECT_EQ(plan.legs[1].venue, "kraken");
  EXPECT_DOUBLE_EQ(plan.legs[1].price, 100.1);

  EXPECT_EQ(plan.legs[2].venue, "kraken");
  EXPECT_DOUBLE_EQ(plan.legs[2].price, 100.2);

  EXPECT_TRUE(plan.complete);
}

TEST(SmartOrderRouter, SellUsesHighestBidLevelsFirst) {
  const std::vector<VenueQuote> quotes{
      quote(
          "coinbase",
          {
              {100.0, 0.2},
              {99.8, 1.0}
          },
          {{101.0, 1.0}}
      ),
      quote(
          "kraken",
          {
              {100.2, 0.3},
              {99.9, 1.0}
          },
          {{101.2, 1.0}}
      ),
  };

  const auto plan = build_route_plan(
      RouteRequest{RouteSide::Sell, 0.40},
      quotes
  );

  ASSERT_EQ(plan.legs.size(), 2U);
  EXPECT_EQ(plan.legs[0].venue, "kraken");
  EXPECT_DOUBLE_EQ(plan.legs[0].price, 100.2);
  EXPECT_EQ(plan.legs[1].venue, "coinbase");
  EXPECT_DOUBLE_EQ(plan.legs[1].price, 100.0);
}

TEST(SmartOrderRouter, ReportsPartialDepthRoute) {
  const std::vector<VenueQuote> quotes{
      quote(
          "coinbase",
          {{99.0, 1.0}},
          {
              {100.0, 0.20},
              {100.1, 0.30}
          }
      )
  };

  const auto plan = build_route_plan(
      RouteRequest{RouteSide::Buy, 1.0},
      quotes
  );

  EXPECT_FALSE(plan.complete);
  EXPECT_DOUBLE_EQ(plan.routed_quantity, 0.50);
}

TEST(SmartOrderRouter, IgnoresInvalidLevels) {
  const std::vector<VenueQuote> quotes{
      quote(
          "coinbase",
          {{99.0, 1.0}},
          {
              {0.0, 1.0},
              {100.0, 0.0},
              {100.1, 0.5}
          }
      )
  };

  const auto plan = build_route_plan(
      RouteRequest{RouteSide::Buy, 0.5},
      quotes
  );

  ASSERT_EQ(plan.legs.size(), 1U);
  EXPECT_DOUBLE_EQ(plan.legs[0].price, 100.1);
}

TEST(SmartOrderRouter, LatencyPenaltyCanChangeVenue) {
  const std::vector<VenueQuote> quotes{
      quote(
          "slow",
          {{99.0, 1.0}},
          {{100.0, 1.0}},
          0.0,
          100.0,
          0.1
      ),
      quote(
          "fast",
          {{99.0, 1.0}},
          {{100.05, 1.0}},
          0.0,
          1.0,
          0.1
      ),
  };

  const auto plan = build_route_plan(
      RouteRequest{RouteSide::Buy, 1.0},
      quotes
  );

  ASSERT_EQ(plan.legs.size(), 1U);
  EXPECT_EQ(plan.legs[0].venue, "fast");
}

}  // namespace
