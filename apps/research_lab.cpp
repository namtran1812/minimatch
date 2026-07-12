#include "minimatch/quant.hpp"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

int main() {
  using namespace minimatch;
  std::mt19937 rng(7);
  std::normal_distribution<double> noise(0.0, 0.35);
  std::vector<double> x(1500), y(1500), returns;
  x[0] = 100.0;
  y[0] = 12.0 + 1.4 * x[0];
  for (std::size_t i = 1; i < x.size(); ++i) {
    x[i] = x[i - 1] + noise(rng);
    y[i] = 12.0 + 1.4 * x[i] + 0.75 * noise(rng);
    returns.push_back((x[i] - x[i - 1]) / x[i - 1]);
  }
  const auto model = fit_pairs_model(x, y);
  const auto z = spread_zscores(x, y, model);
  const auto risk = historical_risk(returns, 0.99);
  std::cout << std::fixed << std::setprecision(6)
            << "alpha=" << model.alpha << '\n'
            << "beta=" << model.beta << '\n'
            << "adf_t_stat=" << model.adf_t_stat << '\n'
            << "stationary=" << model.stationary << '\n'
            << "latest_zscore=" << z.back() << '\n'
            << "var_99=" << risk.value_at_risk << '\n'
            << "expected_shortfall_99=" << risk.expected_shortfall << '\n';
}
