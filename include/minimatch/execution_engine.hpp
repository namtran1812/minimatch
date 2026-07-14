#pragma once

#include "minimatch/router.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace minimatch {

enum class ChildExecutionStatus {
  Filled,
  PartiallyFilled,
  Rejected
};

struct RoutedChildExecution {
  std::string venue;
  std::size_t level_index{0};

  double requested_quantity{0.0};
  double filled_quantity{0.0};
  double remaining_quantity{0.0};

  double price{0.0};
  double notional{0.0};
  double fee{0.0};
  double latency_ms{0.0};

  ChildExecutionStatus status{
      ChildExecutionStatus::Rejected
  };
};

struct RoutedExecutionSummary {
  bool complete{false};

  double requested_quantity{0.0};
  double filled_quantity{0.0};
  double remaining_quantity{0.0};

  double average_fill_price{0.0};
  double total_notional{0.0};
  double total_fees{0.0};
  double total_latency_ms{0.0};

  std::vector<RoutedChildExecution> children;
};

struct ExecutionSimulationConfig {
  // 1.0 means every routed leg fills completely.
  double fill_ratio{1.0};
};

RoutedExecutionSummary simulate_route_execution(
    const RoutePlan& plan,
    const ExecutionSimulationConfig& config = {}
);

std::string to_string(ChildExecutionStatus status);

}  // namespace minimatch
