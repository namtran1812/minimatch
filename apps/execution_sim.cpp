#include "minimatch/execution.hpp"
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  using namespace minimatch;
  const auto twap = twap_schedule(100000, 10);
  const std::vector<double> curve{0.16, 0.12, 0.09, 0.08, 0.07, 0.07, 0.08, 0.09, 0.11, 0.13};
  const auto vwap = vwap_schedule(100000, curve);
  const auto twap_cost = simulate_execution_cost(Side::Buy, 10000, twap, 5000000.0, 0.02);
  const auto vwap_cost = simulate_execution_cost(Side::Buy, 10000, vwap, 5000000.0, 0.02);
  std::cout << std::fixed << std::setprecision(6)
            << "twap_avg_fill=" << twap_cost.average_fill_price << '\n'
            << "twap_shortfall=" << twap_cost.implementation_shortfall << '\n'
            << "vwap_avg_fill=" << vwap_cost.average_fill_price << '\n'
            << "vwap_shortfall=" << vwap_cost.implementation_shortfall << '\n';
}
