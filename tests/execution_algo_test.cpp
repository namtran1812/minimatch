#include "minimatch/execution_algo.hpp"

#include <gtest/gtest.h>

#include <numeric>
#include <vector>

namespace {

using minimatch::ExecutionAlgorithm;
using minimatch::ExecutionScheduleRequest;
using minimatch::Slice;
using minimatch::build_execution_schedule;

double total_quantity(
    const std::vector<Slice>& schedule
) {
  return std::accumulate(
      schedule.begin(),
      schedule.end(),
      0.0,
      [](double total, const Slice& slice) {
        return total + slice.quantity;
      }
  );
}

TEST(ExecutionAlgo, MarketUsesSingleImmediateSlice) {
  const auto schedule = build_execution_schedule(
      ExecutionScheduleRequest{
          .algorithm = ExecutionAlgorithm::Market,
          .quantity = 100.0
      }
  );

  ASSERT_EQ(schedule.size(), 1U);
  EXPECT_DOUBLE_EQ(schedule[0].quantity, 100.0);
  EXPECT_DOUBLE_EQ(schedule[0].delay_seconds, 0.0);
}

TEST(ExecutionAlgo, TWAPUsesEqualTimeSlices) {
  const auto schedule = build_execution_schedule(
      ExecutionScheduleRequest{
          .algorithm = ExecutionAlgorithm::TWAP,
          .quantity = 100.0,
          .slices = 5,
          .duration_seconds = 50.0
      }
  );

  ASSERT_EQ(schedule.size(), 5U);

  EXPECT_DOUBLE_EQ(schedule[0].quantity, 20.0);
  EXPECT_DOUBLE_EQ(schedule[4].quantity, 20.0);

  EXPECT_DOUBLE_EQ(schedule[0].delay_seconds, 0.0);
  EXPECT_DOUBLE_EQ(schedule[4].delay_seconds, 40.0);

  EXPECT_DOUBLE_EQ(total_quantity(schedule), 100.0);
}

TEST(ExecutionAlgo, VWAPFollowsVolumeProfile) {
  const auto schedule = build_execution_schedule(
      ExecutionScheduleRequest{
          .algorithm = ExecutionAlgorithm::VWAP,
          .quantity = 100.0,
          .slices = 4,
          .duration_seconds = 40.0,
          .volume_profile = {
              1.0,
              2.0,
              4.0,
              3.0
          }
      }
  );

  ASSERT_EQ(schedule.size(), 4U);

  EXPECT_DOUBLE_EQ(schedule[0].quantity, 10.0);
  EXPECT_DOUBLE_EQ(schedule[1].quantity, 20.0);
  EXPECT_DOUBLE_EQ(schedule[2].quantity, 40.0);
  EXPECT_DOUBLE_EQ(schedule[3].quantity, 30.0);

  EXPECT_DOUBLE_EQ(total_quantity(schedule), 100.0);
}

TEST(ExecutionAlgo, POVRespectsParticipationRate) {
  const auto schedule = build_execution_schedule(
      ExecutionScheduleRequest{
          .algorithm = ExecutionAlgorithm::POV,
          .quantity = 100.0,
          .slices = 3,
          .duration_seconds = 30.0,
          .volume_profile = {
              100.0,
              200.0,
              300.0
          },
          .participation_rate = 0.10
      }
  );

  ASSERT_EQ(schedule.size(), 3U);

  EXPECT_DOUBLE_EQ(schedule[0].quantity, 10.0);
  EXPECT_DOUBLE_EQ(schedule[1].quantity, 20.0);
  EXPECT_DOUBLE_EQ(schedule[2].quantity, 30.0);

  EXPECT_DOUBLE_EQ(total_quantity(schedule), 60.0);
}

TEST(ExecutionAlgo, POVStopsWhenParentQuantityIsFilled) {
  const auto schedule = build_execution_schedule(
      ExecutionScheduleRequest{
          .algorithm = ExecutionAlgorithm::POV,
          .quantity = 25.0,
          .slices = 3,
          .duration_seconds = 30.0,
          .volume_profile = {
              100.0,
              200.0,
              300.0
          },
          .participation_rate = 0.10
      }
  );

  ASSERT_EQ(schedule.size(), 2U);

  EXPECT_DOUBLE_EQ(schedule[0].quantity, 10.0);
  EXPECT_DOUBLE_EQ(schedule[1].quantity, 15.0);
  EXPECT_DOUBLE_EQ(total_quantity(schedule), 25.0);
}

TEST(ExecutionAlgo, IcebergUsesDisplayedQuantity) {
  const auto schedule = build_execution_schedule(
      ExecutionScheduleRequest{
          .algorithm = ExecutionAlgorithm::Iceberg,
          .quantity = 60.0,
          .slices = 1,
          .duration_seconds = 30.0,
          .displayed_quantity = 25.0
      }
  );

  ASSERT_EQ(schedule.size(), 3U);

  EXPECT_DOUBLE_EQ(schedule[0].quantity, 25.0);
  EXPECT_DOUBLE_EQ(schedule[1].quantity, 25.0);
  EXPECT_DOUBLE_EQ(schedule[2].quantity, 10.0);

  EXPECT_DOUBLE_EQ(total_quantity(schedule), 60.0);
}

TEST(ExecutionAlgo, BackwardCompatibleTWAPOverload) {
  const auto schedule = build_execution_schedule(
      ExecutionAlgorithm::TWAP,
      100.0,
      5,
      50.0
  );

  ASSERT_EQ(schedule.size(), 5U);
  EXPECT_DOUBLE_EQ(total_quantity(schedule), 100.0);
}

TEST(ExecutionAlgo, RejectsInvalidCommonSettings) {
  EXPECT_THROW(
      build_execution_schedule(
          ExecutionScheduleRequest{
              .algorithm = ExecutionAlgorithm::TWAP,
              .quantity = 0.0,
              .slices = 5,
              .duration_seconds = 10.0
          }
      ),
      std::invalid_argument
  );

  EXPECT_THROW(
      build_execution_schedule(
          ExecutionScheduleRequest{
              .algorithm = ExecutionAlgorithm::TWAP,
              .quantity = 10.0,
              .slices = 0,
              .duration_seconds = 10.0
          }
      ),
      std::invalid_argument
  );

  EXPECT_THROW(
      build_execution_schedule(
          ExecutionScheduleRequest{
              .algorithm = ExecutionAlgorithm::TWAP,
              .quantity = 10.0,
              .slices = 2,
              .duration_seconds = -1.0
          }
      ),
      std::invalid_argument
  );
}

TEST(ExecutionAlgo, RejectsInvalidAlgorithmSettings) {
  EXPECT_THROW(
      build_execution_schedule(
          ExecutionScheduleRequest{
              .algorithm = ExecutionAlgorithm::VWAP,
              .quantity = 10.0,
              .slices = 2,
              .duration_seconds = 10.0,
              .volume_profile = {1.0}
          }
      ),
      std::invalid_argument
  );

  EXPECT_THROW(
      build_execution_schedule(
          ExecutionScheduleRequest{
              .algorithm = ExecutionAlgorithm::POV,
              .quantity = 10.0,
              .slices = 2,
              .duration_seconds = 10.0,
              .volume_profile = {100.0, 100.0},
              .participation_rate = 1.5
          }
      ),
      std::invalid_argument
  );

  EXPECT_THROW(
      build_execution_schedule(
          ExecutionScheduleRequest{
              .algorithm = ExecutionAlgorithm::Iceberg,
              .quantity = 10.0,
              .displayed_quantity = 0.0
          }
      ),
      std::invalid_argument
  );
}

}  // namespace
