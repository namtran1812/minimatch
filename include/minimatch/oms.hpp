#pragma once

#include "minimatch/execution_algo.hpp"
#include "minimatch/router.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace minimatch {

using ParentOrderId = std::uint64_t;
using ChildOrderId = std::uint64_t;

enum class OrderStatus {
  New,
  Working,
  PartiallyFilled,
  Filled,
  Cancelled,
  Rejected
};

struct ParentOrderRequest {
  std::string symbol;
  RouteSide side{RouteSide::Buy};
  double quantity{0.0};

  ExecutionAlgorithm algorithm{
      ExecutionAlgorithm::Market
  };
};

struct ParentOrder {
  ParentOrderId id{0};

  std::string symbol;
  RouteSide side{RouteSide::Buy};
  ExecutionAlgorithm algorithm{
      ExecutionAlgorithm::Market
  };

  double quantity{0.0};
  double filled_quantity{0.0};
  double remaining_quantity{0.0};

  OrderStatus status{OrderStatus::New};

  std::uint64_t created_timestamp_ns{0};
  std::uint64_t updated_timestamp_ns{0};

  std::vector<ChildOrderId> child_ids;
};

struct ChildOrderRequest {
  ParentOrderId parent_id{0};

  std::string venue;

  double price{0.0};
  double quantity{0.0};

  std::uint64_t created_timestamp_ns{0};
};

struct ChildOrder {
  ChildOrderId id{0};
  ParentOrderId parent_id{0};

  std::string venue;

  double price{0.0};
  double quantity{0.0};
  double filled_quantity{0.0};
  double remaining_quantity{0.0};

  OrderStatus status{OrderStatus::New};

  std::uint64_t created_timestamp_ns{0};
  std::uint64_t updated_timestamp_ns{0};
};

struct OmsExecutionReport {
  std::uint64_t execution_report_id{0};

  ParentOrderId parent_id{0};
  ChildOrderId child_id{0};

  std::string venue;

  double price{0.0};
  double quantity{0.0};
  double notional{0.0};
  double fee{0.0};

  std::uint64_t timestamp_ns{0};
};

class OrderManagementSystem {
 public:
  ParentOrderId create_parent(
      const ParentOrderRequest& request,
      std::uint64_t timestamp_ns
  );

  ChildOrderId create_child(
      const ChildOrderRequest& request
  );

  OmsExecutionReport record_fill(
      ChildOrderId child_id,
      double price,
      double quantity,
      double fee,
      std::uint64_t timestamp_ns
  );

  bool mark_child_working(
      ChildOrderId child_id,
      std::uint64_t timestamp_ns
  );

  bool cancel_child(
      ChildOrderId child_id,
      std::uint64_t timestamp_ns
  );

  bool reject_child(
      ChildOrderId child_id,
      std::uint64_t timestamp_ns
  );

  bool cancel_parent(
      ParentOrderId parent_id,
      std::uint64_t timestamp_ns
  );

  [[nodiscard]] std::optional<ParentOrder>
  find_parent(ParentOrderId parent_id) const;

  [[nodiscard]] std::optional<ChildOrder>
  find_child(ChildOrderId child_id) const;

  [[nodiscard]] std::vector<ChildOrder>
  children_for_parent(
      ParentOrderId parent_id
  ) const;

  [[nodiscard]] std::vector<OmsExecutionReport>
  fills_for_parent(
      ParentOrderId parent_id
  ) const;

  [[nodiscard]] std::vector<ParentOrder>
  parents() const;

  [[nodiscard]] std::size_t parent_count() const;
  [[nodiscard]] std::size_t child_count() const;
  [[nodiscard]] std::size_t execution_report_count() const;

 private:
  ParentOrderId next_parent_id_{1};
  ChildOrderId next_child_id_{1};
  std::uint64_t next_execution_report_id_{1};

  std::unordered_map<
      ParentOrderId,
      ParentOrder
  > parents_;

  std::unordered_map<
      ChildOrderId,
      ChildOrder
  > children_;

  std::vector<OmsExecutionReport>
      execution_reports_;

  void refresh_parent(
      ParentOrderId parent_id,
      std::uint64_t timestamp_ns
  );
};

std::string to_string(OrderStatus status);

}  // namespace minimatch
