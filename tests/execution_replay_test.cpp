#include "minimatch/execution_replay.hpp"

#include <gtest/gtest.h>

namespace {

using minimatch::ChildExecutionStatus;
using minimatch::ExecutionReplay;
using minimatch::ReplayEventType;
using minimatch::RoutedChildExecution;
using minimatch::StoredRouteExecution;
using minimatch::build_execution_replay;

StoredRouteExecution completed_execution() {
  StoredRouteExecution execution;

  execution.execution_id = 42;
  execution.symbol = "btcusd";
  execution.side = "BUY";
  execution.complete = true;

  execution.requested_quantity = 1.0;
  execution.filled_quantity = 1.0;
  execution.remaining_quantity = 0.0;
  execution.total_fees = 0.15;
  execution.total_latency_ms = 3.0;

  execution.children = {
      RoutedChildExecution{
          "coinbase",
          0,
          0.4,
          0.4,
          0.0,
          100.0,
          40.0,
          0.04,
          1.0,
          ChildExecutionStatus::Filled
      },
      RoutedChildExecution{
          "kraken",
          1,
          0.6,
          0.6,
          0.0,
          101.0,
          60.6,
          0.11,
          2.0,
          ChildExecutionStatus::Filled
      }
  };

  return execution;
}

TEST(ExecutionReplay, BuildsDeterministicTimeline) {
  const ExecutionReplay replay =
      build_execution_replay(
          completed_execution()
      );

  EXPECT_EQ(replay.execution_id, 42);
  EXPECT_EQ(replay.symbol, "btcusd");
  EXPECT_EQ(replay.side, "BUY");
  EXPECT_TRUE(replay.complete);

  ASSERT_EQ(replay.events.size(), 6U);

  EXPECT_EQ(
      replay.events[0].type,
      ReplayEventType::ParentStarted
  );

  EXPECT_EQ(
      replay.events[1].type,
      ReplayEventType::ChildSubmitted
  );

  EXPECT_EQ(
      replay.events[2].type,
      ReplayEventType::ChildFilled
  );

  EXPECT_EQ(
      replay.events[5].type,
      ReplayEventType::ParentCompleted
  );
}

TEST(ExecutionReplay, PreservesChildOrder) {
  const auto replay =
      build_execution_replay(
          completed_execution()
      );

  EXPECT_EQ(replay.events[1].venue, "coinbase");
  EXPECT_EQ(replay.events[2].venue, "coinbase");

  EXPECT_EQ(replay.events[3].venue, "kraken");
  EXPECT_EQ(replay.events[4].venue, "kraken");
}

TEST(ExecutionReplay, AccumulatesLatency) {
  const auto replay =
      build_execution_replay(
          completed_execution()
      );

  EXPECT_DOUBLE_EQ(
      replay.events[2].timestamp_ms,
      1.0
  );

  EXPECT_DOUBLE_EQ(
      replay.events[4].timestamp_ms,
      3.0
  );

  EXPECT_DOUBLE_EQ(
      replay.events.back().timestamp_ms,
      3.0
  );
}

TEST(ExecutionReplay, RepresentsRejectedChild) {
  auto execution = completed_execution();

  execution.complete = false;
  execution.filled_quantity = 0.4;
  execution.remaining_quantity = 0.6;

  execution.children[1].filled_quantity = 0.0;
  execution.children[1].remaining_quantity = 0.6;
  execution.children[1].notional = 0.0;
  execution.children[1].fee = 0.0;
  execution.children[1].status =
      ChildExecutionStatus::Rejected;

  const auto replay =
      build_execution_replay(execution);

  EXPECT_EQ(
      replay.events[4].type,
      ReplayEventType::ChildRejected
  );

  EXPECT_EQ(
      replay.events.back().type,
      ReplayEventType::ParentIncomplete
  );

  EXPECT_DOUBLE_EQ(
      replay.events.back().filled_quantity,
      0.4
  );

  EXPECT_DOUBLE_EQ(
      replay.events.back().remaining_quantity,
      0.6
  );
}

TEST(ExecutionReplay, RepresentsPartialFill) {
  auto execution = completed_execution();

  execution.complete = false;
  execution.filled_quantity = 0.7;
  execution.remaining_quantity = 0.3;

  execution.children[1].filled_quantity = 0.3;
  execution.children[1].remaining_quantity = 0.3;
  execution.children[1].notional = 30.3;
  execution.children[1].fee = 0.05;
  execution.children[1].status =
      ChildExecutionStatus::PartiallyFilled;

  const auto replay =
      build_execution_replay(execution);

  EXPECT_EQ(
      replay.events[4].type,
      ReplayEventType::ChildPartiallyFilled
  );

  EXPECT_EQ(
      replay.events.back().type,
      ReplayEventType::ParentIncomplete
  );
}

TEST(ExecutionReplay, EmptyChildrenStillProducesParentEvents) {
  StoredRouteExecution execution;

  execution.execution_id = 7;
  execution.symbol = "ethusd";
  execution.side = "SELL";
  execution.complete = false;
  execution.requested_quantity = 2.0;
  execution.remaining_quantity = 2.0;

  const auto replay =
      build_execution_replay(execution);

  ASSERT_EQ(replay.events.size(), 2U);

  EXPECT_EQ(
      replay.events.front().type,
      ReplayEventType::ParentStarted
  );

  EXPECT_EQ(
      replay.events.back().type,
      ReplayEventType::ParentIncomplete
  );
}

}  // namespace
