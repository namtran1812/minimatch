#pragma once

#include "minimatch/fix.hpp"
#include "minimatch/oms.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

namespace minimatch {

struct FixGatewayResult {
  bool accepted{false};
  std::string message;
  FixMessage response;

  ParentOrderId parent_order_id{0};
  ChildOrderId child_order_id{0};
};

class FixOrderGateway {
 public:
  explicit FixOrderGateway(
      OrderManagementSystem& oms
  );

  FixGatewayResult handle(
      const FixMessage& message,
      std::uint64_t timestamp_ns
  );

  [[nodiscard]] std::optional<ParentOrderId>
  parent_id_for_client_order(
      const std::string& client_order_id
  ) const;

  [[nodiscard]] std::optional<ChildOrderId>
  child_id_for_client_order(
      const std::string& client_order_id
  ) const;

 private:
  OrderManagementSystem& oms_;

  std::unordered_map<
      std::string,
      ParentOrderId
  > parent_by_client_order_id_;

  std::unordered_map<
      std::string,
      ChildOrderId
  > child_by_client_order_id_;

  FixGatewayResult handle_new_order(
      const FixMessage& message,
      std::uint64_t timestamp_ns
  );

  FixGatewayResult handle_cancel(
      const FixMessage& message,
      std::uint64_t timestamp_ns
  );

  FixMessage execution_report(
      const std::string& client_order_id,
      const std::string& execution_id,
      const std::string& execution_type,
      const std::string& order_status,
      double order_quantity,
      double cumulative_quantity,
      double leaves_quantity,
      double price,
      const std::string& text = ""
  ) const;

  FixMessage reject_message(
      const std::string& text
  ) const;
};

}  // namespace minimatch
