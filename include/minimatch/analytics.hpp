#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <vector>

namespace minimatch {
struct PerformanceMetrics {
  double total_return{};
  double annualized_return{};
  double annualized_volatility{};
  double sharpe{};
  double max_drawdown{};
  double win_rate{};
  double turnover{};
};

inline PerformanceMetrics calculate_metrics(const std::vector<double>& returns,
                                             const std::vector<double>& equity,
                                             double periods_per_year,
                                             double turnover) {
  PerformanceMetrics m{};
  m.turnover = turnover;
  if (returns.empty() || equity.size() < 2) return m;
  const double mean = std::accumulate(returns.begin(), returns.end(), 0.0) /
                      static_cast<double>(returns.size());
  double variance = 0.0;
  std::size_t wins = 0;
  for (const double r : returns) {
    variance += (r - mean) * (r - mean);
    if (r > 0.0) ++wins;
  }
  variance /= static_cast<double>(returns.size() > 1 ? returns.size() - 1 : 1);
  const double vol = std::sqrt(variance);
  m.annualized_volatility = vol * std::sqrt(periods_per_year);
  m.sharpe = vol > 0.0 ? mean / vol * std::sqrt(periods_per_year) : 0.0;
  m.total_return = equity.back() / equity.front() - 1.0;
  const double years = static_cast<double>(returns.size()) / periods_per_year;
  m.annualized_return = years > 0.0 && equity.back() > 0.0
      ? std::pow(equity.back() / equity.front(), 1.0 / years) - 1.0 : 0.0;
  double peak = equity.front();
  for (const double e : equity) {
    peak = std::max(peak, e);
    if (peak > 0.0) m.max_drawdown = std::min(m.max_drawdown, e / peak - 1.0);
  }
  m.win_rate = static_cast<double>(wins) / static_cast<double>(returns.size());
  return m;
}
}  // namespace minimatch
