#include "minimatch/execution.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace minimatch {
std::vector<ExecutionSlice> twap_schedule(Quantity total_quantity, std::size_t intervals) {
  if (total_quantity == 0 || intervals == 0) throw std::invalid_argument("invalid TWAP inputs");
  std::vector<ExecutionSlice> out(intervals);
  const Quantity base = total_quantity / static_cast<Quantity>(intervals);
  const Quantity remainder = total_quantity % static_cast<Quantity>(intervals);
  for (std::size_t i = 0; i < intervals; ++i) {
    const Quantity extra = i < static_cast<std::size_t>(remainder) ? Quantity{1} : Quantity{0};
    out[i] = {i, static_cast<Quantity>(base + extra), 1.0 / static_cast<double>(intervals)};
  }
  return out;
}

std::vector<ExecutionSlice> vwap_schedule(Quantity total_quantity,
                                          const std::vector<double>& volume_curve) {
  if (total_quantity == 0 || volume_curve.empty()) throw std::invalid_argument("invalid VWAP inputs");
  const double total_weight = std::accumulate(volume_curve.begin(), volume_curve.end(), 0.0);
  if (total_weight <= 0.0) throw std::invalid_argument("volume weights must be positive");
  std::vector<ExecutionSlice> out(volume_curve.size());
  Quantity allocated = 0;
  for (std::size_t i = 0; i < volume_curve.size(); ++i) {
    const double weight = std::max(volume_curve[i], 0.0) / total_weight;
    Quantity q = i + 1 == volume_curve.size()
                     ? total_quantity - allocated
                     : static_cast<Quantity>(std::llround(weight * total_quantity));
    q = std::min(q, static_cast<Quantity>(total_quantity - allocated));
    allocated += q;
    out[i] = {i, q, weight};
  }
  if (allocated < total_quantity) out.back().quantity += total_quantity - allocated;
  return out;
}

ExecutionCost simulate_execution_cost(Side side, Price arrival_price,
                                      const std::vector<ExecutionSlice>& schedule,
                                      double daily_volume, double volatility,
                                      double temporary_impact_coefficient,
                                      double permanent_impact_coefficient) {
  if (schedule.empty() || daily_volume <= 0.0 || volatility < 0.0) {
    throw std::invalid_argument("invalid execution cost inputs");
  }
  const double sign = side == Side::Buy ? 1.0 : -1.0;
  double total_qty = 0.0, weighted_px = 0.0, permanent = 0.0, temporary = 0.0;
  for (const auto& slice : schedule) {
    const double q = slice.quantity;
    const double participation = q / daily_volume;
    const double temp = temporary_impact_coefficient * volatility * std::sqrt(participation);
    permanent += permanent_impact_coefficient * volatility * participation;
    const double px = static_cast<double>(arrival_price) * (1.0 + sign * (temp + permanent));
    weighted_px += px * q;
    total_qty += q;
    temporary += temp * q;
  }
  ExecutionCost out;
  out.arrival_price = static_cast<double>(arrival_price);
  out.average_fill_price = weighted_px / total_qty;
  out.implementation_shortfall = sign * (out.average_fill_price - out.arrival_price) * total_qty;
  out.temporary_impact = temporary / total_qty;
  out.permanent_impact = permanent;
  return out;
}
}  // namespace minimatch
