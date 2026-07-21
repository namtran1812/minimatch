#include "minimatch/algo_scheduler.hpp"
#include "minimatch/algo_store.hpp"
#include "minimatch/oms.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

namespace {

using namespace minimatch;

TEST(
    AlgoRecovery,
    ResumesFromPersistedCurrentSlice
) {
  OrderManagementSystem oms;

  const auto quote_provider =
      [](
          const std::string&
      ) {
        return std::vector<
            VenueQuote
        >{};
      };

  std::uint64_t now =
      1'000'000'000ULL;

  AlgoScheduler scheduler(
      oms,
      quote_provider,
      [&]() {
        return now;
      }
  );

  PersistedAlgoOrder persisted;

  persisted.has_request =
      true;

  persisted.state.parent_order_id =
      42;

  persisted.state.status =
      AlgoOrderStatus::Running;

  persisted.state.symbol =
      "btcusd";

  persisted.state.algorithm =
      ExecutionAlgorithm::TWAP;

  persisted.state.current_slice =
      2;

  persisted.state.total_slices =
      4;

  persisted.state.requested_quantity =
      4.0;

  persisted.state.filled_quantity =
      2.0;

  persisted.state.remaining_quantity =
      2.0;

  persisted.request.symbol =
      "btcusd";

  persisted.request.quantity =
      4.0;

  persisted.request.schedule.algorithm =
      ExecutionAlgorithm::TWAP;

  persisted.request.schedule.quantity =
      4.0;

  persisted.request.schedule.slices =
      4;

  persisted.request.schedule.duration_seconds =
      0.0;

  scheduler.restore_recoverable(
      {persisted}
  );

  std::this_thread::sleep_for(
      std::chrono::milliseconds(
          50
      )
  );

  const auto recovered =
      scheduler.find(
          42
      );

  ASSERT_TRUE(
      recovered.has_value()
  );

  EXPECT_GE(
      recovered->current_slice,
      2U
  );
}

TEST(
    AlgoRecovery,
    MarksMissingRequestAsFailed
) {
  OrderManagementSystem oms;

  const auto quote_provider =
      [](
          const std::string&
      ) {
        return std::vector<
            VenueQuote
        >{};
      };

  AlgoScheduler scheduler(
      oms,
      quote_provider,
      []() {
        return std::uint64_t{
            1'000'000'000ULL
        };
      }
  );

  PersistedAlgoOrder persisted;

  persisted.has_request =
      false;

  persisted.state.parent_order_id =
      43;

  persisted.state.status =
      AlgoOrderStatus::Running;

  scheduler.restore_recoverable(
      {persisted}
  );

  const auto recovered =
      scheduler.find(
          43
      );

  ASSERT_TRUE(
      recovered.has_value()
  );

  EXPECT_EQ(
      recovered->status,
      AlgoOrderStatus::Failed
  );

  EXPECT_EQ(
      recovered->error,
      "missing persisted algo request"
  );
}

}  // namespace

TEST(
    AlgoRecovery,
    PreservesFutureNextSliceDeadline
) {
  OrderManagementSystem oms;

  const auto quote_provider =
      [](
          const std::string&
      ) {
        return std::vector<
            VenueQuote
        >{};
      };

  std::uint64_t now =
      1'000'000'000ULL;

  AlgoScheduler scheduler(
      oms,
      quote_provider,
      [&]() {
        return now;
      }
  );

  PersistedAlgoOrder persisted;

  persisted.has_request =
      true;

  persisted.state.parent_order_id =
      44;

  persisted.state.status =
      AlgoOrderStatus::Running;

  persisted.state.symbol =
      "btcusd";

  persisted.state.algorithm =
      ExecutionAlgorithm::TWAP;

  persisted.state.current_slice =
      1;

  persisted.state.total_slices =
      2;

  persisted.state.requested_quantity =
      2.0;

  persisted.state.filled_quantity =
      1.0;

  persisted.state.remaining_quantity =
      1.0;

  persisted.state.next_slice_at_ns =
      now +
      5'000'000'000ULL;

  persisted.request.symbol =
      "btcusd";

  persisted.request.quantity =
      2.0;

  persisted.request.schedule.algorithm =
      ExecutionAlgorithm::TWAP;

  persisted.request.schedule.quantity =
      2.0;

  persisted.request.schedule.slices =
      2;

  persisted.request.schedule.duration_seconds =
      10.0;

  scheduler.restore_recoverable(
      {persisted}
  );

  std::this_thread::sleep_for(
      std::chrono::milliseconds(
          50
      )
  );

  const auto recovered =
      scheduler.find(
          44
      );

  ASSERT_TRUE(
      recovered.has_value()
  );

  // Worker should still be waiting for the
  // persisted future deadline.
  EXPECT_EQ(
      recovered->current_slice,
      1U
  );

  EXPECT_EQ(
      recovered->next_slice_at_ns,
      persisted.state.next_slice_at_ns
  );
}



TEST(
    AlgoRecovery,
    ExecutesImmediatelyWhenPersistedDeadlineExpired
) {
  OrderManagementSystem oms;

  const auto quote_provider =
      [](
          const std::string&
      ) {
        return std::vector<
            VenueQuote
        >{};
      };

  std::uint64_t now =
      10'000'000'000ULL;

  AlgoScheduler scheduler(
      oms,
      quote_provider,
      [&]() {
        return now;
      }
  );

  PersistedAlgoOrder persisted;

  persisted.has_request =
      true;

  persisted.state.parent_order_id =
      45;

  persisted.state.status =
      AlgoOrderStatus::Running;

  persisted.state.symbol =
      "btcusd";

  persisted.state.algorithm =
      ExecutionAlgorithm::TWAP;

  persisted.state.current_slice =
      1;

  persisted.state.total_slices =
      2;

  persisted.state.requested_quantity =
      2.0;

  persisted.state.filled_quantity =
      1.0;

  persisted.state.remaining_quantity =
      1.0;

  // Deadline already passed before recovery.
  persisted.state.next_slice_at_ns =
      now -
      1'000'000'000ULL;

  persisted.request.symbol =
      "btcusd";

  persisted.request.quantity =
      2.0;

  persisted.request.schedule.algorithm =
      ExecutionAlgorithm::TWAP;

  persisted.request.schedule.quantity =
      2.0;

  persisted.request.schedule.slices =
      2;

  persisted.request.schedule.duration_seconds =
      10.0;

  scheduler.restore_recoverable(
      {persisted}
  );

  std::this_thread::sleep_for(
      std::chrono::milliseconds(
          100
      )
  );

  const auto recovered =
      scheduler.find(
          45
      );

  ASSERT_TRUE(
      recovered.has_value()
  );

  // It should not remain blocked waiting for
  // a newly-created full TWAP delay.
  EXPECT_NE(
      recovered->next_slice_at_ns,
      persisted.state.next_slice_at_ns
  );
}
