#include "minimatch/execution_replay.hpp"

#include "minimatch/execution_engine.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace minimatch {

std::string to_string(ReplayEventType type) {
  switch (type) {
    case ReplayEventType::ParentStarted:
      return "PARENT_STARTED";

    case ReplayEventType::ChildSubmitted:
      return "CHILD_SUBMITTED";

    case ReplayEventType::ChildFilled:
      return "CHILD_FILLED";

    case ReplayEventType::ChildPartiallyFilled:
      return "CHILD_PARTIALLY_FILLED";

    case ReplayEventType::ChildRejected:
      return "CHILD_REJECTED";

    case ReplayEventType::ParentCompleted:
      return "PARENT_COMPLETED";

    case ReplayEventType::ParentIncomplete:
      return "PARENT_INCOMPLETE";
  }

  return "UNKNOWN";
}

namespace {

ReplayEventType child_event_type(
    ChildExecutionStatus status
) {
  switch (status) {
    case ChildExecutionStatus::Filled:
      return ReplayEventType::ChildFilled;

    case ChildExecutionStatus::PartiallyFilled:
      return ReplayEventType::ChildPartiallyFilled;

    case ChildExecutionStatus::Rejected:
      return ReplayEventType::ChildRejected;
  }

  return ReplayEventType::ChildRejected;
}

std::string child_description(
    const RoutedChildExecution& child
) {
  std::ostringstream out;

  out << to_string(child.status)
      << " child on "
      << child.venue
      << " at "
      << child.price
      << " for "
      << child.filled_quantity
      << " of "
      << child.requested_quantity;

  return out.str();
}

}  // namespace

ExecutionReplay build_execution_replay(
    const StoredRouteExecution& execution
) {
  ExecutionReplay replay;

  replay.execution_id = execution.execution_id;
  replay.symbol = execution.symbol;
  replay.side = execution.side;
  replay.complete = execution.complete;

  replay.requested_quantity =
      execution.requested_quantity;

  replay.filled_quantity =
      execution.filled_quantity;

  replay.remaining_quantity =
      execution.remaining_quantity;

  replay.total_fees =
      execution.total_fees;

  replay.total_latency_ms =
      execution.total_latency_ms;

  std::size_t sequence = 0;

  replay.events.push_back(
      ReplayEvent{
          .sequence = sequence++,
          .type = ReplayEventType::ParentStarted,
          .execution_id = execution.execution_id,
          .timestamp_ms = 0.0,
          .requested_quantity =
              execution.requested_quantity,
          .filled_quantity = 0.0,
          .remaining_quantity =
              execution.requested_quantity,
          .description =
              "Parent execution started"
      }
  );

  double elapsed_ms = 0.0;
  double cumulative_filled = 0.0;

  for (const auto& child : execution.children) {
    replay.events.push_back(
        ReplayEvent{
            .sequence = sequence++,
            .type =
                ReplayEventType::ChildSubmitted,
            .execution_id =
                execution.execution_id,
            .venue = child.venue,
            .level_index =
                child.level_index,
            .timestamp_ms = elapsed_ms,
            .requested_quantity =
                child.requested_quantity,
            .filled_quantity = 0.0,
            .remaining_quantity =
                child.requested_quantity,
            .price = child.price,
            .fee = 0.0,
            .description =
                "Child order submitted to " +
                child.venue
        }
    );

    elapsed_ms += child.latency_ms;
    cumulative_filled += child.filled_quantity;

    replay.events.push_back(
        ReplayEvent{
            .sequence = sequence++,
            .type =
                child_event_type(child.status),
            .execution_id =
                execution.execution_id,
            .venue = child.venue,
            .level_index =
                child.level_index,
            .timestamp_ms = elapsed_ms,
            .requested_quantity =
                child.requested_quantity,
            .filled_quantity =
                child.filled_quantity,
            .remaining_quantity =
                child.remaining_quantity,
            .price = child.price,
            .fee = child.fee,
            .description =
                child_description(child)
        }
    );
  }

  replay.events.push_back(
      ReplayEvent{
          .sequence = sequence,
          .type = execution.complete
              ? ReplayEventType::ParentCompleted
              : ReplayEventType::ParentIncomplete,
          .execution_id = execution.execution_id,
          .timestamp_ms = elapsed_ms,
          .requested_quantity =
              execution.requested_quantity,
          .filled_quantity =
              cumulative_filled,
          .remaining_quantity =
              std::max(
                  0.0,
                  execution.requested_quantity -
                      cumulative_filled
              ),
          .description = execution.complete
              ? "Parent execution completed"
              : "Parent execution incomplete"
      }
  );

  return replay;
}

}  // namespace minimatch
