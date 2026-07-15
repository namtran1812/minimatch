#pragma once

#include "minimatch/router.hpp"
#include "minimatch/market_data.hpp"

namespace minimatch {

std::vector<VenueQuote>
market_data_to_quotes(
    const ConsolidatedMarketData& market,
    const std::string& symbol,
    double taker_fee_bps = 0.0,
    double latency_ms = 0.0,
    double latency_cost_bps_per_ms = 0.0
);

RoutePlan build_route_plan(
    const RouteRequest& request,
    const ConsolidatedMarketData& market,
    const std::string& symbol,
    double taker_fee_bps = 0.0,
    double latency_ms = 0.0,
    double latency_cost_bps_per_ms = 0.0
);

}
