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

}  // namespace
