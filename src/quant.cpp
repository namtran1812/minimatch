#include "minimatch/quant.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>

namespace minimatch {
namespace {
constexpr double pi = 3.14159265358979323846;
double normal_pdf(double x) { return std::exp(-0.5 * x * x) / std::sqrt(2.0 * pi); }
double normal_cdf(double x) { return 0.5 * std::erfc(-x / std::sqrt(2.0)); }
double payoff(OptionKind kind, double terminal, double strike) {
  return kind == OptionKind::Call ? std::max(terminal - strike, 0.0)
                                  : std::max(strike - terminal, 0.0);
}
void validate_option(const OptionInputs& in) {
  if (in.spot <= 0.0 || in.strike <= 0.0 || in.volatility <= 0.0 ||
      in.time_to_expiry <= 0.0) {
    throw std::invalid_argument("option inputs must be positive");
  }
}
}  // namespace

OptionResult black_scholes(OptionKind kind, const OptionInputs& in) {
  validate_option(in);
  const double sqrt_t = std::sqrt(in.time_to_expiry);
  const double d1 = (std::log(in.spot / in.strike) +
                     (in.rate - in.dividend_yield + 0.5 * in.volatility * in.volatility) *
                         in.time_to_expiry) /
                    (in.volatility * sqrt_t);
  const double d2 = d1 - in.volatility * sqrt_t;
  const double disc_r = std::exp(-in.rate * in.time_to_expiry);
  const double disc_q = std::exp(-in.dividend_yield * in.time_to_expiry);
  OptionResult out;
  if (kind == OptionKind::Call) {
    out.price = in.spot * disc_q * normal_cdf(d1) - in.strike * disc_r * normal_cdf(d2);
    out.delta = disc_q * normal_cdf(d1);
    out.theta = -(in.spot * disc_q * normal_pdf(d1) * in.volatility) / (2.0 * sqrt_t) -
                in.rate * in.strike * disc_r * normal_cdf(d2) +
                in.dividend_yield * in.spot * disc_q * normal_cdf(d1);
    out.rho = in.strike * in.time_to_expiry * disc_r * normal_cdf(d2);
  } else {
    out.price = in.strike * disc_r * normal_cdf(-d2) - in.spot * disc_q * normal_cdf(-d1);
    out.delta = disc_q * (normal_cdf(d1) - 1.0);
    out.theta = -(in.spot * disc_q * normal_pdf(d1) * in.volatility) / (2.0 * sqrt_t) +
                in.rate * in.strike * disc_r * normal_cdf(-d2) -
                in.dividend_yield * in.spot * disc_q * normal_cdf(-d1);
    out.rho = -in.strike * in.time_to_expiry * disc_r * normal_cdf(-d2);
  }
  out.gamma = disc_q * normal_pdf(d1) / (in.spot * in.volatility * sqrt_t);
  out.vega = in.spot * disc_q * normal_pdf(d1) * sqrt_t;
  return out;
}

double implied_volatility(OptionKind kind, double market_price, OptionInputs in,
                          double tolerance, std::size_t max_iterations) {
  if (market_price <= 0.0) throw std::invalid_argument("market price must be positive");
  in.volatility = std::clamp(in.volatility, 1e-4, 5.0);
  for (std::size_t i = 0; i < max_iterations; ++i) {
    const auto result = black_scholes(kind, in);
    const double error = result.price - market_price;
    if (std::abs(error) < tolerance) return in.volatility;
    if (result.vega < 1e-12) break;
    in.volatility = std::clamp(in.volatility - error / result.vega, 1e-4, 5.0);
  }
  double lo = 1e-4, hi = 5.0;
  for (std::size_t i = 0; i < max_iterations; ++i) {
    in.volatility = 0.5 * (lo + hi);
    const double px = black_scholes(kind, in).price;
    if (std::abs(px - market_price) < tolerance) return in.volatility;
    if (px > market_price) hi = in.volatility; else lo = in.volatility;
  }
  return in.volatility;
}

double monte_carlo_option_price(OptionKind kind, const OptionInputs& in,
                                std::size_t paths, unsigned seed) {
  validate_option(in);
  if (paths == 0) throw std::invalid_argument("paths must be nonzero");
  std::mt19937 rng(seed);
  std::normal_distribution<double> normal(0.0, 1.0);
  const double drift = (in.rate - in.dividend_yield - 0.5 * in.volatility * in.volatility) *
                       in.time_to_expiry;
  const double diffusion = in.volatility * std::sqrt(in.time_to_expiry);
  double sum = 0.0;
  for (std::size_t i = 0; i < paths; ++i) {
    const double z = normal(rng);
    const double terminal = in.spot * std::exp(drift + diffusion * z);
    const double anti = in.spot * std::exp(drift - diffusion * z);
    sum += 0.5 * (payoff(kind, terminal, in.strike) + payoff(kind, anti, in.strike));
  }
  return std::exp(-in.rate * in.time_to_expiry) * sum / static_cast<double>(paths);
}

RiskSummary historical_risk(const std::vector<double>& returns, double confidence) {
  if (returns.size() < 2 || confidence <= 0.5 || confidence >= 1.0) {
    throw std::invalid_argument("invalid historical risk inputs");
  }
  RiskSummary out;
  out.mean = std::accumulate(returns.begin(), returns.end(), 0.0) /
             static_cast<double>(returns.size());
  double variance = 0.0;
  for (double r : returns) variance += (r - out.mean) * (r - out.mean);
  out.volatility = std::sqrt(variance / static_cast<double>(returns.size() - 1));
  std::vector<double> sorted = returns;
  std::sort(sorted.begin(), sorted.end());
  const std::size_t tail_count = std::max<std::size_t>(1, static_cast<std::size_t>(
      std::ceil((1.0 - confidence) * static_cast<double>(sorted.size()))));
  out.value_at_risk = -sorted[tail_count - 1];
  const double tail_sum = std::accumulate(sorted.begin(), sorted.begin() +
      static_cast<std::ptrdiff_t>(tail_count), 0.0);
  out.expected_shortfall = -tail_sum / static_cast<double>(tail_count);
  return out;
}

PairsModel fit_pairs_model(const std::vector<double>& x, const std::vector<double>& y) {
  if (x.size() != y.size() || x.size() < 20) throw std::invalid_argument("invalid pairs data");
  const double mx = std::accumulate(x.begin(), x.end(), 0.0) / x.size();
  const double my = std::accumulate(y.begin(), y.end(), 0.0) / y.size();
  double cov = 0.0, varx = 0.0;
  for (std::size_t i = 0; i < x.size(); ++i) {
    cov += (x[i] - mx) * (y[i] - my);
    varx += (x[i] - mx) * (x[i] - mx);
  }
  PairsModel model;
  model.beta = cov / varx;
  model.alpha = my - model.beta * mx;
  std::vector<double> spread(x.size());
  for (std::size_t i = 0; i < x.size(); ++i) spread[i] = y[i] - model.alpha - model.beta * x[i];
  model.spread_mean = std::accumulate(spread.begin(), spread.end(), 0.0) / spread.size();
  double ss = 0.0;
  for (double s : spread) ss += (s - model.spread_mean) * (s - model.spread_mean);
  model.spread_stddev = std::sqrt(ss / static_cast<double>(spread.size() - 1));

  double sum_xx = 0.0, sum_xy = 0.0;
  for (std::size_t i = 1; i < spread.size(); ++i) {
    sum_xx += spread[i - 1] * spread[i - 1];
    sum_xy += spread[i - 1] * (spread[i] - spread[i - 1]);
  }
  const double gamma = sum_xy / sum_xx;
  double residual_ss = 0.0;
  for (std::size_t i = 1; i < spread.size(); ++i) {
    const double residual = (spread[i] - spread[i - 1]) - gamma * spread[i - 1];
    residual_ss += residual * residual;
  }
  const double sigma2 = residual_ss / static_cast<double>(spread.size() - 2);
  const double se = std::sqrt(sigma2 / sum_xx);
  model.adf_t_stat = gamma / se;
  model.stationary = model.adf_t_stat < -2.86;
  return model;
}

std::vector<double> spread_zscores(const std::vector<double>& x,
                                   const std::vector<double>& y,
                                   const PairsModel& model) {
  if (x.size() != y.size() || model.spread_stddev <= 0.0) {
    throw std::invalid_argument("invalid zscore inputs");
  }
  std::vector<double> out(x.size());
  for (std::size_t i = 0; i < x.size(); ++i) {
    const double spread = y[i] - model.alpha - model.beta * x[i];
    out[i] = (spread - model.spread_mean) / model.spread_stddev;
  }
  return out;
}

}  // namespace minimatch
