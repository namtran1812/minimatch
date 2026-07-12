#pragma once
#include <cstddef>
#include <vector>

namespace minimatch {

enum class OptionKind { Call, Put };

struct OptionInputs {
  double spot{};
  double strike{};
  double rate{};
  double dividend_yield{};
  double volatility{};
  double time_to_expiry{};
};

struct OptionResult {
  double price{};
  double delta{};
  double gamma{};
  double vega{};
  double theta{};
  double rho{};
};

OptionResult black_scholes(OptionKind kind, const OptionInputs& in);
double implied_volatility(OptionKind kind, double market_price, OptionInputs in,
                          double tolerance = 1e-8, std::size_t max_iterations = 100);
double monte_carlo_option_price(OptionKind kind, const OptionInputs& in,
                                std::size_t paths, unsigned seed = 42);

struct RiskSummary {
  double mean{};
  double volatility{};
  double value_at_risk{};
  double expected_shortfall{};
};

RiskSummary historical_risk(const std::vector<double>& returns, double confidence = 0.99);

struct PairsModel {
  double alpha{};
  double beta{};
  double spread_mean{};
  double spread_stddev{};
  double adf_t_stat{};
  bool stationary{};
};

PairsModel fit_pairs_model(const std::vector<double>& x, const std::vector<double>& y);
std::vector<double> spread_zscores(const std::vector<double>& x,
                                   const std::vector<double>& y,
                                   const PairsModel& model);

}  // namespace minimatch
