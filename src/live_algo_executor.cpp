#include "minimatch/live_algo_executor.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace minimatch {

namespace {

constexpr double epsilon =
    1e-12;

}

LiveAlgoResult execute_live_algorithm(
    const LiveAlgoRequest& request,
    OrderManagementSystem& oms,
    const LiveQuoteProvider&
        quote_provider,
    std::uint64_t timestamp_ns
) {
  if (request.symbol.empty()) {
    throw std::invalid_argument(
        "live algo symbol cannot be empty"
    );
  }

  if (!std::isfinite(
          request.quantity
      ) ||
      request.quantity <= 0.0) {
    throw std::invalid_argument(
        "live algo quantity must be positive"
    );
  }

  ExecutionScheduleRequest schedule_request =
      request.schedule;

  schedule_request.quantity =
      request.quantity;

  const auto schedule =
      build_execution_schedule(
          schedule_request
      );

  if (schedule.empty()) {
    throw std::runtime_error(
        "execution schedule is empty"
    );
  }

  LiveAlgoResult result;

  result.requested_quantity =
      request.quantity;

  result.remaining_quantity =
      request.quantity;

  const ParentOrderId parent_id =
      oms.create_parent(
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
          timestamp_ns
      );

  result.parent_order_id =
      parent_id;

  for (
      std::size_t slice_index = 0;
      slice_index < schedule.size();
      ++slice_index
  ) {
    if (
        result.remaining_quantity <=
        epsilon
    ) {
      break;
    }

    const auto& slice =
        schedule[slice_index];

    const double requested_quantity =
        std::min(
            slice.quantity,
            result.remaining_quantity
        );

    if (
        requested_quantity <=
        epsilon
    ) {
      continue;
    }

    const auto quotes =
        quote_provider(
            request.symbol
        );

    LiveAlgoSliceResult slice_result;

    slice_result.slice_index =
        slice_index;

    slice_result.requested_quantity =
        requested_quantity;

    if (quotes.empty()) {
      result.slices.push_back(
          slice_result
      );

      continue;
    }

    const RouteRequest route_request{
        .side =
            request.side,
        .quantity =
            requested_quantity,
        .limit_price =
            request.limit_price,
        .max_slippage_bps =
            request.max_slippage_bps,
        .max_venue_count =
            request.max_venue_count,
        .all_or_none =
            request.all_or_none
    };

    const RoutePlan plan =
        build_route_plan(
            route_request,
            quotes
        );

    slice_result.routed_quantity =
        plan.routed_quantity;

    if (plan.legs.empty()) {
      result.slices.push_back(
          slice_result
      );

      continue;
    }

    const auto execution =
        simulate_route_execution(
            plan,
            request.simulation
        );

    for (
        const auto& routed_child :
        execution.children
    ) {
      const ChildOrderId child_id =
          oms.create_child(
              ChildOrderRequest{
                  .parent_id =
                      parent_id,
                  .venue =
                      routed_child.venue,
                  .price =
                      routed_child.price,
                  .quantity =
                      routed_child
                          .requested_quantity,
                  .created_timestamp_ns =
                      timestamp_ns
              }
          );

      ++slice_result.child_count;

      if (
          routed_child.status ==
          ChildExecutionStatus::
              Rejected
      ) {
        oms.reject_child(
            child_id,
            timestamp_ns
        );

        ++result
              .rejected_child_count;

        continue;
      }

      oms.mark_child_working(
          child_id,
          timestamp_ns
      );

      ++result
            .accepted_child_count;

      if (
          routed_child
              .filled_quantity >
          epsilon
      ) {
        oms.record_fill(
            child_id,
            routed_child.price,
            routed_child
                .filled_quantity,
            routed_child.fee,
            timestamp_ns
        );

        result.filled_quantity +=
            routed_child
                .filled_quantity;

        result.total_notional +=
            routed_child.notional;

        result.total_fees +=
            routed_child.fee;

        slice_result
            .filled_quantity +=
            routed_child
                .filled_quantity;

        slice_result.notional +=
            routed_child.notional;

        slice_result.fees +=
            routed_child.fee;
      }

      if (
          routed_child
              .remaining_quantity >
          epsilon
      ) {
        oms.cancel_child(
            child_id,
            timestamp_ns
        );
      }
    }

    result.remaining_quantity =
        std::max(
            0.0,
            request.quantity -
                result.filled_quantity
        );

    result.slices.push_back(
        slice_result
    );
  }

  result.complete =
      result.remaining_quantity <=
      epsilon;

  if (!result.complete) {
    oms.cancel_parent(
        parent_id,
        timestamp_ns
    );
  }

  if (
      result.filled_quantity >
      epsilon
  ) {
    result.average_fill_price =
        result.total_notional /
        result.filled_quantity;
  }

  return result;
}

}  // namespace minimatch
