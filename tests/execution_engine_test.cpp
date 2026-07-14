#include "minimatch/execution_engine.hpp"

#include <gtest/gtest.h>

namespace {

using minimatch::ChildExecutionStatus;
using minimatch::ExecutionSimulationConfig;
using minimatch::RouteLeg;
using minimatch::RoutePlan;
using minimatch::simulate_route_execution;

RoutePlan sample_route() {
  RoutePlan plan;

  plan.complete = true;
  plan.requested_quantity = 1.0;
  plan.routed_quantity = 1.0;

  plan.legs = {
      RouteLeg{
          "coinbase",
          100.0,
          0.4,
          0.04,
          100.1,
          1.0,
          10.0,
          0.0,
          0
      },
      RouteLeg{
          "kraken",
          101.0,
          0.6,
          0.1212,
          101.2,
          2.0,
          20.0,
          0.0,
          1
      }
  };

  return plan;
}

TEST(RoutedExecutionEngine, FullyExecutesRoute) {
  const auto summary =
      simulate_route_execution(sample_route());

  EXPECT_TRUE(summary.complete);
  EXPECT_DOUBLE_EQ(summary.filled_quantity, 1.0);
  EXPECT_DOUBLE_EQ(summary.remaining_quantity, 0.0);
  ASSERT_EQ(summary.children.size(), 2U);

  EXPECT_EQ(
      summary.children[0].status,
      ChildExecutionStatus::Filled
  );

  EXPECT_EQ(
      summary.children[1].status,
      ChildExecutionStatus::Filled
  );
}

TEST(RoutedExecutionEngine, SupportsPartialFills) {
  const auto summary = simulate_route_execution(
      sample_route(),
      ExecutionSimulationConfig{
          .fill_ratio = 0.5
      }
  );

  EXPECT_FALSE(summary.complete);
  EXPECT_DOUBLE_EQ(summary.filled_quantity, 0.5);
  EXPECT_DOUBLE_EQ(summary.remaining_quantity, 0.5);

  for (const auto& child : summary.children) {
    EXPECT_EQ(
        child.status,
        ChildExecutionStatus::PartiallyFilled
    );
  }
}

TEST(RoutedExecutionEngine, ComputesAverageFillPrice) {
  const auto summary =
      simulate_route_execution(sample_route());

  const double expected_price =
      (100.0 * 0.4 + 101.0 * 0.6) / 1.0;

  EXPECT_DOUBLE_EQ(
      summary.average_fill_price,
      expected_price
  );
}

TEST(RoutedExecutionEngine, ComputesFees) {
  const auto summary =
      simulate_route_execution(sample_route());

  const double expected_fees =
      (100.0 * 0.4 * 10.0 / 10000.0) +
      (101.0 * 0.6 * 20.0 / 10000.0);

  EXPECT_DOUBLE_EQ(
      summary.total_fees,
      expected_fees
  );
}

TEST(RoutedExecutionEngine, RejectsInvalidFillRatio) {
  EXPECT_THROW(
      simulate_route_execution(
          sample_route(),
          ExecutionSimulationConfig{
              .fill_ratio = 1.5
          }
      ),
      std::invalid_argument
  );
}


TEST(RoutedExecutionEngine, RejectsAllChildren) {
  const auto summary = simulate_route_execution(
      sample_route(),
      ExecutionSimulationConfig{
          .fill_ratio = 1.0,
          .rejection_probability = 1.0,
          .base_latency_ms = 2.0,
          .latency_jitter_ms = 1.0,
          .seed = 7
      }
  );

  EXPECT_FALSE(summary.complete);
  EXPECT_DOUBLE_EQ(summary.filled_quantity, 0.0);
  EXPECT_DOUBLE_EQ(summary.remaining_quantity, 1.0);

  ASSERT_EQ(summary.children.size(), 2U);

  for (const auto& child : summary.children) {
    EXPECT_EQ(
        child.status,
        ChildExecutionStatus::Rejected
    );

    EXPECT_DOUBLE_EQ(child.filled_quantity, 0.0);
    EXPECT_DOUBLE_EQ(
        child.remaining_quantity,
        child.requested_quantity
    );

    EXPECT_GE(child.latency_ms, 2.0);
  }
}

TEST(RoutedExecutionEngine, SameSeedProducesSameResults) {
  const ExecutionSimulationConfig config{
      .fill_ratio = 0.75,
      .rejection_probability = 0.4,
      .base_latency_ms = 2.0,
      .latency_jitter_ms = 3.0,
      .seed = 12345
  };

  const auto first =
      simulate_route_execution(sample_route(), config);

  const auto second =
      simulate_route_execution(sample_route(), config);

  ASSERT_EQ(first.children.size(), second.children.size());

  EXPECT_DOUBLE_EQ(
      first.filled_quantity,
      second.filled_quantity
  );

  EXPECT_DOUBLE_EQ(
      first.total_latency_ms,
      second.total_latency_ms
  );

  for (std::size_t index = 0;
       index < first.children.size();
       ++index) {
    EXPECT_EQ(
        first.children[index].status,
        second.children[index].status
    );

    EXPECT_DOUBLE_EQ(
        first.children[index].latency_ms,
        second.children[index].latency_ms
    );

    EXPECT_DOUBLE_EQ(
        first.children[index].filled_quantity,
        second.children[index].filled_quantity
    );
  }
}

TEST(RoutedExecutionEngine, AddsConfiguredLatency) {
  const auto summary = simulate_route_execution(
      sample_route(),
      ExecutionSimulationConfig{
          .fill_ratio = 1.0,
          .rejection_probability = 0.0,
          .base_latency_ms = 5.0,
          .latency_jitter_ms = 0.0,
          .seed = 1
      }
  );

  ASSERT_EQ(summary.children.size(), 2U);

  EXPECT_DOUBLE_EQ(
      summary.children[0].latency_ms,
      6.0
  );

  EXPECT_DOUBLE_EQ(
      summary.children[1].latency_ms,
      7.0
  );

  EXPECT_DOUBLE_EQ(
      summary.total_latency_ms,
      13.0
  );
}

TEST(RoutedExecutionEngine, RejectsInvalidSimulationSettings) {
  EXPECT_THROW(
      simulate_route_execution(
          sample_route(),
          ExecutionSimulationConfig{
              .rejection_probability = 1.5
          }
      ),
      std::invalid_argument
  );

  EXPECT_THROW(
      simulate_route_execution(
          sample_route(),
          ExecutionSimulationConfig{
              .base_latency_ms = -1.0
          }
      ),
      std::invalid_argument
  );

  EXPECT_THROW(
      simulate_route_execution(
          sample_route(),
          ExecutionSimulationConfig{
              .latency_jitter_ms = -1.0
          }
      ),
      std::invalid_argument
  );
}


}  // namespace
