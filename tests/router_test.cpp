#include "minimatch/router.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

using minimatch::RouteRequest;
using minimatch::RouteSide;
using minimatch::VenueQuote;
using minimatch::build_route_plan;

TEST(SmartOrderRouter, BuyUsesLowestFeeAdjustedAskFirst) {
  const std::vector<VenueQuote> quotes{
      {"coinbase", 99.0, 1.0, 100.0, 1.0, 10.0, 1.0, 0.0, true},
      {"kraken", 98.9, 1.0, 99.8, 1.0, 20.0, 1.0, 0.0, true},
      {"binance", 98.8, 1.0, 100.2, 1.0, 0.0, 1.0, 0.0, true},
  };

  const auto plan = build_route_plan(
      RouteRequest{RouteSide::Buy, 1.0},
      quotes
  );

  ASSERT_EQ(plan.legs.size(), 1U);
  EXPECT_EQ(plan.legs[0].venue, "kraken");
  EXPECT_TRUE(plan.complete);
}

TEST(SmartOrderRouter, SellUsesHighestEffectiveBidFirst) {
  const std::vector<VenueQuote> quotes{
      {"coinbase", 100.0, 1.0, 101.0, 1.0, 10.0, 1.0, 0.0, true},
      {"kraken", 100.2, 1.0, 101.2, 1.0, 20.0, 1.0, 0.0, true},
      {"binance", 99.9, 1.0, 100.9, 1.0, 0.0, 1.0, 0.0, true},
  };

  const auto plan = build_route_plan(
      RouteRequest{RouteSide::Sell, 1.0},
      quotes
  );

  ASSERT_EQ(plan.legs.size(), 1U);
  EXPECT_EQ(plan.legs[0].venue, "kraken");
  EXPECT_TRUE(plan.complete);
}

TEST(SmartOrderRouter, SplitsAcrossVenues) {
  const std::vector<VenueQuote> quotes{
      {"coinbase", 99.0, 1.0, 100.0, 0.4, 0.0, 1.0, 0.0, true},
      {"kraken", 99.0, 1.0, 100.1, 0.5, 0.0, 1.0, 0.0, true},
      {"binance", 99.0, 1.0, 100.2, 1.0, 0.0, 1.0, 0.0, true},
  };

  const auto plan = build_route_plan(
      RouteRequest{RouteSide::Buy, 1.2},
      quotes
  );

  ASSERT_EQ(plan.legs.size(), 3U);
  EXPECT_DOUBLE_EQ(plan.routed_quantity, 1.2);
  EXPECT_TRUE(plan.complete);
}

TEST(SmartOrderRouter, ReportsPartialRoute) {
  const std::vector<VenueQuote> quotes{
      {"coinbase", 99.0, 1.0, 100.0, 0.25, 0.0, 1.0, 0.0, true},
  };

  const auto plan = build_route_plan(
      RouteRequest{RouteSide::Buy, 1.0},
      quotes
  );

  EXPECT_FALSE(plan.complete);
  EXPECT_DOUBLE_EQ(plan.routed_quantity, 0.25);
}

TEST(SmartOrderRouter, IgnoresUnhealthyVenues) {
  const std::vector<VenueQuote> quotes{
      {"coinbase", 99.0, 1.0, 90.0, 1.0, 0.0, 1.0, 0.0, false},
      {"kraken", 99.0, 1.0, 100.0, 1.0, 0.0, 1.0, 0.0, true},
  };

  const auto plan = build_route_plan(
      RouteRequest{RouteSide::Buy, 1.0},
      quotes
  );

  ASSERT_EQ(plan.legs.size(), 1U);
  EXPECT_EQ(plan.legs[0].venue, "kraken");
}


TEST(SmartOrderRouter, LatencyPenaltyCanChangeBestVenue) {
  const std::vector<VenueQuote> quotes{
      {
          "slow",
          99.0,
          1.0,
          100.0,
          1.0,
          0.0,
          100.0,
          0.1,
          true
      },
      {
          "fast",
          99.0,
          1.0,
          100.05,
          1.0,
          0.0,
          1.0,
          0.1,
          true
      },
  };

  const auto plan = build_route_plan(
      RouteRequest{RouteSide::Buy, 1.0},
      quotes
  );

  ASSERT_EQ(plan.legs.size(), 1U);
  EXPECT_EQ(plan.legs[0].venue, "fast");
  EXPECT_TRUE(plan.complete);
}


}  // namespace
