#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace minimatch {

enum class RouteSide {
  Buy,
  Sell
};

struct VenueQuote {
  std::string venue;
  double bid{0.0};
  double bid_quantity{0.0};
  double ask{0.0};
  double ask_quantity{0.0};
  double taker_fee_bps{0.0};
  double latency_ms{0.0};
  bool healthy{false};
};

struct RouteRequest {
  RouteSide side{RouteSide::Buy};
  double quantity{0.0};
};

struct RouteLeg {
  std::string venue;
  double price{0.0};
  double quantity{0.0};
  double estimated_fee{0.0};
};

struct RoutePlan {
  bool complete{false};
  double requested_quantity{0.0};
  double routed_quantity{0.0};
  double estimated_notional{0.0};
  double estimated_fees{0.0};
  double average_price{0.0};
  std::vector<RouteLeg> legs;
};

RoutePlan build_route_plan(
    const RouteRequest& request,
    const std::vector<VenueQuote>& quotes
);

}  // namespace minimatch
