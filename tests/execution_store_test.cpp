#include "minimatch/execution_store.hpp"

#include <gtest/gtest.h>

#include <filesystem>

namespace {

using minimatch::ChildExecutionStatus;
using minimatch::ExecutionSimulationConfig;
using minimatch::ExecutionStore;
using minimatch::RouteSide;
using minimatch::RoutedChildExecution;
using minimatch::RoutedExecutionSummary;

std::filesystem::path test_database_path() {
  return std::filesystem::temp_directory_path() /
      "minimatch_execution_store_test.db";
}

void remove_database() {
  const auto path = test_database_path();

  std::filesystem::remove(path);
  std::filesystem::remove(path.string() + "-wal");
  std::filesystem::remove(path.string() + "-shm");
}

RoutedExecutionSummary sample_summary() {
  RoutedExecutionSummary summary;

  summary.complete = true;
  summary.requested_quantity = 1.0;
  summary.filled_quantity = 1.0;
  summary.remaining_quantity = 0.0;
  summary.average_fill_price = 100.6;
  summary.total_notional = 100.6;
  summary.total_fees = 0.1612;
  summary.total_latency_ms = 3.0;

  summary.children = {
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
          0.1212,
          2.0,
          ChildExecutionStatus::Filled
      }
  };

  return summary;
}

TEST(ExecutionStore, CreatesSchema) {
  remove_database();

  {
    ExecutionStore store(
        test_database_path().string()
    );

    EXPECT_EQ(store.route_count(), 0);
    EXPECT_EQ(store.child_count(), 0);
  }

  remove_database();
}

TEST(ExecutionStore, PersistsRouteAndChildren) {
  remove_database();

  {
    ExecutionStore store(
        test_database_path().string()
    );

    const ExecutionSimulationConfig config{
        .fill_ratio = 1.0,
        .rejection_probability = 0.0,
        .base_latency_ms = 1.0,
        .latency_jitter_ms = 0.0,
        .seed = 7
    };

    const std::int64_t execution_id = store.save(
        "btcusd",
        RouteSide::Buy,
        config,
        sample_summary()
    );

    EXPECT_GT(execution_id, 0);
    EXPECT_EQ(store.route_count(), 1);
    EXPECT_EQ(store.child_count(), 2);
  }

  remove_database();
}

TEST(ExecutionStore, PersistsMultipleExecutions) {
  remove_database();

  {
    ExecutionStore store(
        test_database_path().string()
    );

    const ExecutionSimulationConfig config{};

    store.save(
        "btcusd",
        RouteSide::Buy,
        config,
        sample_summary()
    );

    store.save(
        "btcusd",
        RouteSide::Sell,
        config,
        sample_summary()
    );

    EXPECT_EQ(store.route_count(), 2);
    EXPECT_EQ(store.child_count(), 4);
  }

  remove_database();
}


TEST(ExecutionStore, LoadsExecutionById) {
  remove_database();

  {
    ExecutionStore store(
        test_database_path().string()
    );

    const auto id = store.save(
        "btcusd",
        RouteSide::Buy,
        ExecutionSimulationConfig{},
        sample_summary()
    );

    const auto result = store.find(id);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->execution_id, id);
    EXPECT_EQ(result->symbol, "btcusd");
    EXPECT_EQ(result->side, "BUY");
    EXPECT_TRUE(result->complete);
    ASSERT_EQ(result->children.size(), 2U);
    EXPECT_EQ(result->children[0].venue, "coinbase");
  }

  remove_database();
}

TEST(ExecutionStore, ReturnsRecentExecutionsNewestFirst) {
  remove_database();

  {
    ExecutionStore store(
        test_database_path().string()
    );

    const auto first = store.save(
        "btcusd",
        RouteSide::Buy,
        ExecutionSimulationConfig{},
        sample_summary()
    );

    const auto second = store.save(
        "ethusd",
        RouteSide::Sell,
        ExecutionSimulationConfig{},
        sample_summary()
    );

    const auto recent = store.recent(10);

    ASSERT_EQ(recent.size(), 2U);
    EXPECT_EQ(recent[0].execution_id, second);
    EXPECT_EQ(recent[1].execution_id, first);
    EXPECT_EQ(recent[0].symbol, "ethusd");
  }

  remove_database();
}

TEST(ExecutionStore, MissingExecutionReturnsEmpty) {
  remove_database();

  {
    ExecutionStore store(
        test_database_path().string()
    );

    EXPECT_FALSE(store.find(999999).has_value());
  }

  remove_database();
}


}  // namespace
