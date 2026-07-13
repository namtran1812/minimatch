#include "minimatch/execution.hpp"
#include "minimatch/quant.hpp"
#include "minimatch/queue_model.hpp"
#include <gtest/gtest.h>
#include <numeric>
#include <cmath>

using namespace minimatch;

TEST(Options, PutCallParityAndImpliedVolatility) {
  OptionInputs in{100.0, 100.0, 0.05, 0.02, 0.2, 1.0};
  const auto call = black_scholes(OptionKind::Call, in);
  const auto put = black_scholes(OptionKind::Put, in);
  const double rhs = in.spot * std::exp(-in.dividend_yield * in.time_to_expiry) -
                     in.strike * std::exp(-in.rate * in.time_to_expiry);
  EXPECT_NEAR(call.price - put.price, rhs, 1e-10);
  OptionInputs guess = in;
  guess.volatility = 0.4;
  EXPECT_NEAR(implied_volatility(OptionKind::Call, call.price, guess), 0.2, 1e-6);
}

TEST(Options, MonteCarloConvergesNearBlackScholes) {
  OptionInputs in{100.0, 105.0, 0.03, 0.0, 0.25, 0.75};
  const double bs = black_scholes(OptionKind::Call, in).price;
  const double mc = monte_carlo_option_price(OptionKind::Call, in, 300000, 123);
  EXPECT_NEAR(mc, bs, 0.15);
}

TEST(Execution, SchedulesConserveQuantity) {
  const auto twap = twap_schedule(101, 8);
  const auto vwap = vwap_schedule(101, {0.4, 0.3, 0.2, 0.1});
  auto sum = [](const auto& xs) {
    return std::accumulate(xs.begin(), xs.end(), 0u,
                           [](Quantity a, const ExecutionSlice& x) { return a + x.quantity; });
  };
  EXPECT_EQ(sum(twap), 101u);
  EXPECT_EQ(sum(vwap), 101u);
}

TEST(QueueModel, TradesConsumeAheadBeforeOwnOrder) {
  QueuePositionModel q(100, 50);
  q.on_trade(70);
  EXPECT_EQ(q.state().ahead, 30u);
  EXPECT_EQ(q.state().filled, 0u);
  q.on_trade(45);
  EXPECT_EQ(q.state().ahead, 0u);
  EXPECT_EQ(q.state().filled, 15u);
}

TEST(Pairs, RecoversSyntheticHedgeRatio) {
  std::vector<double> x(300), y(300);
  for (std::size_t i = 0; i < x.size(); ++i) {
    x[i] = 100.0 + static_cast<double>(i) * 0.1;
    y[i] = 5.0 + 1.75 * x[i] + 0.2 * std::sin(static_cast<double>(i));
  }
  const auto model = fit_pairs_model(x, y);
  EXPECT_NEAR(model.beta, 1.75, 0.01);
  EXPECT_NEAR(model.alpha, 5.0, 0.3);
}
