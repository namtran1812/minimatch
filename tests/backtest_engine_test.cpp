#include "minimatch/backtest_engine.hpp"

#include <gtest/gtest.h>

namespace {

using minimatch::BacktestRequest;
using minimatch::ExecutionAlgorithm;
using minimatch::ExecutionScheduleRequest;
using minimatch::MarketEventType;
using minimatch::MarketReplay;
using minimatch::MarketReplayEvent;
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

}  // namespace
