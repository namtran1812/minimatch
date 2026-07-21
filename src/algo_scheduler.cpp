#include "minimatch/algo_scheduler.hpp"
#include "minimatch/algo_store.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace minimatch {

namespace {

constexpr double epsilon =
    1e-12;

}

std::string to_string(
    AlgoOrderStatus status
) {
  switch (status) {
    case AlgoOrderStatus::Running:
      return "RUNNING";

    case AlgoOrderStatus::Completed:
      return "COMPLETED";

    case AlgoOrderStatus::Cancelled:
      return "CANCELLED";

    case AlgoOrderStatus::Failed:
      return "FAILED";
  }

  return "UNKNOWN";
}

AlgoScheduler::AlgoScheduler(
    OrderManagementSystem& oms,
    LiveQuoteProvider quote_provider,
    TimestampProvider timestamp_provider
)
    : oms_(oms),
      quote_provider_(
          std::move(
              quote_provider
          )
      ),
      timestamp_provider_(
          std::move(
              timestamp_provider
          )
      ) {
}

AlgoScheduler::~AlgoScheduler() {
  std::vector<
      std::shared_ptr<Job>
  > jobs;

  {
    std::lock_guard lock(
        mutex_
    );

    for (
        const auto& [id, job] :
        jobs_
    ) {
      (void)id;

      job->cancel_requested.store(
          true
      );

      jobs.push_back(job);
    }
  }

  for (const auto& job : jobs) {
    if (job->worker.joinable()) {
      job->worker.join();
    }
  }
}

ParentOrderId AlgoScheduler::submit(
    const LiveAlgoRequest& request
) {
  if (request.symbol.empty()) {
    throw std::invalid_argument(
        "algo symbol cannot be empty"
    );
  }

  if (
      !std::isfinite(
          request.quantity
      ) ||
      request.quantity <= 0.0
  ) {
    throw std::invalid_argument(
        "algo quantity must be positive"
    );
  }

  auto schedule_request =
      request.schedule;

  schedule_request.quantity =
      request.quantity;

  const auto schedule =
      build_execution_schedule(
          schedule_request
      );

  if (schedule.empty()) {
    throw std::invalid_argument(
        "algo schedule cannot be empty"
    );
  }

  const auto now =
      timestamp_provider_();

  double arrival_price = 0.0;

  {
    const auto quotes =
        quote_provider_(
            request.symbol
        );

    double best_bid = 0.0;
    double best_ask = 0.0;

    for (const auto& quote : quotes) {
      if (
          !quote.healthy ||
          quote.bids.empty() ||
          quote.asks.empty()
      ) {
        continue;
      }

      const double bid =
          quote.bids.front().price;

      const double ask =
          quote.asks.front().price;

      if (
          best_bid == 0.0 ||
          bid > best_bid
      ) {
        best_bid = bid;
      }

      if (
          best_ask == 0.0 ||
          ask < best_ask
      ) {
        best_ask = ask;
      }
    }

    if (
        best_bid > 0.0 &&
        best_ask > 0.0
    ) {
      arrival_price =
          (
              best_bid +
              best_ask
          ) /
          2.0;
    }
  }

  const auto parent_id =
      oms_.create_parent(
          ParentOrderRequest{
              .symbol =
                  request.symbol,
              .side =
                  request.side,
              .quantity =
                  request.quantity,
              .algorithm =
                  schedule_request.algorithm
          },
          now
      );

  auto job =
      std::make_shared<Job>();

  job->request =
      request;

  job->state.parent_order_id =
      parent_id;

  job->state.status =
      AlgoOrderStatus::Running;

  job->state.symbol =
      request.symbol;

  job->state.algorithm =
      schedule_request.algorithm;

  job->state.total_slices =
      schedule.size();

  job->state.started_at_ns =
      now;

  job->state.requested_quantity =
      request.quantity;

  job->state.remaining_quantity =
      request.quantity;

  job->state.arrival_price =
      arrival_price;

  {
    std::lock_guard lock(
        mutex_
    );

    jobs_.emplace(
        parent_id,
        job
    );

    if (state_callback_) {
      state_callback_(
          job->state
      );
    }
  }

  job->worker =
      std::thread(
          [this, job]() {
            run_job(job);
          }
      );

  return parent_id;
}

void AlgoScheduler::run_job(
    const std::shared_ptr<Job>& job
) {
  try {
    auto schedule_request =
        job->request.schedule;

    schedule_request.quantity =
        job->request.quantity;

    const auto schedule =
        build_execution_schedule(
            schedule_request
        );

    std::size_t start_index = 0;

    {
      std::lock_guard lock(
          mutex_
      );

      start_index =
          std::min(
              job->state.current_slice,
              schedule.size()
          );
    }

    for (
        std::size_t index =
            start_index;
        index < schedule.size();
        ++index
    ) {
      if (
          job->cancel_requested.load()
      ) {
        const auto now =
            timestamp_provider_();

        oms_.cancel_parent(
            job->state.parent_order_id,
            now
        );

        std::lock_guard lock(
            mutex_
        );

        job->state.status =
            AlgoOrderStatus::Cancelled;

        job->state.completed_at_ns =
            now;

        return;
      }

      const auto& slice =
          schedule[index];

      if (
          slice.delay_seconds >
          0.0
      ) {
        const auto now_ns =
            timestamp_provider_();

        std::uint64_t target_ns =
            0;

        {
          std::lock_guard lock(
              mutex_
          );

          if (
              job->state.next_slice_at_ns >
              now_ns
          ) {
            target_ns =
                job->state.next_slice_at_ns;

          } else if (
              job->state.next_slice_at_ns !=
              0
          ) {
            // Persisted deadline already passed while
            // the process was down. Execute immediately.
            target_ns =
                now_ns;

          } else {
            // Fresh slice with no persisted deadline.
            target_ns =
                now_ns +
                static_cast<
                    std::uint64_t
                >(
                    slice.delay_seconds *
                    1'000'000'000.0
                );

            job->state.next_slice_at_ns =
                target_ns;

            if (state_callback_) {
              state_callback_(
                  job->state
              );
            }
          }
        }

        const auto remaining_ns =
            target_ns > now_ns
                ? target_ns - now_ns
                : 0;

        const auto delay =
            std::chrono::nanoseconds(
                remaining_ns
            );

        const auto deadline =
            std::chrono::
                steady_clock::now() +
            delay;

        while (
            std::chrono::
                steady_clock::now() <
            deadline
        ) {
          if (
              job
                  ->cancel_requested
                  .load()
          ) {
            break;
          }

          std::this_thread::
              sleep_for(
                  std::chrono::
                      milliseconds(
                          20
                      )
              );
        }

        if (
            job
                ->cancel_requested
                .load()
        ) {
          continue;
        }
      }

      double remaining = 0.0;

      {
        std::lock_guard lock(
            mutex_
        );

        remaining =
            job->state
                .remaining_quantity;

        job->state.next_slice_at_ns =
            0;
      }

      if (remaining <= epsilon) {
        break;
      }

      const double quantity =
          std::min(
              slice.quantity,
              remaining
          );

      if (quantity <= epsilon) {
        continue;
      }

      const auto quotes =
          quote_provider_(
              job->request.symbol
          );

      if (quotes.empty()) {
        continue;
      }

      const RoutePlan plan =
          build_route_plan(
              RouteRequest{
                  .side =
                      job->request.side,
                  .quantity =
                      quantity,
                  .limit_price =
                      job->request
                          .limit_price,
                  .max_slippage_bps =
                      job->request
                          .max_slippage_bps,
                  .max_venue_count =
                      job->request
                          .max_venue_count,
                  .all_or_none =
                      job->request
                          .all_or_none
              },
              quotes
          );

      if (plan.legs.empty()) {
        continue;
      }

      const auto execution =
          simulate_route_execution(
              plan,
              job->request.simulation
          );

      double slice_filled = 0.0;

      for (
          const auto& routed_child :
          execution.children
      ) {
        const auto now =
            timestamp_provider_();

        const auto child_id =
            oms_.create_child(
                ChildOrderRequest{
                    .parent_id =
                        job->state
                            .parent_order_id,
                    .venue =
                        routed_child
                            .venue,
                    .price =
                        routed_child
                            .price,
                    .quantity =
                        routed_child
                            .requested_quantity,
                    .created_timestamp_ns =
                        now
                }
            );

        if (
            routed_child.status ==
            ChildExecutionStatus::
                Rejected
        ) {
          oms_.reject_child(
              child_id,
              now
          );

          continue;
        }

        oms_.mark_child_working(
            child_id,
            now
        );

        if (
            routed_child
                .filled_quantity >
            epsilon
        ) {
          oms_.record_fill(
              child_id,
              routed_child.price,
              routed_child
                  .filled_quantity,
              routed_child.fee,
              now
          );

          slice_filled +=
              routed_child
                  .filled_quantity;
        }

        if (
            routed_child
                .remaining_quantity >
            epsilon
        ) {
          oms_.cancel_child(
              child_id,
              now
          );
        }
      }

      {
        std::lock_guard lock(
            mutex_
        );

        job->state
            .filled_quantity +=
            slice_filled;

        job->state
            .remaining_quantity =
            std::max(
                0.0,
                job->state
                    .requested_quantity -
                job->state
                    .filled_quantity
            );

        job->state.current_slice =
            index + 1;

        job->state.next_slice_at_ns =
            0;

        if (state_callback_) {
          state_callback_(
              job->state
          );
        }
      }
    }

    const auto now =
        timestamp_provider_();

    std::lock_guard lock(
        mutex_
    );

    if (
        job->state
            .remaining_quantity <=
        epsilon
    ) {
      job->state.status =
          AlgoOrderStatus::Completed;
    } else {
      oms_.cancel_parent(
          job->state.parent_order_id,
          now
      );

      job->state.status =
          job->cancel_requested.load()
              ? AlgoOrderStatus::
                    Cancelled
              : AlgoOrderStatus::
                    Completed;
    }

    job->state.completed_at_ns =
        now;

    job->state.next_slice_at_ns =
        0;

    if (state_callback_) {
      state_callback_(
          job->state
      );
    }

  } catch (
      const std::exception& ex
  ) {
    const auto now =
        timestamp_provider_();

    try {
      oms_.cancel_parent(
          job->state.parent_order_id,
          now
      );
    } catch (...) {
    }

    std::lock_guard lock(
        mutex_
    );

    job->state.status =
        AlgoOrderStatus::Failed;

    job->state.error =
        ex.what();

    job->state.completed_at_ns =
        now;

    if (state_callback_) {
      state_callback_(
          job->state
      );
    }
  }
}

bool AlgoScheduler::cancel(
    ParentOrderId parent_order_id
) {
  std::lock_guard lock(
      mutex_
  );

  const auto iterator =
      jobs_.find(
          parent_order_id
      );

  if (iterator == jobs_.end()) {
    return false;
  }

  if (
      iterator->second->state.status !=
      AlgoOrderStatus::Running
  ) {
    return false;
  }

  iterator
      ->second
      ->cancel_requested
      .store(true);

  return true;
}

std::optional<AlgoOrderState>
AlgoScheduler::find(
    ParentOrderId parent_order_id
) const {
  std::lock_guard lock(
      mutex_
  );

  const auto iterator =
      jobs_.find(
          parent_order_id
      );

  if (iterator == jobs_.end()) {
    return std::nullopt;
  }

  return iterator->second->state;
}

std::vector<AlgoOrderState>
AlgoScheduler::orders() const {
  std::lock_guard lock(
      mutex_
  );

  std::vector<AlgoOrderState>
      result;

  result.reserve(
      jobs_.size()
  );

  for (
      const auto& [id, job] :
      jobs_
  ) {
    (void)id;

    result.push_back(
        job->state
    );
  }

  std::sort(
      result.begin(),
      result.end(),
      [](
          const AlgoOrderState& lhs,
          const AlgoOrderState& rhs
      ) {
        return lhs.parent_order_id <
            rhs.parent_order_id;
      }
  );

  return result;
}

}  // namespace minimatch

namespace minimatch {

void AlgoScheduler::set_state_callback(
    StateCallback callback
) {
  std::lock_guard lock(
      mutex_
  );

  state_callback_ =
      std::move(callback);
}

void AlgoScheduler::restore_recoverable(
    const std::vector<
        PersistedAlgoOrder
    >& orders
) {
  std::vector<
      std::shared_ptr<Job>
  > jobs_to_resume;

  for (
      const auto& persisted :
      orders
  ) {
    auto job =
        std::make_shared<Job>();

    job->state =
        persisted.state;

    job->state.recovered =
        false;

    if (
        persisted.state.status ==
        AlgoOrderStatus::Running
    ) {
      if (!persisted.has_request) {
        job->state.status =
            AlgoOrderStatus::Failed;

        job->state.error =
            "missing persisted algo request";

        job->state.next_slice_at_ns =
            0;

        if (state_callback_) {
          state_callback_(
              job->state
          );
        }
      } else {
        job->request =
            persisted.request;

        job->state.error.clear();

        job->state.recovered =
            true;

        ++job->state.recovery_count;

        job->state.last_recovered_at_ns =
            timestamp_provider_();

        if (state_callback_) {
          state_callback_(
              job->state
          );
        }

        jobs_to_resume.push_back(
            job
        );
      }
    }

    {
      std::lock_guard lock(
          mutex_
      );

      jobs_[
          job->state.parent_order_id
      ] = job;
    }
  }

  for (
      const auto& job :
      jobs_to_resume
  ) {
    job->worker =
        std::thread(
            [this, job]() {
              run_job(
                  job
              );
            }
        );
  }
}


void AlgoScheduler::restore_history(
    const std::vector<
        AlgoOrderState
    >& states
) {
  std::lock_guard lock(
      mutex_
  );

  for (const auto& state : states) {
    // Never restart previously-running jobs blindly.
    // Mark them failed after process recovery.
    AlgoOrderState recovered =
        state;

    if (
        recovered.status ==
        AlgoOrderStatus::Running
    ) {
      recovered.status =
          AlgoOrderStatus::Failed;

      recovered.error =
          "interrupted by process restart";

      recovered.next_slice_at_ns =
          0;
    }

    auto job =
        std::make_shared<Job>();

    job->state =
        recovered;

    jobs_[
        recovered.parent_order_id
    ] = job;
  }
}

}  // namespace minimatch
