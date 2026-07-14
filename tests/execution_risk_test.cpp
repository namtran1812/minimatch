#include "minimatch/execution_risk.hpp"

#include <gtest/gtest.h>

namespace {

using minimatch::ExecutionRiskCheck;
using minimatch::ExecutionRiskLimits;
using minimatch::ExecutionRiskManager;
using minimatch::ExecutionRiskRejectReason;
using minimatch::RouteSide;

ExecutionRiskCheck valid_buy() {
  return ExecutionRiskCheck{
      .side = RouteSide::Buy,
      .parent_quantity = 10.0,
      .child_quantity = 1.0,
      .order_price = 100.0,
      .reference_price = 100.0,
      .observed_market_volume = 20.0
  };
}

TEST(ExecutionRisk, AcceptsValidOrder) {
  const ExecutionRiskManager manager(
      ExecutionRiskLimits{
          .max_parent_quantity = 100.0,
          .max_child_quantity = 10.0,
          .max_order_notional = 10000.0,
          .max_participation_rate = 0.20,
          .price_collar_bps = 50.0
      }
  );

  const auto decision =
      manager.check(valid_buy());

  EXPECT_TRUE(decision.accepted);
  EXPECT_EQ(
      decision.reason,
      ExecutionRiskRejectReason::None
  );
}

TEST(ExecutionRisk, KillSwitchRejectsOrders) {
  ExecutionRiskManager manager;

  manager.activate_kill_switch();

  const auto decision =
      manager.check(valid_buy());

  EXPECT_FALSE(decision.accepted);
  EXPECT_EQ(
      decision.reason,
      ExecutionRiskRejectReason::KillSwitchActive
  );

  manager.deactivate_kill_switch();

  EXPECT_TRUE(
      manager.check(valid_buy()).accepted
  );
}

TEST(ExecutionRisk, RejectsParentQuantityLimit) {
  const ExecutionRiskManager manager(
      ExecutionRiskLimits{
          .max_parent_quantity = 5.0
      }
  );

  const auto decision =
      manager.check(valid_buy());

  EXPECT_EQ(
      decision.reason,
      ExecutionRiskRejectReason::ParentQuantityExceeded
  );
}

TEST(ExecutionRisk, RejectsChildQuantityLimit) {
  const ExecutionRiskManager manager(
      ExecutionRiskLimits{
          .max_child_quantity = 0.5
      }
  );

  const auto decision =
      manager.check(valid_buy());

  EXPECT_EQ(
      decision.reason,
      ExecutionRiskRejectReason::ChildQuantityExceeded
  );
}

TEST(ExecutionRisk, RejectsNotionalLimit) {
  const ExecutionRiskManager manager(
      ExecutionRiskLimits{
          .max_order_notional = 50.0
      }
  );

  const auto decision =
      manager.check(valid_buy());

  EXPECT_EQ(
      decision.reason,
      ExecutionRiskRejectReason::NotionalExceeded
  );
}

TEST(ExecutionRisk, RejectsParticipationLimit) {
  const ExecutionRiskManager manager(
      ExecutionRiskLimits{
          .max_participation_rate = 0.04
      }
  );

  const auto decision =
      manager.check(valid_buy());

  EXPECT_EQ(
      decision.reason,
      ExecutionRiskRejectReason::ParticipationExceeded
  );
}

TEST(ExecutionRisk, RejectsBuyAbovePriceCollar) {
  const ExecutionRiskManager manager(
      ExecutionRiskLimits{
          .price_collar_bps = 10.0
      }
  );

  auto request = valid_buy();
  request.order_price = 101.0;

  const auto decision =
      manager.check(request);

  EXPECT_EQ(
      decision.reason,
      ExecutionRiskRejectReason::PriceCollarExceeded
  );
}

TEST(ExecutionRisk, RejectsSellBelowPriceCollar) {
  const ExecutionRiskManager manager(
      ExecutionRiskLimits{
          .price_collar_bps = 10.0
      }
  );

  auto request = valid_buy();
  request.side = RouteSide::Sell;
  request.order_price = 99.0;

  const auto decision =
      manager.check(request);

  EXPECT_EQ(
      decision.reason,
      ExecutionRiskRejectReason::PriceCollarExceeded
  );
}

TEST(ExecutionRisk, AllowsFavorablePriceMovement) {
  const ExecutionRiskManager manager(
      ExecutionRiskLimits{
          .price_collar_bps = 1.0
      }
  );

  auto buy = valid_buy();
  buy.order_price = 99.0;

  EXPECT_TRUE(
      manager.check(buy).accepted
  );

  auto sell = valid_buy();
  sell.side = RouteSide::Sell;
  sell.order_price = 101.0;

  EXPECT_TRUE(
      manager.check(sell).accepted
  );
}

TEST(ExecutionRisk, RejectsInvalidConfiguration) {
  EXPECT_THROW(
      ExecutionRiskManager(
          ExecutionRiskLimits{
              .max_participation_rate = 0.0
          }
      ),
      std::invalid_argument
  );

  EXPECT_THROW(
      ExecutionRiskManager(
          ExecutionRiskLimits{
              .price_collar_bps = -1.0
          }
      ),
      std::invalid_argument
  );
}

TEST(ExecutionRisk, RejectsInvalidOrderValues) {
  const ExecutionRiskManager manager;

  auto request = valid_buy();
  request.child_quantity = 0.0;

  const auto decision =
      manager.check(request);

  EXPECT_EQ(
      decision.reason,
      ExecutionRiskRejectReason::InvalidOrder
  );
}

}  // namespace
