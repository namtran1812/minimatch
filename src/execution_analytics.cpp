#include "minimatch/execution_analytics.hpp"

#include <cmath>

namespace minimatch {

ExecutionQuality
ExecutionAnalytics::analyze(
    const ParentOrder& parent,
    const std::vector<
        OmsExecutionReport
    >& fills,
    double arrival_price,
    const std::vector<
        ExecutionMarkout
    >& markouts
) const {
  ExecutionQuality result;

  result.parent_order_id =
      parent.id;

  result.symbol =
      parent.symbol;

  result.side =
      parent.side;

  result.requested_quantity =
      parent.quantity;

  result.arrival_price =
      arrival_price;

  for (const auto& fill : fills) {
    result.filled_quantity +=
        fill.quantity;

    result.total_notional +=
        fill.notional;

    result.total_fees +=
        fill.fee;
  }

  if (
      result.filled_quantity >
      1e-12
  ) {
    result.average_fill_price =
        result.total_notional /
        result.filled_quantity;
  }

  if (
      result.arrival_price >
          1e-12 &&
      result.average_fill_price >
          1e-12
  ) {
    const double signed_diff =
        parent.side ==
                RouteSide::Buy
            ? (
                result
                    .average_fill_price -
                result
                    .arrival_price
              )
            : (
                result
                    .arrival_price -
                result
                    .average_fill_price
              );

    result
        .implementation_shortfall_bps =
        signed_diff /
        result.arrival_price *
        10000.0;

    const double fee_bps =
        result.total_notional >
                1e-12
            ? (
                result.total_fees /
                result.total_notional *
                10000.0
              )
            : 0.0;

    result
        .fee_adjusted_shortfall_bps =
        result
            .implementation_shortfall_bps +
        fee_bps;
  }

  result.markouts =
      markouts;

  return result;
}

}  // namespace minimatch
