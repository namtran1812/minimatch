#pragma once
#include "minimatch/types.hpp"
#include <cstddef>
#include <vector>

namespace minimatch {

struct ExecutionSlice {
  std::size_t interval{};
  Quantity quantity{};
  double participation_rate{};
};

std::vector<ExecutionSlice> twap_schedule(Quantity total_quantity, std::size_t intervals);
std::vector<ExecutionSlice> vwap_schedule(Quantity total_quantity,
                                          const std::vector<double>& volume_curve);

struct ExecutionCost {
  double arrival_price{};
  double average_fill_price{};
  double implementation_shortfall{};
  double temporary_impact{};
  double permanent_impact{};
};

ExecutionCost simulate_execution_cost(Side side, Price arrival_price,
                                      const std::vector<ExecutionSlice>& schedule,
                                      double daily_volume, double volatility,
                                      double temporary_impact_coefficient = 0.12,
                                      double permanent_impact_coefficient = 0.04);

}  // namespace minimatch
