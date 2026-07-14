#pragma once

#include "minimatch/execution_store.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace minimatch {

enum class ReplayEventType {
  ParentStarted,
  ChildSubmitted,
  ChildFilled,
  ChildPartiallyFilled,
  ChildRejected,
  ParentCompleted,
  ParentIncomplete
};

struct ReplayEvent {
  std::size_t sequence{0};
  ReplayEventType type{
      ReplayEventType::ParentStarted
  };

  std::int64_t execution_id{0};

  std::string venue;
  std::size_t level_index{0};

  double timestamp_ms{0.0};
  double requested_quantity{0.0};
  double filled_quantity{0.0};
  double remaining_quantity{0.0};
  double price{0.0};
  double fee{0.0};

  std::string description;
};

struct ExecutionReplay {
  std::int64_t execution_id{0};
  std::string symbol;
  std::string side;

  bool complete{false};

  double requested_quantity{0.0};
  double filled_quantity{0.0};
  double remaining_quantity{0.0};
  double total_fees{0.0};
  double total_latency_ms{0.0};

  std::vector<ReplayEvent> events;
};

ExecutionReplay build_execution_replay(
    const StoredRouteExecution& execution
);

std::string to_string(ReplayEventType type);

}  // namespace minimatch
