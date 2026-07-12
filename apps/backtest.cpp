#include "minimatch/analytics.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace {
std::vector<double> load_prices(const std::string& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot open price file");
  std::vector<double> prices;
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    std::stringstream ss(line);
    std::string token, last;
    while (std::getline(ss, token, ',')) last = token;
    try { prices.push_back(std::stod(last)); } catch (...) {}
  }
  return prices;
}

std::vector<double> synthetic_prices(std::size_t n) {
  std::mt19937_64 rng(42);
  std::normal_distribution<double> noise(0.0, 0.009);
  std::vector<double> p(n, 100.0);
  for (std::size_t i = 1; i < n; ++i) {
    const double regime = ((i / 250) % 2 == 0) ? 0.00045 : -0.00015;
    p[i] = p[i - 1] * std::exp(regime + noise(rng));
  }
  return p;
}
}

int main(int argc, char** argv) {
  const auto prices = argc > 1 ? load_prices(argv[1]) : synthetic_prices(4'000);
  if (prices.size() < 260) {
    std::cerr << "need at least 260 prices\n";
    return 1;
  }
  constexpr std::size_t fast = 20, slow = 100, vol_window = 20;
  constexpr double target_annual_vol = 0.15, periods = 252.0, cost_bps = 1.0;
  std::vector<double> returns;
  std::vector<double> equity{100'000.0};
  double previous_position = 0.0;
  double turnover = 0.0;
  for (std::size_t i = slow; i < prices.size(); ++i) {
    const auto avg = [&](std::size_t w) {
      double sum = 0.0;
      for (std::size_t j = i + 1 - w; j <= i; ++j) sum += prices[j];
      return sum / static_cast<double>(w);
    };
    const double signal = avg(fast) > avg(slow) ? 1.0 : -1.0;
    double variance = 0.0, mean = 0.0;
    for (std::size_t j = i - vol_window; j < i; ++j)
      mean += std::log(prices[j + 1] / prices[j]);
    mean /= static_cast<double>(vol_window);
    for (std::size_t j = i - vol_window; j < i; ++j) {
      const double r = std::log(prices[j + 1] / prices[j]);
      variance += (r - mean) * (r - mean);
    }
    const double realized = std::sqrt(variance / static_cast<double>(vol_window - 1)) * std::sqrt(periods);
    const double leverage = realized > 1e-12 ? std::clamp(target_annual_vol / realized, 0.0, 2.0) : 0.0;
    const double position = signal * leverage;
    const double asset_return = prices[i] / prices[i - 1] - 1.0;
    const double trade = std::abs(position - previous_position);
    turnover += trade;
    const double strategy_return = previous_position * asset_return - trade * cost_bps * 1e-4;
    returns.push_back(strategy_return);
    equity.push_back(equity.back() * (1.0 + strategy_return));
    previous_position = position;
  }
  const auto m = minimatch::calculate_metrics(returns, equity, periods, turnover);
  std::cout << std::fixed << std::setprecision(6)
            << "observations=" << returns.size() << '\n'
            << "final_equity=" << equity.back() << '\n'
            << "total_return=" << m.total_return << '\n'
            << "annualized_return=" << m.annualized_return << '\n'
            << "annualized_volatility=" << m.annualized_volatility << '\n'
            << "sharpe=" << m.sharpe << '\n'
            << "max_drawdown=" << m.max_drawdown << '\n'
            << "win_rate=" << m.win_rate << '\n'
            << "turnover=" << m.turnover << '\n';
}
