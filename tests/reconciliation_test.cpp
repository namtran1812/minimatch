#include "minimatch/reconciliation.hpp"
#include "minimatch/oms.hpp"
#include "minimatch/position_manager.hpp"

#include <gtest/gtest.h>

#include <cmath>

namespace {

using namespace minimatch;

TEST(
    Reconciliation,
    ParentMatchesOmsAndChildFills
) {
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
          1
      );

  const auto child_id =
      oms.create_child(
          ChildOrderRequest{
              .parent_id = parent_id,
              .venue = "coinbase",
              .price = 65000.0,
              .quantity = 1.0,
              .created_timestamp_ns = 2
          }
      );

  ASSERT_TRUE(
      oms.mark_child_working(
          child_id,
          3
      )
  );

  oms.record_fill(
      child_id,
      65000.0,
      1.0,
      65.0,
      4
  );

  const auto parent =
      oms.find_parent(
          parent_id
      );

  ASSERT_TRUE(
      parent.has_value()
  );

  const auto fills =
      oms.fills_for_parent(
          parent_id
      );

  const auto children =
      oms.children_for_parent(
          parent_id
      );

  double fill_quantity =
      0.0;

  for (
      const auto& fill :
      fills
  ) {
    fill_quantity +=
        fill.quantity;
  }

  double child_fill_quantity =
      0.0;

  for (
      const auto& child :
      children
  ) {
    child_fill_quantity +=
        child.filled_quantity;
  }

  EXPECT_NEAR(
      parent->filled_quantity,
      1.0,
      1e-9
  );

  EXPECT_NEAR(
      fill_quantity,
      parent->filled_quantity,
      1e-9
  );

  EXPECT_NEAR(
      child_fill_quantity,
      parent->filled_quantity,
      1e-9
  );
}

}  // namespace

TEST(
    Reconciliation,
    PositionMatchesIndependentFillReconstruction
) {
  OrderManagementSystem oms;
  PositionManager positions;

  const auto buy_parent_id =
      oms.create_parent(
          ParentOrderRequest{
              .symbol = "btcusd",
              .side = RouteSide::Buy,
              .quantity = 2.0,
              .algorithm =
                  ExecutionAlgorithm::Market
          },
          10
      );

  const auto buy_child_id =
      oms.create_child(
          ChildOrderRequest{
              .parent_id = buy_parent_id,
              .venue = "coinbase",
              .price = 100.0,
              .quantity = 2.0,
              .created_timestamp_ns = 11
          }
      );

  oms.mark_child_working(
      buy_child_id,
      12
  );

  const auto buy_fill =
      oms.record_fill(
          buy_child_id,
          100.0,
          2.0,
          1.0,
          13
      );

  const auto buy_parent =
      oms.find_parent(
          buy_parent_id
      );

  ASSERT_TRUE(
      buy_parent.has_value()
  );

  positions.apply_fill(
      *buy_parent,
      buy_fill
  );

  const auto sell_parent_id =
      oms.create_parent(
          ParentOrderRequest{
              .symbol = "btcusd",
              .side = RouteSide::Sell,
              .quantity = 1.0,
              .algorithm =
                  ExecutionAlgorithm::Market
          },
          20
      );

  const auto sell_child_id =
      oms.create_child(
          ChildOrderRequest{
              .parent_id = sell_parent_id,
              .venue = "kraken",
              .price = 110.0,
              .quantity = 1.0,
              .created_timestamp_ns = 21
          }
      );

  oms.mark_child_working(
      sell_child_id,
      22
  );

  const auto sell_fill =
      oms.record_fill(
          sell_child_id,
          110.0,
          1.0,
          0.5,
          23
      );

  const auto sell_parent =
      oms.find_parent(
          sell_parent_id
      );

  ASSERT_TRUE(
      sell_parent.has_value()
  );

  positions.apply_fill(
      *sell_parent,
      sell_fill
  );

  const auto actual =
      positions.position(
          "btcusd"
      );

  const double expected_net_quantity =
      1.0;

  const double expected_average_cost =
      100.0;

  const double expected_realized_pnl =
      (
          110.0 -
          100.0
      ) *
      1.0 -
      1.0 -
      0.5;

  EXPECT_NEAR(
      actual.net_quantity,
      expected_net_quantity,
      1e-9
  );

  EXPECT_NEAR(
      actual.average_cost,
      expected_average_cost,
      1e-9
  );

  EXPECT_NEAR(
      actual.realized_pnl,
      expected_realized_pnl,
      1e-9
  );
}

TEST(
    Reconciliation,
    DetectsParentFillMismatch
) {
  OrderManagementSystem oms;

  ParentOrder parent;
  parent.id = 100;
  parent.symbol = "btcusd";
  parent.side = RouteSide::Buy;
  parent.algorithm =
      ExecutionAlgorithm::Market;
  parent.quantity = 1.0;

  // Deliberately corrupted parent state.
  parent.filled_quantity = 1.0;
  parent.remaining_quantity = 0.0;
  parent.status = OrderStatus::Filled;

  ChildOrder child;
  child.id = 200;
  child.parent_id = 100;
  child.venue = "coinbase";
  child.price = 100.0;
  child.quantity = 1.0;

  // Child only records half the quantity.
  child.filled_quantity = 0.5;
  child.remaining_quantity = 0.5;
  child.status =
      OrderStatus::PartiallyFilled;

  parent.child_ids.push_back(
      child.id
  );

  OmsExecutionReport fill;
  fill.execution_report_id = 300;
  fill.parent_id = parent.id;
  fill.child_id = child.id;
  fill.venue = "coinbase";
  fill.price = 100.0;
  fill.quantity = 0.5;
  fill.notional = 50.0;
  fill.fee = 0.05;
  fill.timestamp_ns = 10;

  oms.restore(
      {parent},
      {child},
      {fill}
  );

  const auto restored_parent =
      oms.find_parent(
          parent.id
      );

  ASSERT_TRUE(
      restored_parent.has_value()
  );

  const auto fills =
      oms.fills_for_parent(
          parent.id
      );

  const auto children =
      oms.children_for_parent(
          parent.id
      );

  double oms_fill_quantity = 0.0;

  for (
      const auto& report :
      fills
  ) {
    oms_fill_quantity +=
        report.quantity;
  }

  double child_fill_quantity = 0.0;

  for (
      const auto& restored_child :
      children
  ) {
    child_fill_quantity +=
        restored_child.filled_quantity;
  }

  EXPECT_NE(
      restored_parent->filled_quantity,
      oms_fill_quantity
  );

  EXPECT_NE(
      restored_parent->filled_quantity,
      child_fill_quantity
  );

  EXPECT_NEAR(
      restored_parent->filled_quantity,
      1.0,
      1e-9
  );

  EXPECT_NEAR(
      oms_fill_quantity,
      0.5,
      1e-9
  );

  EXPECT_NEAR(
      child_fill_quantity,
      0.5,
      1e-9
  );
}

TEST(
    Reconciliation,
    AggregatesParentClassifications
) {
    using minimatch::
        ParentReconciliationInput;
    using minimatch::
        ReconciliationClassification;

    const std::vector<
        ParentReconciliationInput
    > parents{
        {
            1,
            ReconciliationClassification::
                Consistent
        },
        {
            2,
            ReconciliationClassification::
                LegacyUnverifiable
        },
        {
            3,
            ReconciliationClassification::
                Inconsistent
        },
        {
            4,
            ReconciliationClassification::
                Consistent
        }
    };

    const auto result =
        minimatch::reconcile_parents(
            parents
        );

    EXPECT_EQ(
        result.parent_count,
        4
    );

    EXPECT_EQ(
        result.consistent_parents,
        2
    );

    EXPECT_EQ(
        result.legacy_unverifiable_parents,
        1
    );

    EXPECT_EQ(
        result.inconsistent_parents,
        1
    );

    EXPECT_FALSE(
        result.all_verifiable_consistent
    );

    ASSERT_EQ(
        result.legacy_unverifiable_ids.size(),
        1
    );

    EXPECT_EQ(
        result.legacy_unverifiable_ids[0],
        2
    );

    ASSERT_EQ(
        result.inconsistent_ids.size(),
        1
    );

    EXPECT_EQ(
        result.inconsistent_ids[0],
        3
    );
}


TEST(
    Reconciliation,
    ParentAggregationHealthyWithoutInconsistency
) {
    using minimatch::
        ParentReconciliationInput;
    using minimatch::
        ReconciliationClassification;

    const std::vector<
        ParentReconciliationInput
    > parents{
        {
            10,
            ReconciliationClassification::
                Consistent
        },
        {
            11,
            ReconciliationClassification::
                LegacyUnverifiable
        }
    };

    const auto result =
        minimatch::reconcile_parents(
            parents
        );

    EXPECT_EQ(
        result.parent_count,
        2
    );

    EXPECT_EQ(
        result.inconsistent_parents,
        0
    );

    EXPECT_TRUE(
        result.all_verifiable_consistent
    );
}

TEST(
    Reconciliation,
    PurePositionReconciliationMatchesExpectedState
) {
    minimatch::PositionReconciliationInput input;

    input.symbol = "btcusd";

    input.fills = {
        {
            .side = minimatch::RouteSide::Buy,
            .quantity = 1.0,
            .price = 100.0,
            .fee = 1.0
        },
        {
            .side = minimatch::RouteSide::Sell,
            .quantity = 0.4,
            .price = 110.0,
            .fee = 0.5
        }
    };

    input.actual = minimatch::PositionState{
        .symbol = "btcusd",
        .net_quantity = 0.6,
        .average_cost = 100.0,
        .realized_pnl = 2.5,
        .unrealized_pnl = 6.0,
        .mark_price = 110.0
    };

    const auto results =
        minimatch::reconcile_positions(
            {input}
        );

    ASSERT_EQ(
        results.size(),
        1
    );

    const auto& result =
        results.front();

    EXPECT_DOUBLE_EQ(
        result.expected_net_quantity,
        0.6
    );

    EXPECT_DOUBLE_EQ(
        result.expected_average_cost,
        100.0
    );

    EXPECT_DOUBLE_EQ(
        result.expected_realized_pnl,
        2.5
    );

    EXPECT_DOUBLE_EQ(
        result.expected_unrealized_pnl,
        6.0
    );

    EXPECT_DOUBLE_EQ(
        result.expected_total_pnl,
        8.5
    );

    EXPECT_TRUE(
        result.consistent
    );
}


TEST(
    Reconciliation,
    PurePositionReconciliationDetectsMismatch
) {
    minimatch::PositionReconciliationInput input;

    input.symbol = "ethusd";

    input.fills = {
        {
            .side = minimatch::RouteSide::Buy,
            .quantity = 2.0,
            .price = 50.0,
            .fee = 0.0
        }
    };

    input.actual = minimatch::PositionState{
        .symbol = "ethusd",
        .net_quantity = 1.5,
        .average_cost = 50.0,
        .realized_pnl = 0.0,
        .unrealized_pnl = 0.0,
        .mark_price = 50.0
    };

    const auto results =
        minimatch::reconcile_positions(
            {input}
        );

    ASSERT_EQ(
        results.size(),
        1
    );

    const auto& result =
        results.front();

    EXPECT_FALSE(
        result.quantity_consistent
    );

    EXPECT_FALSE(
        result.consistent
    );
}


TEST(
    Reconciliation,
    PurePositionReconciliationHandlesPositionFlip
) {
    minimatch::PositionReconciliationInput input;

    input.symbol = "solusd";

    input.fills = {
        {
            .side = minimatch::RouteSide::Buy,
            .quantity = 1.0,
            .price = 100.0,
            .fee = 0.0
        },
        {
            .side = minimatch::RouteSide::Sell,
            .quantity = 2.0,
            .price = 120.0,
            .fee = 0.0
        }
    };

    input.actual = minimatch::PositionState{
        .symbol = "solusd",
        .net_quantity = -1.0,
        .average_cost = 120.0,
        .realized_pnl = 20.0,
        .unrealized_pnl = 0.0,
        .mark_price = 120.0
    };

    const auto results =
        minimatch::reconcile_positions(
            {input}
        );

    ASSERT_EQ(
        results.size(),
        1
    );

    const auto& result =
        results.front();

    EXPECT_DOUBLE_EQ(
        result.expected_net_quantity,
        -1.0
    );

    EXPECT_DOUBLE_EQ(
        result.expected_average_cost,
        120.0
    );

    EXPECT_DOUBLE_EQ(
        result.expected_realized_pnl,
        20.0
    );

    EXPECT_TRUE(
        result.consistent
    );
}
