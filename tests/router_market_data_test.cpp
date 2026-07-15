#include "minimatch/router_market_data.hpp"

#include <gtest/gtest.h>

using namespace minimatch;

TEST(RouterMarketData, ConvertsBookToVenueQuotes) {
    ConsolidatedMarketData market;

    market.apply_snapshot(
        {
            "coinbase",
            "BTCUSD",
            1,
            1,
            {
                {100.0,2},
                {99.9,1}
            },
            {
                {100.1,3}
            }
        });

    auto quotes =
        market_data_to_quotes(
            market,
            "BTCUSD"
        );

    ASSERT_EQ(quotes.size(),1u);

    EXPECT_EQ(
        quotes[0].venue,
        "coinbase"
    );

    EXPECT_EQ(
        quotes[0].asks.size(),
        1u
    );
}

TEST(RouterMarketData, RoutesUsingMarketData) {
    ConsolidatedMarketData market;

    market.apply_snapshot(
        {
            "coinbase",
            "BTCUSD",
            1,
            1,
            {
                {100,2}
            },
            {
                {100.1,1}
            }
        });

    RouteRequest request{};
    request.side = RouteSide::Buy;
    request.quantity = 1.0;

    auto plan =
        build_route_plan(
            request,
            market,
            "BTCUSD"
        );

    EXPECT_TRUE(plan.complete);
    EXPECT_EQ(plan.legs.size(),1u);
}
