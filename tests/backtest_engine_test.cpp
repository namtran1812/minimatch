#include "minimatch/backtest_engine.hpp"

#include <gtest/gtest.h>

namespace {

using minimatch::BacktestRequest;
using minimatch::ExecutionAlgorithm;
using minimatch::ExecutionScheduleRequest;
using minimatch::ExecutionRiskLimits;
using minimatch::ExecutionRiskRejectReason;
using minimatch::MarketEventType;
using minimatch::MarketReplay;
using minimatch::MarketReplayEvent;
using minimatch::OrderManagementSystem;
using minimatch::OrderStatus;
using minimatch::RouteSide;
using minimatch::run_historical_backtest;

MarketReplay sample_replay() {
  return MarketReplay(
      {
          MarketReplayEvent{
              .timestamp_ns = 1'000'000'000,
              .type = MarketEventType::Quote,
              .symbol = "btcusd",
              .venue = "coinbase",
              .bid_price = 99.0,
              .bid_quantity = 2.0,
              .ask_price = 100.0,
              .ask_quantity = 2.0
          },
          MarketReplayEvent{
              .timestamp_ns = 2'000'000'000,
              .type = MarketEventType::Trade,
              .symbol = "btcusd",
              .venue = "coinbase",
              .trade_price = 100.2,
              .trade_quantity = 1.0
          },
          MarketReplayEvent{
              .timestamp_ns = 3'000'000'000,
              .type = MarketEventType::Quote,
              .symbol = "btcusd",
              .venue = "coinbase",
              .bid_price = 100.0,
              .bid_quantity = 2.0,
              .ask_price = 101.0,
              .ask_quantity = 2.0
          },
          MarketReplayEvent{
              .timestamp_ns = 4'000'000'000,
              .type = MarketEventType::Trade,
              .symbol = "btcusd",
              .venue = "coinbase",
              .trade_price = 100.8,
              .trade_quantity = 1.0
          }
      }
  );
}

TEST(BacktestEngine, RunsTwapAgainstHistoricalQuotes) {
  const BacktestRequest request{
      .symbol = "btcusd",
      .side = RouteSide::Buy,
      .quantity = 2.0,
      .schedule = ExecutionScheduleRequest{
          .algorithm = ExecutionAlgorithm::TWAP,
          .quantity = 2.0,
          .slices = 2,
          .duration_seconds = 2.0
      }
  };

  const auto result =
      run_historical_backtest(
          request,
          sample_replay()
      );

  EXPECT_TRUE(result.complete);
  EXPECT_DOUBLE_EQ(result.filled_quantity, 2.0);
  EXPECT_DOUBLE_EQ(result.remaining_quantity, 0.0);

  ASSERT_EQ(result.fills.size(), 2U);
  EXPECT_DOUBLE_EQ(result.fills[0].price, 100.0);
  EXPECT_DOUBLE_EQ(result.fills[1].price, 101.0);

  EXPECT_DOUBLE_EQ(
      result.average_fill_price,
      100.5
  );
}

TEST(BacktestEngine, ReportsPartialFill) {
  const BacktestRequest request{
      .symbol = "btcusd",
      .side = RouteSide::Buy,
      .quantity = 10.0,
      .schedule = ExecutionScheduleRequest{
          .algorithm = ExecutionAlgorithm::Market,
          .quantity = 10.0,
          .slices = 1,
          .duration_seconds = 0.0
      }
  };

  const auto result =
      run_historical_backtest(
          request,
          sample_replay()
      );

  EXPECT_FALSE(result.complete);
  EXPECT_DOUBLE_EQ(result.filled_quantity, 2.0);
  EXPECT_DOUBLE_EQ(result.remaining_quantity, 8.0);
}

TEST(BacktestEngine, CalculatesMarketVwap) {
  const BacktestRequest request{
      .symbol = "btcusd",
      .side = RouteSide::Buy,
      .quantity = 1.0,
      .schedule = ExecutionScheduleRequest{
          .algorithm = ExecutionAlgorithm::Market,
          .quantity = 1.0,
          .slices = 1
      }
  };

  const auto result =
      run_historical_backtest(
          request,
          sample_replay()
      );

  EXPECT_DOUBLE_EQ(result.market_vwap, 100.5);
}

TEST(BacktestEngine, RejectsMissingSymbol) {
  const BacktestRequest request{
      .symbol = "ethusd",
      .side = RouteSide::Buy,
      .quantity = 1.0,
      .schedule = ExecutionScheduleRequest{
          .algorithm = ExecutionAlgorithm::Market,
          .quantity = 1.0,
          .slices = 1
      }
  };

  EXPECT_THROW(
      run_historical_backtest(
          request,
          sample_replay()
      ),
      std::runtime_error
  );
}

TEST(BacktestEngine, RejectsInvalidQuantity) {
  const BacktestRequest request{
      .symbol = "btcusd",
      .side = RouteSide::Buy,
      .quantity = 0.0,
      .schedule = ExecutionScheduleRequest{
          .algorithm = ExecutionAlgorithm::Market,
          .quantity = 1.0,
          .slices = 1
      }
  };

  EXPECT_THROW(
      run_historical_backtest(
          request,
          sample_replay()
      ),
      std::invalid_argument
  );
}


TEST(BacktestEngine, CreatesParentAndChildOrders) {
  OrderManagementSystem oms;

  const BacktestRequest request{
      .symbol = "btcusd",
      .side = RouteSide::Buy,
      .quantity = 2.0,
      .schedule = ExecutionScheduleRequest{
          .algorithm = ExecutionAlgorithm::TWAP,
          .quantity = 2.0,
          .slices = 2,
          .duration_seconds = 2.0
      }
  };

  const auto result =
      run_historical_backtest(
          request,
          sample_replay(),
          &oms
      );

  EXPECT_GT(result.parent_order_id, 0U);
  EXPECT_EQ(oms.parent_count(), 1U);
  EXPECT_EQ(oms.child_count(), 2U);
  EXPECT_EQ(oms.execution_report_count(), 2U);

  const auto parent =
      oms.find_parent(result.parent_order_id);

  ASSERT_TRUE(parent.has_value());
  EXPECT_EQ(parent->status, OrderStatus::Filled);
  EXPECT_EQ(result.parent_status, OrderStatus::Filled);

  for (const auto& child :
       oms.children_for_parent(
           result.parent_order_id
       )) {
    EXPECT_EQ(child.status, OrderStatus::Filled);
  }
}

TEST(BacktestEngine, CancelsIncompleteParent) {
  OrderManagementSystem oms;

  const BacktestRequest request{
      .symbol = "btcusd",
      .side = RouteSide::Buy,
      .quantity = 10.0,
      .schedule = ExecutionScheduleRequest{
          .algorithm = ExecutionAlgorithm::Market,
          .quantity = 10.0,
          .slices = 1,
          .duration_seconds = 0.0
      }
  };

  const auto result =
      run_historical_backtest(
          request,
          sample_replay(),
          &oms
      );

  EXPECT_FALSE(result.complete);
  EXPECT_EQ(
      result.parent_status,
      OrderStatus::Cancelled
  );

  const auto parent =
      oms.find_parent(result.parent_order_id);

  ASSERT_TRUE(parent.has_value());
  EXPECT_EQ(
      parent->status,
      OrderStatus::Cancelled
  );
}

TEST(BacktestEngine, FillsReferenceOmsRecords) {
  OrderManagementSystem oms;

  const BacktestRequest request{
      .symbol = "btcusd",
      .side = RouteSide::Buy,
      .quantity = 1.0,
      .schedule = ExecutionScheduleRequest{
          .algorithm = ExecutionAlgorithm::Market,
          .quantity = 1.0,
          .slices = 1
      }
  };

  const auto result =
      run_historical_backtest(
          request,
          sample_replay(),
          &oms
      );

  ASSERT_EQ(result.fills.size(), 1U);

  EXPECT_EQ(
      result.fills[0].parent_order_id,
      result.parent_order_id
  );

  EXPECT_GT(
      result.fills[0].child_order_id,
      0U
  );

  EXPECT_GT(
      result.fills[0].execution_report_id,
      0U
  );

  EXPECT_TRUE(
      oms.find_child(
          result.fills[0].child_order_id
      ).has_value()
  );
}



TEST(BacktestEngine, RejectsChildWhenKillSwitchIsActive) {
  OrderManagementSystem oms;

  const BacktestRequest request{
      .symbol = "btcusd",
      .side = RouteSide::Buy,
      .quantity = 1.0,
      .schedule = ExecutionScheduleRequest{
          .algorithm = ExecutionAlgorithm::Market,
          .quantity = 1.0,
          .slices = 1
      },
      .risk_limits = ExecutionRiskLimits{
          .kill_switch_active = true
      }
  };

  const auto result =
      run_historical_backtest(
          request,
          sample_replay(),
          &oms
      );

  EXPECT_FALSE(result.complete);
  EXPECT_DOUBLE_EQ(result.filled_quantity, 0.0);
  EXPECT_EQ(result.accepted_child_count, 0U);
  EXPECT_EQ(result.rejected_child_count, 1U);

  ASSERT_EQ(result.fills.size(), 1U);
  EXPECT_FALSE(result.fills[0].risk_accepted);

  EXPECT_EQ(
      result.fills[0].risk_reject_reason,
      ExecutionRiskRejectReason::KillSwitchActive
  );

  const auto child =
      oms.find_child(
          result.fills[0].child_order_id
      );

  ASSERT_TRUE(child.has_value());
  EXPECT_EQ(child->status, OrderStatus::Rejected);
}

TEST(BacktestEngine, RejectsChildAboveQuantityLimit) {
  OrderManagementSystem oms;

  const BacktestRequest request{
      .symbol = "btcusd",
      .side = RouteSide::Buy,
      .quantity = 2.0,
      .schedule = ExecutionScheduleRequest{
          .algorithm = ExecutionAlgorithm::Market,
          .quantity = 2.0,
          .slices = 1
      },
      .risk_limits = ExecutionRiskLimits{
          .max_child_quantity = 0.5
      }
  };

  const auto result =
      run_historical_backtest(
          request,
          sample_replay(),
          &oms
      );

  EXPECT_EQ(result.rejected_child_count, 1U);
  EXPECT_DOUBLE_EQ(result.filled_quantity, 0.0);

  ASSERT_EQ(result.fills.size(), 1U);

  EXPECT_EQ(
      result.fills[0].risk_reject_reason,
      ExecutionRiskRejectReason::ChildQuantityExceeded
  );
}

TEST(BacktestEngine, AcceptsChildWithinRiskLimits) {
  OrderManagementSystem oms;

  const BacktestRequest request{
      .symbol = "btcusd",
      .side = RouteSide::Buy,
      .quantity = 1.0,
      .schedule = ExecutionScheduleRequest{
          .algorithm = ExecutionAlgorithm::Market,
          .quantity = 1.0,
          .slices = 1
      },
      .risk_limits = ExecutionRiskLimits{
          .max_parent_quantity = 10.0,
          .max_child_quantity = 2.0,
          .max_order_notional = 1000.0,
          .max_participation_rate = 1.0,
          .price_collar_bps = 100.0
      }
  };

  const auto result =
      run_historical_backtest(
          request,
          sample_replay(),
          &oms
      );

  EXPECT_EQ(result.accepted_child_count, 1U);
  EXPECT_EQ(result.rejected_child_count, 0U);
  EXPECT_TRUE(result.fills[0].risk_accepted);
}


}  // namespace
