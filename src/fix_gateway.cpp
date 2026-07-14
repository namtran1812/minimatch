#include "minimatch/fix_gateway.hpp"

#include <cmath>
#include <stdexcept>
#include <string>

namespace minimatch {

namespace {

std::string required_field(
    const FixMessage& message,
    int tag,
    const char* name
) {
  const auto value = message.get(tag);

  if (!value.has_value() || value->empty()) {
    throw std::invalid_argument(
        std::string("missing required FIX field: ") +
        name
    );
  }

  return *value;
}

double required_double(
    const FixMessage& message,
    int tag,
    const char* name
) {
  const std::string value =
      required_field(message, tag, name);

  std::size_t consumed = 0;
  const double parsed =
      std::stod(value, &consumed);

  if (consumed != value.size() ||
      !std::isfinite(parsed) ||
      parsed <= 0.0) {
    throw std::invalid_argument(
        std::string("invalid FIX field: ") +
        name
    );
  }

  return parsed;
}

RouteSide parse_side(
    const std::string& side
) {
  if (side == "1") {
    return RouteSide::Buy;
  }

  if (side == "2") {
    return RouteSide::Sell;
  }

  throw std::invalid_argument(
      "unsupported FIX Side"
  );
}

}  // namespace

FixOrderGateway::FixOrderGateway(
    OrderManagementSystem& oms
)
    : oms_(oms) {}

FixGatewayResult FixOrderGateway::handle(
    const FixMessage& message,
    std::uint64_t timestamp_ns
) {
  try {
    if (message.message_type ==
        FixMessageType::NewOrderSingle) {
      return handle_new_order(
          message,
          timestamp_ns
      );
    }

    if (message.message_type ==
        FixMessageType::OrderCancelRequest) {
      return handle_cancel(
          message,
          timestamp_ns
      );
    }

    return FixGatewayResult{
        .accepted = false,
        .message =
            "unsupported FIX application message",
        .response = reject_message(
            "unsupported FIX application message"
        )
    };
  } catch (const std::exception& error) {
    return FixGatewayResult{
        .accepted = false,
        .message = error.what(),
        .response = reject_message(
            error.what()
        )
    };
  }
}

std::optional<ParentOrderId>
FixOrderGateway::parent_id_for_client_order(
    const std::string& client_order_id
) const {
  const auto iterator =
      parent_by_client_order_id_.find(
          client_order_id
      );

  if (iterator ==
      parent_by_client_order_id_.end()) {
    return std::nullopt;
  }

  return iterator->second;
}

std::optional<ChildOrderId>
FixOrderGateway::child_id_for_client_order(
    const std::string& client_order_id
) const {
  const auto iterator =
      child_by_client_order_id_.find(
          client_order_id
      );

  if (iterator ==
      child_by_client_order_id_.end()) {
    return std::nullopt;
  }

  return iterator->second;
}

FixGatewayResult FixOrderGateway::handle_new_order(
    const FixMessage& message,
    std::uint64_t timestamp_ns
) {
  const std::string client_order_id =
      required_field(message, 11, "ClOrdID");

  if (parent_by_client_order_id_.contains(
          client_order_id
      )) {
    throw std::invalid_argument(
        "duplicate ClOrdID"
    );
  }

  const std::string symbol =
      required_field(message, 55, "Symbol");

  const RouteSide side =
      parse_side(
          required_field(message, 54, "Side")
      );

  const double quantity =
      required_double(message, 38, "OrderQty");

  const std::string order_type =
      required_field(message, 40, "OrdType");

  double price = 0.0;

  if (order_type == "2") {
    price = required_double(
        message,
        44,
        "Price"
    );
  } else if (order_type == "1") {
    price = 1.0;
  } else {
    throw std::invalid_argument(
        "only market and limit FIX orders are supported"
    );
  }

  const ParentOrderId parent_id =
      oms_.create_parent(
          ParentOrderRequest{
              .symbol = symbol,
              .side = side,
              .quantity = quantity,
              .algorithm =
                  ExecutionAlgorithm::Market
          },
          timestamp_ns
      );

  const ChildOrderId child_id =
      oms_.create_child(
          ChildOrderRequest{
              .parent_id = parent_id,
              .venue = "MINIMATCH",
              .price = price,
              .quantity = quantity,
              .created_timestamp_ns =
                  timestamp_ns
          }
      );

  oms_.mark_child_working(
      child_id,
      timestamp_ns
  );

  parent_by_client_order_id_[
      client_order_id
  ] = parent_id;

  child_by_client_order_id_[
      client_order_id
  ] = child_id;

  return FixGatewayResult{
      .accepted = true,
      .message = "order accepted",
      .response = execution_report(
          client_order_id,
          "ACK-" +
              std::to_string(child_id),
          "0",
          "0",
          quantity,
          0.0,
          quantity,
          price
      ),
      .parent_order_id = parent_id,
      .child_order_id = child_id
  };
}

FixGatewayResult FixOrderGateway::handle_cancel(
    const FixMessage& message,
    std::uint64_t timestamp_ns
) {
  const std::string cancel_order_id =
      required_field(message, 11, "ClOrdID");

  const std::string original_order_id =
      required_field(message, 41, "OrigClOrdID");

  const auto child_id =
      child_id_for_client_order(
          original_order_id
      );

  const auto parent_id =
      parent_id_for_client_order(
          original_order_id
      );

  if (!child_id.has_value() ||
      !parent_id.has_value()) {
    throw std::invalid_argument(
        "unknown OrigClOrdID"
    );
  }

  if (!oms_.cancel_child(
          *child_id,
          timestamp_ns
      )) {
    throw std::logic_error(
        "order cannot be cancelled"
    );
  }

  oms_.cancel_parent(
      *parent_id,
      timestamp_ns
  );

  const auto parent =
      oms_.find_parent(*parent_id);

  if (!parent.has_value()) {
    throw std::logic_error(
        "parent order disappeared"
    );
  }

  parent_by_client_order_id_[
      cancel_order_id
  ] = *parent_id;

  child_by_client_order_id_[
      cancel_order_id
  ] = *child_id;

  return FixGatewayResult{
      .accepted = true,
      .message = "order cancelled",
      .response = execution_report(
          cancel_order_id,
          "CXL-" +
              std::to_string(*child_id),
          "4",
          "4",
          parent->quantity,
          parent->filled_quantity,
          parent->remaining_quantity,
          0.0
      ),
      .parent_order_id = *parent_id,
      .child_order_id = *child_id
  };
}

FixMessage FixOrderGateway::execution_report(
    const std::string& client_order_id,
    const std::string& execution_id,
    const std::string& execution_type,
    const std::string& order_status,
    double order_quantity,
    double cumulative_quantity,
    double leaves_quantity,
    double price,
    const std::string& text
) const {
  FixMessage report;
  report.message_type =
      FixMessageType::ExecutionReport;

  report.set(11, client_order_id);
  report.set(17, execution_id);
  report.set(150, execution_type);
  report.set(39, order_status);
  report.set(38, std::to_string(order_quantity));
  report.set(14, std::to_string(cumulative_quantity));
  report.set(151, std::to_string(leaves_quantity));

  if (price > 0.0) {
    report.set(44, std::to_string(price));
  }

  if (!text.empty()) {
    report.set(58, text);
  }

  return report;
}

FixMessage FixOrderGateway::reject_message(
    const std::string& text
) const {
  FixMessage reject;
  reject.message_type = FixMessageType::Reject;
  reject.set(58, text);
  return reject;
}

}  // namespace minimatch
