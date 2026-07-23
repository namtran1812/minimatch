#pragma once

#include "minimatch/router.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace minimatch {

struct ExecutionFill {
  std::uint64_t order_id{0};
  std::string venue;

  double price{0.0};
  double quantity{0.0};
  double fee{0.0};

  std::uint64_t timestamp_ns{0};
};

struct VenueTca {
  std::string venue;

  double filled_quantity{0.0};
  double notional{0.0};
  double fees{0.0};
  double average_fill_price{0.0};

  double arrival_slippage_bps{0.0};
  double vwap_slippage_bps{0.0};
};

struct TcaReport {
  RouteSide side{RouteSide::Buy};

  double ordered_quantity{0.0};
  double filled_quantity{0.0};
  double unfilled_quantity{0.0};
  double fill_rate{0.0};

  double average_fill_price{0.0};
  double arrival_price{0.0};
  double market_vwap{0.0};
  double ending_price{0.0};

  double gross_notional{0.0};
  double fees{0.0};

  double arrival_slippage_bps{0.0};
  double vwap_slippage_bps{0.0};

  double realized_cost{0.0};
  double opportunity_cost{0.0};
  double implementation_shortfall{0.0};
  double implementation_shortfall_bps{0.0};

  double routing_regret{0.0};
  double routing_regret_bps{0.0};

  std::vector<VenueTca> venues;
};

[[nodiscard]]
TcaReport analyze_execution(
    const RouteRequest& request,
    const RoutePlan& route_plan,
    const std::vector<ExecutionFill>& fills,
    double arrival_price,
    double market_vwap,
    double ending_price
);

}  // namespace minimatch
