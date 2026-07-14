#pragma once

#include <vector>

namespace minimatch {

enum class ExecutionAlgorithm {
  Market,
  TWAP,
  VWAP,
  POV,
  Iceberg
};

struct Slice {
  double quantity{0.0};
  double delay_seconds{0.0};
};

struct ExecutionScheduleRequest {
  ExecutionAlgorithm algorithm{
      ExecutionAlgorithm::Market
  };

  double quantity{0.0};
  int slices{1};
  double duration_seconds{0.0};

  // VWAP: relative volume weights.
  // POV: expected market volume for each interval.
  std::vector<double> volume_profile;

  // POV participation rate between zero and one.
  double participation_rate{0.1};

  // Iceberg visible quantity per child order.
  double displayed_quantity{0.0};
};

std::vector<Slice> build_execution_schedule(
    const ExecutionScheduleRequest& request
);

// Backward-compatible overload.
std::vector<Slice> build_execution_schedule(
    ExecutionAlgorithm algorithm,
    double quantity,
    int slices,
    double duration_seconds
);

}  // namespace minimatch
