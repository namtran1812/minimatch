#include "minimatch/analytics.hpp"
#include "minimatch/risk.hpp"
#include <gtest/gtest.h>

using namespace minimatch;

TEST(RiskEngine, RejectsOversizedOrder) {
  RiskEngine risk(RiskLimits{100, 1'000, 1'000'000, 100'000});
  EXPECT_FALSE(risk.allow({1, 1, Side::Buy, OrderType::Limit, 100, 101, 1}));
}

TEST(RiskEngine, EnforcesPositionLimit) {
  RiskEngine risk(RiskLimits{100, 100, 10'000'000, 100'000});
  risk.on_fill(1, 1, Side::Buy, 100, 90);
  EXPECT_FALSE(risk.allow({2, 1, Side::Buy, OrderType::Limit, 100, 20, 1}));
  EXPECT_TRUE(risk.allow({3, 1, Side::Sell, OrderType::Limit, 100, 20, 1}));
}

TEST(RiskEngine, DailyLossTriggersKillSwitch) {
  RiskEngine risk(RiskLimits{100, 1'000, 10'000'000, 1'000});
  risk.set_realized_pnl(1, 1, -1'000);
  EXPECT_TRUE(risk.state(1, 1).killed);
  EXPECT_FALSE(risk.allow({1, 1, Side::Buy, OrderType::Limit, 100, 10, 1}));
}

TEST(Analytics, ComputesDrawdownAndWinRate) {
  std::vector<double> returns{0.10, -0.20, 0.10};
  std::vector<double> equity{100.0, 110.0, 88.0, 96.8};
  const auto m = calculate_metrics(returns, equity, 252.0, 3.0);
  EXPECT_NEAR(m.max_drawdown, -0.20, 1e-12);
  EXPECT_NEAR(m.win_rate, 2.0 / 3.0, 1e-12);
  EXPECT_NEAR(m.total_return, -0.032, 1e-12);
}
