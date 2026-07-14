#include "minimatch/execution_engine.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace minimatch {

std::string to_string(ChildExecutionStatus status) {
  switch (status) {
    case ChildExecutionStatus::Filled:
      return "FILLED";

    case ChildExecutionStatus::PartiallyFilled:
      return "PARTIALLY_FILLED";

    case ChildExecutionStatus::Rejected:
      return "REJECTED";
  }

  return "UNKNOWN";
}

RoutedExecutionSummary simulate_route_execution(
    const RoutePlan& plan,
    const ExecutionSimulationConfig& config
) {
  if (!std::isfinite(config.fill_ratio) ||
      config.fill_ratio < 0.0 ||
      config.fill_ratio > 1.0) {
    throw std::invalid_argument(
        "fill ratio must be between zero and one"
    );
  }

  RoutedExecutionSummary summary;
  summary.requested_quantity = plan.requested_quantity;
  summary.children.reserve(plan.legs.size());

  for (const auto& leg : plan.legs) {
    RoutedChildExecution child;

    child.venue = leg.venue;
    child.level_index = leg.level_index;
    child.requested_quantity = leg.quantity;
    child.price = leg.price;
    child.latency_ms = leg.latency_ms;

    child.filled_quantity =
        leg.quantity * config.fill_ratio;

    child.filled_quantity = std::clamp(
        child.filled_quantity,
        0.0,
        leg.quantity
    );

    child.remaining_quantity =
        leg.quantity - child.filled_quantity;

    child.notional =
        child.filled_quantity * child.price;

    child.fee =
        child.notional *
        leg.taker_fee_bps /
        10000.0;

    if (child.filled_quantity <= 1e-12) {
      child.status = ChildExecutionStatus::Rejected;
    } else if (child.remaining_quantity <= 1e-12) {
      child.status = ChildExecutionStatus::Filled;
    } else {
      child.status =
          ChildExecutionStatus::PartiallyFilled;
    }

    summary.filled_quantity += child.filled_quantity;
    summary.total_notional += child.notional;
    summary.total_fees += child.fee;
    summary.total_latency_ms += child.latency_ms;

    summary.children.push_back(child);
  }

  summary.remaining_quantity = std::max(
      0.0,
      summary.requested_quantity -
      summary.filled_quantity
  );

  summary.complete =
      summary.remaining_quantity <= 1e-12;

  if (summary.filled_quantity > 0.0) {
    summary.average_fill_price =
        summary.total_notional /
        summary.filled_quantity;
  }

  return summary;
}

}  // namespace minimatch
