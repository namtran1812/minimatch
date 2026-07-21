#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "minimatch/oms.hpp"
#include "minimatch/position_manager.hpp"

namespace minimatch {

enum class ReconciliationClassification {
    Consistent,
    LegacyUnverifiable,
    Inconsistent
};

struct PositionReconciliationResult {
    std::string symbol;

    double expected_net_quantity{0.0};
    double actual_net_quantity{0.0};
    double quantity_difference{0.0};

    double expected_average_cost{0.0};
    double actual_average_cost{0.0};
    double average_cost_difference{0.0};

    double expected_realized_pnl{0.0};
    double actual_realized_pnl{0.0};
    double realized_pnl_difference{0.0};

    double mark_price{0.0};

    double expected_unrealized_pnl{0.0};
    double actual_unrealized_pnl{0.0};
    double unrealized_pnl_difference{0.0};

    double expected_total_pnl{0.0};
    double actual_total_pnl{0.0};
    double total_pnl_difference{0.0};

    bool quantity_consistent{false};
    bool average_cost_consistent{false};
    bool realized_pnl_consistent{false};
    bool unrealized_pnl_consistent{false};
    bool total_pnl_consistent{false};

    bool consistent{false};
};


struct PositionFillInput {
    RouteSide side{RouteSide::Buy};

    double quantity{0.0};
    double price{0.0};
    double fee{0.0};
};

struct PositionReconciliationInput {
    std::string symbol;

    std::vector<PositionFillInput> fills;

    PositionState actual;
};

std::vector<PositionReconciliationResult>
reconcile_positions(
    const std::vector<
        PositionReconciliationInput
    >& inputs,
    double epsilon = 1e-9
);


struct ParentReconciliationInput {
    ParentOrderId parent_id{};
    ReconciliationClassification classification{
        ReconciliationClassification::Consistent
    };
};

struct GlobalParentReconciliationResult {
    std::size_t parent_count{0};

    std::size_t consistent_parents{0};
    std::size_t legacy_unverifiable_parents{0};
    std::size_t inconsistent_parents{0};

    std::vector<ParentOrderId>
        legacy_unverifiable_ids;

    std::vector<ParentOrderId>
        inconsistent_ids;

    bool all_verifiable_consistent{true};
};

GlobalParentReconciliationResult
reconcile_parents(
    const std::vector<
        ParentReconciliationInput
    >& parents
);

}  // namespace minimatch
