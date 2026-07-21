#include "minimatch/position_manager.hpp"

#include <algorithm>
#include <cmath>

namespace minimatch {

void PositionManager::apply_fill(
    const ParentOrder& parent,
    const OmsExecutionReport& fill
) {
  auto& position =
      positions_[parent.symbol];

  position.symbol =
      parent.symbol;

  const double signed_quantity =
      parent.side == RouteSide::Buy
          ? fill.quantity
          : -fill.quantity;

  const double old_quantity =
      position.net_quantity;

  const double new_quantity =
      old_quantity +
      signed_quantity;

  if (
      std::abs(old_quantity) < 1e-12 ||
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
        std::abs(old_quantity) *
        position.average_cost;

    const double new_notional =
        std::abs(signed_quantity) *
        fill.price;

    const double total_quantity =
        std::abs(old_quantity) +
        std::abs(signed_quantity);

    if (total_quantity > 1e-12) {
      position.average_cost =
          (
              old_notional +
              new_notional
          ) /
          total_quantity;
    }
  } else {
    const double closing_quantity =
        std::min(
            std::abs(old_quantity),
            std::abs(signed_quantity)
        );

    if (old_quantity > 0.0) {
      position.realized_pnl +=
          (
              fill.price -
              position.average_cost
          ) *
          closing_quantity;
    } else {
      position.realized_pnl +=
          (
              position.average_cost -
              fill.price
          ) *
          closing_quantity;
    }

    if (
        std::abs(signed_quantity) >
        std::abs(old_quantity)
    ) {
      position.average_cost =
          fill.price;
    }

    if (
        std::abs(new_quantity) <
        1e-12
    ) {
      position.average_cost =
          0.0;
    }
  }

  position.net_quantity =
      new_quantity;

  position.realized_pnl -=
      fill.fee;

  if (
      position.mark_price > 0.0
  ) {
    position.unrealized_pnl =
        (
            position.mark_price -
            position.average_cost
        ) *
        position.net_quantity;
  }
}

void PositionManager::mark(
    const std::string& symbol,
    double price
) {
  auto& position =
      positions_[symbol];

  position.symbol =
      symbol;

  position.mark_price =
      price;

  position.unrealized_pnl =
      (
          position.mark_price -
          position.average_cost
      ) *
      position.net_quantity;
}

PositionState PositionManager::position(
    const std::string& symbol
) const {
  const auto iterator =
      positions_.find(symbol);

  if (
      iterator ==
      positions_.end()
  ) {
    return PositionState{
        .symbol = symbol
    };
  }

  return iterator->second;
}

std::vector<PositionState>
PositionManager::positions() const {
  std::vector<PositionState>
      result;

  result.reserve(
      positions_.size()
  );

  for (
      const auto& [symbol, position] :
      positions_
  ) {
    (void)symbol;

    result.push_back(
        position
    );
  }

  std::sort(
      result.begin(),
      result.end(),
      [](
          const PositionState& left,
          const PositionState& right
      ) {
        return left.symbol <
            right.symbol;
      }
  );

  return result;
}

}  // namespace minimatch
