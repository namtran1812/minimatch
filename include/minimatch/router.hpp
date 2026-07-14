#pragma once

#include <string>
#include <vector>

namespace minimatch {

enum class RouteSide {
  Buy,
  Sell
};

struct VenueLevel {
  double price{0.0};
  double quantity{0.0};
};

struct VenueQuote {
  std::string venue;

  std::vector<VenueLevel> bids;
  std::vector<VenueLevel> asks;

  double taker_fee_bps{0.0};
  double latency_ms{0.0};
  double latency_cost_bps_per_ms{0.0};

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
  double effective_price{0.0};

  double latency_ms{0.0};
  double taker_fee_bps{0.0};
  double latency_cost_bps_per_ms{0.0};

  std::size_t level_index{0};
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
