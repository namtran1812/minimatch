#include "minimatch/router_market_data.hpp"

namespace minimatch {

std::vector<VenueQuote>
market_data_to_quotes(
    const ConsolidatedMarketData& market,
    const std::string& symbol,
    double fee,
    double latency,
    double latency_cost
){
    std::vector<VenueQuote> quotes;

    for(const auto& venue :
        market.venues_for_symbol(symbol))
    {
        auto book =
            market.find_book(
                venue,
                symbol
            );

        if(!book || !book->synchronized())
            continue;

        VenueQuote quote;
        quote.venue=venue;
        quote.healthy=true;

        quote.taker_fee_bps=fee;
        quote.latency_ms=latency;
        quote.latency_cost_bps_per_ms=
            latency_cost;

        for(auto level :
            book->bids(50))
        {
            quote.bids.push_back(
                VenueLevel{
                    level.price,
                    level.quantity
                }
            );
        }

        for(auto level :
            book->asks(50))
        {
            quote.asks.push_back(
                VenueLevel{
                    level.price,
                    level.quantity
                }
            );
        }

        quotes.push_back(
            std::move(quote)
        );
    }

    return quotes;
}

RoutePlan build_route_plan(
    const RouteRequest& request,
    const ConsolidatedMarketData& market,
    const std::string& symbol,
    double fee,
    double latency,
    double latency_cost
){
    return build_route_plan(
        request,
        market_data_to_quotes(
            market,
            symbol,
            fee,
            latency,
            latency_cost
        )
    );
}

}
