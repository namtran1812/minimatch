#include "minimatch/fix_gateway.hpp"

#include <gtest/gtest.h>

namespace {

using minimatch::FixMessage;
using minimatch::FixMessageType;
using minimatch::FixOrderGateway;
using minimatch::OrderManagementSystem;
using minimatch::OrderStatus;

FixMessage new_limit_order(
    const std::string& client_order_id
) {
  FixMessage message;

  message.message_type =
      FixMessageType::NewOrderSingle;

  message.set(11, client_order_id);
  message.set(55, "BTCUSD");
  message.set(54, "1");
  message.set(38, "1.5");
  message.set(40, "2");
  message.set(44, "62640.10");
  message.set(59, "0");

  return message;
}

TEST(FixGateway, CreatesOmsOrdersFromNewOrderSingle) {
  OrderManagementSystem oms;
  FixOrderGateway gateway(oms);

  const auto result =
      gateway.handle(
          new_limit_order("ORDER-1"),
          100
      );

  EXPECT_TRUE(result.accepted);
  EXPECT_GT(result.parent_order_id, 0U);
  EXPECT_GT(result.child_order_id, 0U);

  ASSERT_EQ(oms.parent_count(), 1U);
  ASSERT_EQ(oms.child_count(), 1U);

  const auto parent =
      oms.find_parent(
          result.parent_order_id
      );

  ASSERT_TRUE(parent.has_value());
  EXPECT_EQ(
      parent->status,
      OrderStatus::Working
  );

  EXPECT_EQ(
      result.response.message_type,
      FixMessageType::ExecutionReport
  );

  EXPECT_EQ(
      result.response.get(150),
      std::optional<std::string>("0")
  );
}

TEST(FixGateway, RejectsDuplicateClientOrderId) {
  OrderManagementSystem oms;
  FixOrderGateway gateway(oms);

  EXPECT_TRUE(
      gateway.handle(
          new_limit_order("ORDER-1"),
          100
      ).accepted
  );

  const auto duplicate =
      gateway.handle(
          new_limit_order("ORDER-1"),
          200
      );

  EXPECT_FALSE(duplicate.accepted);

  EXPECT_EQ(
      duplicate.response.message_type,
      FixMessageType::Reject
  );
}

TEST(FixGateway, CancelsExistingOrder) {
  OrderManagementSystem oms;
  FixOrderGateway gateway(oms);

  const auto accepted =
      gateway.handle(
          new_limit_order("ORDER-1"),
          100
      );

  ASSERT_TRUE(accepted.accepted);

  FixMessage cancel;

  cancel.message_type =
      FixMessageType::OrderCancelRequest;

  cancel.set(11, "CANCEL-1");
  cancel.set(41, "ORDER-1");
  cancel.set(55, "BTCUSD");
  cancel.set(54, "1");

  const auto result =
      gateway.handle(cancel, 200);

  EXPECT_TRUE(result.accepted);

  EXPECT_EQ(
      result.response.message_type,
      FixMessageType::ExecutionReport
  );

  EXPECT_EQ(
      result.response.get(150),
      std::optional<std::string>("4")
  );

  const auto parent =
      oms.find_parent(
          accepted.parent_order_id
      );

  ASSERT_TRUE(parent.has_value());

  EXPECT_EQ(
      parent->status,
      OrderStatus::Cancelled
  );
}

TEST(FixGateway, RejectsUnknownCancelOrder) {
  OrderManagementSystem oms;
  FixOrderGateway gateway(oms);

  FixMessage cancel;

  cancel.message_type =
      FixMessageType::OrderCancelRequest;

  cancel.set(11, "CANCEL-1");
  cancel.set(41, "UNKNOWN");
  cancel.set(55, "BTCUSD");
  cancel.set(54, "1");

  const auto result =
      gateway.handle(cancel, 200);

  EXPECT_FALSE(result.accepted);

  EXPECT_EQ(
      result.response.message_type,
      FixMessageType::Reject
  );
}

TEST(FixGateway, RejectsMissingRequiredFields) {
  OrderManagementSystem oms;
  FixOrderGateway gateway(oms);

  FixMessage message;
  message.message_type =
      FixMessageType::NewOrderSingle;

  message.set(11, "ORDER-1");

  const auto result =
      gateway.handle(message, 100);

  EXPECT_FALSE(result.accepted);

  EXPECT_EQ(
      result.response.message_type,
      FixMessageType::Reject
  );
}

TEST(FixGateway, SupportsMarketOrders) {
  OrderManagementSystem oms;
  FixOrderGateway gateway(oms);

  FixMessage message;

  message.message_type =
      FixMessageType::NewOrderSingle;

  message.set(11, "MARKET-1");
  message.set(55, "BTCUSD");
  message.set(54, "2");
  message.set(38, "2.0");
  message.set(40, "1");

  const auto result =
      gateway.handle(message, 100);

  EXPECT_TRUE(result.accepted);
  EXPECT_EQ(oms.parent_count(), 1U);
  EXPECT_EQ(oms.child_count(), 1U);
}

}  // namespace
