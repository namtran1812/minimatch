#pragma once

#include "minimatch/live_algo_executor.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace minimatch {

struct PersistedAlgoOrder;

enum class AlgoOrderStatus {
  Running,
  Completed,
  Cancelled,
  Failed
};

struct AlgoOrderState {
  ParentOrderId parent_order_id{0};

  AlgoOrderStatus status{
      AlgoOrderStatus::Running
  };

  std::string symbol;

  ExecutionAlgorithm algorithm{
      ExecutionAlgorithm::Market
  };

  std::size_t current_slice{0};
  std::size_t total_slices{0};

  std::uint64_t started_at_ns{0};
  std::uint64_t next_slice_at_ns{0};
  std::uint64_t completed_at_ns{0};

  double requested_quantity{0.0};
  double filled_quantity{0.0};
  double remaining_quantity{0.0};

  double arrival_price{0.0};

  bool recovered{false};

  std::size_t recovery_count{0};

  std::uint64_t last_recovered_at_ns{0};

  std::string error;
};

class AlgoScheduler {
 public:
  using TimestampProvider =
      std::function<std::uint64_t()>;

  using StateCallback =
      std::function<void(
          const AlgoOrderState&
      )>;

  AlgoScheduler(
      OrderManagementSystem& oms,
      LiveQuoteProvider quote_provider,
      TimestampProvider timestamp_provider
  );

  ~AlgoScheduler();

  AlgoScheduler(
      const AlgoScheduler&
  ) = delete;

  AlgoScheduler& operator=(
      const AlgoScheduler&
  ) = delete;

  ParentOrderId submit(
      const LiveAlgoRequest& request
  );

  void restore_history(
      const std::vector<
          AlgoOrderState
      >& states
  );

  void restore_recoverable(
      const std::vector<
          PersistedAlgoOrder
      >& orders
  );

  void set_state_callback(
      StateCallback callback
  );

  bool cancel(
      ParentOrderId parent_order_id
  );

  [[nodiscard]]
  std::optional<AlgoOrderState>
  find(
      ParentOrderId parent_order_id
  ) const;

  [[nodiscard]]
  std::vector<AlgoOrderState>
  orders() const;

 private:
  struct Job {
    AlgoOrderState state;
    LiveAlgoRequest request;

    std::atomic<bool>
        cancel_requested{false};

    std::thread worker;
  };

  void run_job(
      const std::shared_ptr<Job>& job
  );

  OrderManagementSystem& oms_;

  LiveQuoteProvider
      quote_provider_;

  TimestampProvider
      timestamp_provider_;

  mutable std::mutex mutex_;

  StateCallback state_callback_;

  std::unordered_map<
      ParentOrderId,
      std::shared_ptr<Job>
  > jobs_;
};

std::string to_string(
    AlgoOrderStatus status
);

}  // namespace minimatch
