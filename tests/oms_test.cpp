#include "minimatch/oms.hpp"

#include <gtest/gtest.h>

namespace {

using minimatch::ChildOrderRequest;
using minimatch::ExecutionAlgorithm;
using minimatch::OrderManagementSystem;
using minimatch::OrderStatus;
using minimatch::ParentOrderRequest;
using minimatch::RouteSide;

TEST(OMS, CreatesParentOrder) {
  OrderManagementSystem oms;

  const auto parent_id =
      oms.create_parent(
          ParentOrderRequest{
              .symbol = "btcusd",
              .side = RouteSide::Buy,
              .quantity = 1.0,
              .algorithm =
                  ExecutionAlgorithm::TWAP
          },
          100
      );

  const auto parent =
      oms.find_parent(parent_id);

  ASSERT_TRUE(parent.has_value());

  EXPECT_EQ(parent->symbol, "btcusd");
  EXPECT_EQ(parent->status, OrderStatus::New);
  EXPECT_DOUBLE_EQ(parent->quantity, 1.0);
  EXPECT_DOUBLE_EQ(
      parent->remaining_quantity,
      1.0
  );
}

TEST(OMS, CreatesChildAndMovesParentToWorking) {
  OrderManagementSystem oms;

  const auto parent_id =
      oms.create_parent(
          ParentOrderRequest{
              .symbol = "btcusd",
              .side = RouteSide::Buy,
              .quantity = 1.0,
              .algorithm =
                  ExecutionAlgorithm::TWAP
          },
          100
      );

  const auto child_id =
      oms.create_child(
          ChildOrderRequest{
              .parent_id = parent_id,
              .venue = "coinbase",
              .price = 100.0,
              .quantity = 0.4,
              .created_timestamp_ns = 110
          }
      );

  const auto child =
      oms.find_child(child_id);

  ASSERT_TRUE(child.has_value());
  EXPECT_EQ(child->status, OrderStatus::New);

  const auto parent =
      oms.find_parent(parent_id);

  ASSERT_TRUE(parent.has_value());
  EXPECT_EQ(
      parent->status,
      OrderStatus::Working
  );
  ASSERT_EQ(parent->child_ids.size(), 1U);
}

TEST(OMS, RecordsPartialChildFill) {
  OrderManagementSystem oms;

  const auto parent_id =
      oms.create_parent(
          ParentOrderRequest{
              .symbol = "btcusd",
              .side = RouteSide::Buy,
              .quantity = 1.0,
              .algorithm =
                  ExecutionAlgorithm::TWAP
          },
          100
      );

  const auto child_id =
      oms.create_child(
          ChildOrderRequest{
              .parent_id = parent_id,
              .venue = "coinbase",
              .price = 100.0,
              .quantity = 1.0,
              .created_timestamp_ns = 110
          }
      );

  const auto report =
      oms.record_fill(
          child_id,
          100.0,
          0.4,
          0.04,
          120
      );

  EXPECT_EQ(report.parent_id, parent_id);
  EXPECT_EQ(report.child_id, child_id);
  EXPECT_DOUBLE_EQ(report.notional, 40.0);

  const auto child =
      oms.find_child(child_id);

  ASSERT_TRUE(child.has_value());

  EXPECT_EQ(
      child->status,
      OrderStatus::PartiallyFilled
  );

  EXPECT_DOUBLE_EQ(
      child->filled_quantity,
      0.4
  );

  EXPECT_DOUBLE_EQ(
      child->remaining_quantity,
      0.6
  );

  const auto parent =
      oms.find_parent(parent_id);

  ASSERT_TRUE(parent.has_value());

  EXPECT_EQ(
      parent->status,
      OrderStatus::PartiallyFilled
  );

  EXPECT_DOUBLE_EQ(
      parent->filled_quantity,
      0.4
  );

  EXPECT_DOUBLE_EQ(
      parent->remaining_quantity,
      0.6
  );
}

TEST(OMS, CompletesParentAcrossMultipleChildren) {
  OrderManagementSystem oms;

  const auto parent_id =
      oms.create_parent(
          ParentOrderRequest{
              .symbol = "btcusd",
              .side = RouteSide::Buy,
              .quantity = 1.0,
              .algorithm =
                  ExecutionAlgorithm::VWAP
          },
          100
      );

  const auto first =
      oms.create_child(
          ChildOrderRequest{
              .parent_id = parent_id,
              .venue = "coinbase",
              .price = 100.0,
              .quantity = 0.4,
              .created_timestamp_ns = 110
          }
      );

  const auto second =
      oms.create_child(
          ChildOrderRequest{
              .parent_id = parent_id,
              .venue = "kraken",
              .price = 100.1,
              .quantity = 0.6,
              .created_timestamp_ns = 120
          }
      );

  oms.record_fill(
      first,
      100.0,
      0.4,
      0.04,
      130
  );

  oms.record_fill(
      second,
      100.1,
      0.6,
      0.06,
      140
  );

  const auto parent =
      oms.find_parent(parent_id);

  ASSERT_TRUE(parent.has_value());

  EXPECT_EQ(
      parent->status,
      OrderStatus::Filled
  );

  EXPECT_DOUBLE_EQ(
      parent->filled_quantity,
      1.0
  );

  EXPECT_DOUBLE_EQ(
      parent->remaining_quantity,
      0.0
  );

  EXPECT_EQ(
      oms.execution_report_count(),
      2U
  );
}

TEST(OMS, RejectsOverfill) {
  OrderManagementSystem oms;

  const auto parent_id =
      oms.create_parent(
          ParentOrderRequest{
              .symbol = "btcusd",
              .side = RouteSide::Buy,
              .quantity = 1.0
          },
          100
      );

  const auto child_id =
      oms.create_child(
          ChildOrderRequest{
              .parent_id = parent_id,
              .venue = "coinbase",
              .price = 100.0,
              .quantity = 0.5,
              .created_timestamp_ns = 110
          }
      );

  EXPECT_THROW(
      oms.record_fill(
          child_id,
          100.0,
          0.6,
          0.0,
          120
      ),
      std::invalid_argument
  );
}

TEST(OMS, CancelsParentAndWorkingChildren) {
  OrderManagementSystem oms;

  const auto parent_id =
      oms.create_parent(
          ParentOrderRequest{
              .symbol = "btcusd",
              .side = RouteSide::Sell,
              .quantity = 1.0,
              .algorithm =
                  ExecutionAlgorithm::Iceberg
          },
          100
      );

  const auto first =
      oms.create_child(
          ChildOrderRequest{
              .parent_id = parent_id,
              .venue = "coinbase",
              .price = 100.0,
              .quantity = 0.5,
              .created_timestamp_ns = 110
          }
      );

  const auto second =
      oms.create_child(
          ChildOrderRequest{
              .parent_id = parent_id,
              .venue = "kraken",
              .price = 99.9,
              .quantity = 0.5,
              .created_timestamp_ns = 120
          }
      );

  EXPECT_TRUE(
      oms.cancel_parent(parent_id, 130)
  );

  EXPECT_EQ(
      oms.find_parent(parent_id)->status,
      OrderStatus::Cancelled
  );

  EXPECT_EQ(
      oms.find_child(first)->status,
      OrderStatus::Cancelled
  );

  EXPECT_EQ(
      oms.find_child(second)->status,
      OrderStatus::Cancelled
  );
}

TEST(OMS, RejectsChildAndUpdatesParent) {
  OrderManagementSystem oms;

  const auto parent_id =
      oms.create_parent(
          ParentOrderRequest{
              .symbol = "btcusd",
              .side = RouteSide::Buy,
              .quantity = 1.0
          },
          100
      );

  const auto child_id =
      oms.create_child(
          ChildOrderRequest{
              .parent_id = parent_id,
              .venue = "coinbase",
              .price = 100.0,
              .quantity = 1.0,
              .created_timestamp_ns = 110
          }
      );

  EXPECT_TRUE(
      oms.reject_child(child_id, 120)
  );

  EXPECT_EQ(
      oms.find_child(child_id)->status,
      OrderStatus::Rejected
  );

  EXPECT_EQ(
      oms.find_parent(parent_id)->status,
      OrderStatus::Rejected
  );
}

TEST(OMS, ReturnsChildrenAndFillsForParent) {
  OrderManagementSystem oms;

  const auto parent_id =
      oms.create_parent(
          ParentOrderRequest{
              .symbol = "btcusd",
              .side = RouteSide::Buy,
              .quantity = 1.0
          },
          100
      );

  const auto child_id =
      oms.create_child(
          ChildOrderRequest{
              .parent_id = parent_id,
              .venue = "coinbase",
              .price = 100.0,
              .quantity = 1.0,
              .created_timestamp_ns = 110
          }
      );

  oms.record_fill(
      child_id,
      100.0,
      0.5,
      0.05,
      120
  );

  EXPECT_EQ(
      oms.children_for_parent(parent_id).size(),
      1U
  );

  EXPECT_EQ(
      oms.fills_for_parent(parent_id).size(),
      1U
  );

  EXPECT_EQ(oms.parent_count(), 1U);
  EXPECT_EQ(oms.child_count(), 1U);
}

TEST(OMS, RejectsInvalidParentAndChildRequests) {
  OrderManagementSystem oms;

  EXPECT_THROW(
      oms.create_parent(
          ParentOrderRequest{
              .symbol = "",
              .quantity = 1.0
          },
          100
      ),
      std::invalid_argument
  );

  EXPECT_THROW(
      oms.create_parent(
          ParentOrderRequest{
              .symbol = "btcusd",
              .quantity = 0.0
          },
          100
      ),
      std::invalid_argument
  );

  EXPECT_THROW(
      oms.create_child(
          ChildOrderRequest{
              .parent_id = 999,
              .venue = "coinbase",
              .price = 100.0,
              .quantity = 1.0,
              .created_timestamp_ns = 100
          }
      ),
      std::invalid_argument
  );
}

}  // namespace
