#include "minimatch/router.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <limits>

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


TEST(RouterConstraints, BuyLimitPriceStopsExpensiveLevels) {
  const std::vector<VenueQuote> quotes{
      quote(
          "coinbase",
          {{99.0, 1.0}},
          {
              {100.0, 0.4},
              {101.0, 1.0}
          }
      )
  };

  const auto plan = build_route_plan(
      RouteRequest{
          RouteSide::Buy,
          1.0,
          100.5
      },
      quotes
  );

  EXPECT_FALSE(plan.complete);
  EXPECT_DOUBLE_EQ(plan.routed_quantity, 0.4);
  ASSERT_EQ(plan.legs.size(), 1U);
  EXPECT_DOUBLE_EQ(plan.legs[0].price, 100.0);
}

TEST(RouterConstraints, SellLimitPriceStopsCheapLevels) {
  const std::vector<VenueQuote> quotes{
      quote(
          "coinbase",
          {
              {100.0, 0.4},
              {99.0, 1.0}
          },
          {{101.0, 1.0}}
      )
  };

  const auto plan = build_route_plan(
      RouteRequest{
          RouteSide::Sell,
          1.0,
          99.5
      },
      quotes
  );

  EXPECT_FALSE(plan.complete);
  EXPECT_DOUBLE_EQ(plan.routed_quantity, 0.4);
  ASSERT_EQ(plan.legs.size(), 1U);
  EXPECT_DOUBLE_EQ(plan.legs[0].price, 100.0);
}

TEST(RouterConstraints, MaximumVenueCountIsEnforced) {
  const std::vector<VenueQuote> quotes{
      quote(
          "coinbase",
          {{99.0, 1.0}},
          {{100.0, 0.4}}
      ),
      quote(
          "kraken",
          {{99.0, 1.0}},
          {{100.1, 0.4}}
      ),
      quote(
          "binance",
          {{99.0, 1.0}},
          {{100.2, 0.4}}
      )
  };

  const auto plan = build_route_plan(
      RouteRequest{
          RouteSide::Buy,
          1.0,
          0.0,
          10000.0,
          2
      },
      quotes
  );

  EXPECT_FALSE(plan.complete);
  EXPECT_DOUBLE_EQ(plan.routed_quantity, 0.8);

  std::vector<std::string> venues;

  for (const auto& leg : plan.legs) {
    if (std::find(
            venues.begin(),
            venues.end(),
            leg.venue
        ) == venues.end()) {
      venues.push_back(leg.venue);
    }
  }

  EXPECT_EQ(venues.size(), 2U);
}

TEST(RouterConstraints, AllOrNoneClearsPartialRoute) {
  const std::vector<VenueQuote> quotes{
      quote(
          "coinbase",
          {{99.0, 1.0}},
          {{100.0, 0.4}}
      )
  };

  const auto plan = build_route_plan(
      RouteRequest{
          RouteSide::Buy,
          1.0,
          0.0,
          10000.0,
          std::numeric_limits<std::size_t>::max(),
          true
      },
      quotes
  );

  EXPECT_FALSE(plan.complete);
  EXPECT_TRUE(plan.legs.empty());
  EXPECT_DOUBLE_EQ(plan.routed_quantity, 0.0);
  EXPECT_DOUBLE_EQ(plan.estimated_notional, 0.0);
  EXPECT_DOUBLE_EQ(plan.estimated_fees, 0.0);
}

TEST(RouterConstraints, BuySlippageLimitStopsWorseLevels) {
  const std::vector<VenueQuote> quotes{
      quote(
          "coinbase",
          {{99.0, 1.0}},
          {
              {100.0, 0.4},
              {100.2, 0.4},
              {101.0, 1.0}
          }
      )
  };

  const auto plan = build_route_plan(
      RouteRequest{
          RouteSide::Buy,
          1.0,
          0.0,
          25.0
      },
      quotes
  );

  EXPECT_FALSE(plan.complete);
  EXPECT_DOUBLE_EQ(plan.routed_quantity, 0.8);
  ASSERT_EQ(plan.legs.size(), 2U);
  EXPECT_DOUBLE_EQ(plan.legs[0].price, 100.0);
  EXPECT_DOUBLE_EQ(plan.legs[1].price, 100.2);
}

TEST(RouterConstraints, SellSlippageLimitStopsWorseLevels) {
  const std::vector<VenueQuote> quotes{
      quote(
          "coinbase",
          {
              {100.0, 0.4},
              {99.8, 0.4},
              {99.0, 1.0}
          },
          {{101.0, 1.0}}
      )
  };

  const auto plan = build_route_plan(
      RouteRequest{
          RouteSide::Sell,
          1.0,
          0.0,
          25.0
      },
      quotes
  );

  EXPECT_FALSE(plan.complete);
  EXPECT_DOUBLE_EQ(plan.routed_quantity, 0.8);
  ASSERT_EQ(plan.legs.size(), 2U);
  EXPECT_DOUBLE_EQ(plan.legs[0].price, 100.0);
  EXPECT_DOUBLE_EQ(plan.legs[1].price, 99.8);
}

TEST(RouterConstraints, RejectsInvalidControls) {
  const std::vector<VenueQuote> quotes{
      quote(
          "coinbase",
          {{99.0, 1.0}},
          {{100.0, 1.0}}
      )
  };

  EXPECT_THROW(
      build_route_plan(
          RouteRequest{
              RouteSide::Buy,
              1.0,
              -1.0
          },
          quotes
      ),
      std::invalid_argument
  );

  EXPECT_THROW(
      build_route_plan(
          RouteRequest{
              RouteSide::Buy,
              1.0,
              0.0,
              -1.0
          },
          quotes
      ),
      std::invalid_argument
  );

  EXPECT_THROW(
      build_route_plan(
          RouteRequest{
              RouteSide::Buy,
              1.0,
              0.0,
              10.0,
              0
          },
          quotes
      ),
      std::invalid_argument
  );
}


}  // namespace
