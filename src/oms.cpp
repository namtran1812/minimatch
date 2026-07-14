#include "minimatch/oms.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace minimatch {

namespace {

constexpr double epsilon = 1e-12;

bool terminal_status(OrderStatus status) {
  return status == OrderStatus::Filled ||
         status == OrderStatus::Cancelled ||
         status == OrderStatus::Rejected;
}

void validate_positive(
    double value,
    const char* message
) {
  if (!std::isfinite(value) ||
      value <= 0.0) {
    throw std::invalid_argument(message);
  }
}

}  // namespace

std::string to_string(OrderStatus status) {
  switch (status) {
    case OrderStatus::New:
      return "NEW";

    case OrderStatus::Working:
      return "WORKING";

    case OrderStatus::PartiallyFilled:
      return "PARTIALLY_FILLED";

    case OrderStatus::Filled:
      return "FILLED";

    case OrderStatus::Cancelled:
      return "CANCELLED";

    case OrderStatus::Rejected:
      return "REJECTED";
  }

  return "UNKNOWN";
}

ParentOrderId OrderManagementSystem::create_parent(
    const ParentOrderRequest& request,
    std::uint64_t timestamp_ns
) {
  if (request.symbol.empty()) {
    throw std::invalid_argument(
        "parent symbol cannot be empty"
    );
  }

  validate_positive(
      request.quantity,
      "parent quantity must be positive"
  );

  const ParentOrderId id =
      next_parent_id_++;

  parents_.emplace(
      id,
      ParentOrder{
          .id = id,
          .symbol = request.symbol,
          .side = request.side,
          .algorithm = request.algorithm,
          .quantity = request.quantity,
          .filled_quantity = 0.0,
          .remaining_quantity =
              request.quantity,
          .status = OrderStatus::New,
          .created_timestamp_ns =
              timestamp_ns,
          .updated_timestamp_ns =
              timestamp_ns,
          .child_ids = {}
      }
  );

  return id;
}

ChildOrderId OrderManagementSystem::create_child(
    const ChildOrderRequest& request
) {
  const auto parent_iterator =
      parents_.find(request.parent_id);

  if (parent_iterator == parents_.end()) {
    throw std::invalid_argument(
        "parent order does not exist"
    );
  }

  ParentOrder& parent =
      parent_iterator->second;

  if (terminal_status(parent.status)) {
    throw std::logic_error(
        "cannot create child for terminal parent"
    );
  }

  if (request.venue.empty()) {
    throw std::invalid_argument(
        "child venue cannot be empty"
    );
  }

  validate_positive(
      request.price,
      "child price must be positive"
  );

  validate_positive(
      request.quantity,
      "child quantity must be positive"
  );

  if (request.quantity >
      parent.remaining_quantity + epsilon) {
    throw std::invalid_argument(
        "child quantity exceeds parent remainder"
    );
  }

  const ChildOrderId id =
      next_child_id_++;

  children_.emplace(
      id,
      ChildOrder{
          .id = id,
          .parent_id = request.parent_id,
          .venue = request.venue,
          .price = request.price,
          .quantity = request.quantity,
          .filled_quantity = 0.0,
          .remaining_quantity =
              request.quantity,
          .status = OrderStatus::New,
          .created_timestamp_ns =
              request.created_timestamp_ns,
          .updated_timestamp_ns =
              request.created_timestamp_ns
      }
  );

  parent.child_ids.push_back(id);

  if (parent.status == OrderStatus::New) {
    parent.status = OrderStatus::Working;
  }

  parent.updated_timestamp_ns =
      request.created_timestamp_ns;

  return id;
}

bool OrderManagementSystem::mark_child_working(
    ChildOrderId child_id,
    std::uint64_t timestamp_ns
) {
  const auto iterator =
      children_.find(child_id);

  if (iterator == children_.end()) {
    return false;
  }

  ChildOrder& child = iterator->second;

  if (terminal_status(child.status)) {
    return false;
  }

  child.status = child.filled_quantity > epsilon
      ? OrderStatus::PartiallyFilled
      : OrderStatus::Working;

  child.updated_timestamp_ns =
      timestamp_ns;

  refresh_parent(
      child.parent_id,
      timestamp_ns
  );

  return true;
}

OmsExecutionReport
OrderManagementSystem::record_fill(
    ChildOrderId child_id,
    double price,
    double quantity,
    double fee,
    std::uint64_t timestamp_ns
) {
  auto iterator = children_.find(child_id);

  if (iterator == children_.end()) {
    throw std::invalid_argument(
        "child order does not exist"
    );
  }

  ChildOrder& child = iterator->second;

  if (terminal_status(child.status)) {
    throw std::logic_error(
        "cannot fill terminal child order"
    );
  }

  validate_positive(
      price,
      "fill price must be positive"
  );

  validate_positive(
      quantity,
      "fill quantity must be positive"
  );

  if (!std::isfinite(fee) || fee < 0.0) {
    throw std::invalid_argument(
        "fill fee must be non-negative"
    );
  }

  if (quantity >
      child.remaining_quantity + epsilon) {
    throw std::invalid_argument(
        "fill exceeds child remaining quantity"
    );
  }

  child.filled_quantity += quantity;

  child.remaining_quantity =
      std::max(
          0.0,
          child.quantity -
              child.filled_quantity
      );

  child.status =
      child.remaining_quantity <= epsilon
          ? OrderStatus::Filled
          : OrderStatus::PartiallyFilled;

  child.updated_timestamp_ns =
      timestamp_ns;

  const OmsExecutionReport report{
      .execution_report_id =
          next_execution_report_id_++,
      .parent_id = child.parent_id,
      .child_id = child.id,
      .venue = child.venue,
      .price = price,
      .quantity = quantity,
      .notional = price * quantity,
      .fee = fee,
      .timestamp_ns = timestamp_ns
  };

  execution_reports_.push_back(report);

  refresh_parent(
      child.parent_id,
      timestamp_ns
  );

  return report;
}

bool OrderManagementSystem::cancel_child(
    ChildOrderId child_id,
    std::uint64_t timestamp_ns
) {
  const auto iterator =
      children_.find(child_id);

  if (iterator == children_.end()) {
    return false;
  }

  ChildOrder& child = iterator->second;

  if (terminal_status(child.status)) {
    return false;
  }

  child.status = OrderStatus::Cancelled;
  child.updated_timestamp_ns =
      timestamp_ns;

  refresh_parent(
      child.parent_id,
      timestamp_ns
  );

  return true;
}

bool OrderManagementSystem::reject_child(
    ChildOrderId child_id,
    std::uint64_t timestamp_ns
) {
  const auto iterator =
      children_.find(child_id);

  if (iterator == children_.end()) {
    return false;
  }

  ChildOrder& child = iterator->second;

  if (terminal_status(child.status)) {
    return false;
  }

  child.status = OrderStatus::Rejected;
  child.updated_timestamp_ns =
      timestamp_ns;

  refresh_parent(
      child.parent_id,
      timestamp_ns
  );

  return true;
}

bool OrderManagementSystem::cancel_parent(
    ParentOrderId parent_id,
    std::uint64_t timestamp_ns
) {
  const auto iterator =
      parents_.find(parent_id);

  if (iterator == parents_.end()) {
    return false;
  }

  ParentOrder& parent =
      iterator->second;

  if (terminal_status(parent.status)) {
    return false;
  }

  for (ChildOrderId child_id :
       parent.child_ids) {
    auto child_iterator =
        children_.find(child_id);

    if (child_iterator ==
        children_.end()) {
      continue;
    }

    ChildOrder& child =
        child_iterator->second;

    if (!terminal_status(child.status)) {
      child.status =
          OrderStatus::Cancelled;

      child.updated_timestamp_ns =
          timestamp_ns;
    }
  }

  parent.status = OrderStatus::Cancelled;
  parent.updated_timestamp_ns =
      timestamp_ns;

  return true;
}

std::optional<ParentOrder>
OrderManagementSystem::find_parent(
    ParentOrderId parent_id
) const {
  const auto iterator =
      parents_.find(parent_id);

  if (iterator == parents_.end()) {
    return std::nullopt;
  }

  return iterator->second;
}

std::optional<ChildOrder>
OrderManagementSystem::find_child(
    ChildOrderId child_id
) const {
  const auto iterator =
      children_.find(child_id);

  if (iterator == children_.end()) {
    return std::nullopt;
  }

  return iterator->second;
}

std::vector<ChildOrder>
OrderManagementSystem::children_for_parent(
    ParentOrderId parent_id
) const {
  std::vector<ChildOrder> result;

  const auto parent_iterator =
      parents_.find(parent_id);

  if (parent_iterator == parents_.end()) {
    return result;
  }

  result.reserve(
      parent_iterator->second
          .child_ids.size()
  );

  for (ChildOrderId child_id :
       parent_iterator->second.child_ids) {
    const auto child_iterator =
        children_.find(child_id);

    if (child_iterator !=
        children_.end()) {
      result.push_back(
          child_iterator->second
      );
    }
  }

  return result;
}

std::vector<OmsExecutionReport>
OrderManagementSystem::fills_for_parent(
    ParentOrderId parent_id
) const {
  std::vector<OmsExecutionReport> result;

  for (const auto& report :
       execution_reports_) {
    if (report.parent_id == parent_id) {
      result.push_back(report);
    }
  }

  return result;
}

std::vector<ParentOrder>
OrderManagementSystem::parents() const {
  std::vector<ParentOrder> result;

  result.reserve(parents_.size());

  for (const auto& [id, parent] :
       parents_) {
    static_cast<void>(id);
    result.push_back(parent);
  }

  std::sort(
      result.begin(),
      result.end(),
      [](const ParentOrder& left,
         const ParentOrder& right) {
        return left.id < right.id;
      }
  );

  return result;
}

std::size_t
OrderManagementSystem::parent_count() const {
  return parents_.size();
}

std::size_t
OrderManagementSystem::child_count() const {
  return children_.size();
}

std::size_t
OrderManagementSystem::execution_report_count()
    const {
  return execution_reports_.size();
}

void OrderManagementSystem::refresh_parent(
    ParentOrderId parent_id,
    std::uint64_t timestamp_ns
) {
  auto parent_iterator =
      parents_.find(parent_id);

  if (parent_iterator == parents_.end()) {
    return;
  }

  ParentOrder& parent =
      parent_iterator->second;

  if (parent.status ==
      OrderStatus::Cancelled) {
    return;
  }

  double filled = 0.0;
  bool has_working_child = false;
  bool has_rejected_child = false;
  bool has_cancelled_child = false;

  for (ChildOrderId child_id :
       parent.child_ids) {
    const auto child_iterator =
        children_.find(child_id);

    if (child_iterator ==
        children_.end()) {
      continue;
    }

    const ChildOrder& child =
        child_iterator->second;

    filled += child.filled_quantity;

    if (child.status == OrderStatus::New ||
        child.status == OrderStatus::Working ||
        child.status ==
            OrderStatus::PartiallyFilled) {
      has_working_child = true;
    }

    if (child.status ==
        OrderStatus::Rejected) {
      has_rejected_child = true;
    }

    if (child.status ==
        OrderStatus::Cancelled) {
      has_cancelled_child = true;
    }
  }

  parent.filled_quantity =
      std::min(parent.quantity, filled);

  parent.remaining_quantity =
      std::max(
          0.0,
          parent.quantity -
              parent.filled_quantity
      );

  if (parent.remaining_quantity <= epsilon) {
    parent.status = OrderStatus::Filled;
  } else if (parent.filled_quantity > epsilon) {
    parent.status =
        OrderStatus::PartiallyFilled;
  } else if (has_working_child) {
    parent.status = OrderStatus::Working;
  } else if (has_rejected_child &&
             !has_cancelled_child) {
    parent.status = OrderStatus::Rejected;
  } else if (has_cancelled_child) {
    parent.status = OrderStatus::Cancelled;
  } else {
    parent.status = OrderStatus::New;
  }

  parent.updated_timestamp_ns =
      timestamp_ns;
}

}  // namespace minimatch
