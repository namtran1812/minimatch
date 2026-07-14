#include "minimatch/execution_algo.hpp"

#include <gtest/gtest.h>

using namespace minimatch;

TEST(ExecutionAlgo, TWAPSchedule)
{
    auto schedule =
        build_execution_schedule(
            ExecutionAlgorithm::TWAP,
            100,
            5,
            50);

    ASSERT_EQ(schedule.size(),5);

    EXPECT_DOUBLE_EQ(schedule[0].quantity,20);
    EXPECT_DOUBLE_EQ(schedule[4].quantity,20);

    EXPECT_DOUBLE_EQ(schedule[0].delay_seconds,0);
    EXPECT_DOUBLE_EQ(schedule[4].delay_seconds,40);
}

TEST(ExecutionAlgo, EmptySchedule)
{
    auto schedule =
        build_execution_schedule(
            ExecutionAlgorithm::TWAP,
            100,
            0,
            50);

    EXPECT_TRUE(schedule.empty());
}
