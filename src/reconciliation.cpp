#include "minimatch/reconciliation.hpp"
#include <cmath>
#include <algorithm>

namespace minimatch {

std::vector<PositionReconciliationResult>
reconcile_positions(
    const std::vector<
        PositionReconciliationInput
    >& inputs,
    double epsilon
) {
    std::vector<
        PositionReconciliationResult
    > results;

    results.reserve(
        inputs.size()
    );

    for (
        const auto& input :
        inputs
    ) {
        double expected_net_quantity =
            0.0;

        double expected_average_cost =
            0.0;

        double expected_realized_pnl =
            0.0;

        for (
            const auto& fill :
            input.fills
        ) {
            const double signed_quantity =
                fill.side ==
                        RouteSide::Buy
                    ? fill.quantity
                    : -fill.quantity;

            const double old_quantity =
                expected_net_quantity;

            const double new_quantity =
                old_quantity +
                signed_quantity;

            if (
                std::abs(old_quantity) <
                    1e-12 ||
                (
                    old_quantity > 0.0 &&
                    signed_quantity > 0.0
                ) ||
                (
                    old_quantity < 0.0 &&
                    signed_quantity < 0.0
                )
            ) {
                const double old_notional =
                    std::abs(
                        old_quantity
                    ) *
                    expected_average_cost;

                const double new_notional =
                    std::abs(
                        signed_quantity
                    ) *
                    fill.price;

                const double total_quantity =
                    std::abs(
                        old_quantity
                    ) +
                    std::abs(
                        signed_quantity
                    );

                if (
                    total_quantity >
                    1e-12
                ) {
                    expected_average_cost =
                        (
                            old_notional +
                            new_notional
                        ) /
                        total_quantity;
                }
            } else {
                const double closing_quantity =
                    std::min(
                        std::abs(
                            old_quantity
                        ),
                        std::abs(
                            signed_quantity
                        )
                    );

                if (
                    old_quantity >
                    0.0
                ) {
                    expected_realized_pnl +=
                        (
                            fill.price -
                            expected_average_cost
                        ) *
                        closing_quantity;
                } else {
                    expected_realized_pnl +=
                        (
                            expected_average_cost -
                            fill.price
                        ) *
                        closing_quantity;
                }

                if (
                    std::abs(
                        signed_quantity
                    ) >
                    std::abs(
                        old_quantity
                    )
                ) {
                    expected_average_cost =
                        fill.price;
                }

                if (
                    std::abs(
                        new_quantity
                    ) <
                    1e-12
                ) {
                    expected_average_cost =
                        0.0;
                }
            }

            expected_net_quantity =
                new_quantity;

            expected_realized_pnl -=
                fill.fee;
        }

        PositionReconciliationResult result;

        result.symbol =
            input.symbol;

        result.expected_net_quantity =
            expected_net_quantity;

        result.actual_net_quantity =
            input.actual.net_quantity;

        result.quantity_difference =
            result.actual_net_quantity -
            result.expected_net_quantity;

        result.expected_average_cost =
            expected_average_cost;

        result.actual_average_cost =
            input.actual.average_cost;

        result.average_cost_difference =
            result.actual_average_cost -
            result.expected_average_cost;

        result.expected_realized_pnl =
            expected_realized_pnl;

        result.actual_realized_pnl =
            input.actual.realized_pnl;

        result.realized_pnl_difference =
            result.actual_realized_pnl -
            result.expected_realized_pnl;

        result.mark_price =
            input.actual.mark_price;

        result.expected_unrealized_pnl =
            (
                result.mark_price -
                result.expected_average_cost
            ) *
            result.expected_net_quantity;

        result.actual_unrealized_pnl =
            input.actual.unrealized_pnl;

        result.unrealized_pnl_difference =
            result.actual_unrealized_pnl -
            result.expected_unrealized_pnl;

        result.expected_total_pnl =
            result.expected_realized_pnl +
            result.expected_unrealized_pnl;

        result.actual_total_pnl =
            result.actual_realized_pnl +
            result.actual_unrealized_pnl;

        result.total_pnl_difference =
            result.actual_total_pnl -
            result.expected_total_pnl;

        result.quantity_consistent =
            std::abs(
                result.quantity_difference
            ) <= epsilon;

        result.average_cost_consistent =
            std::abs(
                result.average_cost_difference
            ) <= epsilon;

        result.realized_pnl_consistent =
            std::abs(
                result.realized_pnl_difference
            ) <= epsilon;

        result.unrealized_pnl_consistent =
            std::abs(
                result.unrealized_pnl_difference
            ) <= epsilon;

        result.total_pnl_consistent =
            std::abs(
                result.total_pnl_difference
            ) <= epsilon;

        result.consistent =
            result.quantity_consistent &&
            result.average_cost_consistent &&
            result.realized_pnl_consistent &&
            result.unrealized_pnl_consistent &&
            result.total_pnl_consistent;

        results.push_back(
            std::move(result)
        );
    }

    std::sort(
        results.begin(),
        results.end(),
        [](
            const auto& left,
            const auto& right
        ) {
            return left.symbol <
                right.symbol;
        }
    );

    return results;
}


GlobalParentReconciliationResult
reconcile_parents(
    const std::vector<
        ParentReconciliationInput
    >& parents
) {
    GlobalParentReconciliationResult result;

    result.parent_count =
        parents.size();

    for (const auto& parent : parents) {
        switch (parent.classification) {
            case ReconciliationClassification::
                Consistent:
                ++result.consistent_parents;
                break;

            case ReconciliationClassification::
                LegacyUnverifiable:
                ++result
                    .legacy_unverifiable_parents;

                result
                    .legacy_unverifiable_ids
                    .push_back(
                        parent.parent_id
                    );
                break;

            case ReconciliationClassification::
                Inconsistent:
                ++result
                    .inconsistent_parents;

                result
                    .inconsistent_ids
                    .push_back(
                        parent.parent_id
                    );
                break;
        }
    }

    result.all_verifiable_consistent =
        result.inconsistent_parents == 0;

    return result;
}

}  // namespace minimatch
