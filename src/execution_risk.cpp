#include "minimatch/execution_risk.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace minimatch {

namespace {

constexpr double epsilon = 1e-12;

ExecutionRiskDecision reject(
    ExecutionRiskRejectReason reason,
    std::string message
) {
  return ExecutionRiskDecision{
      .accepted = false,
      .reason = reason,
      .message = std::move(message)
  };
}

}  // namespace

std::string to_string(
    ExecutionRiskRejectReason reason
) {
  switch (reason) {
    case ExecutionRiskRejectReason::None:
      return "NONE";

    case ExecutionRiskRejectReason::KillSwitchActive:
      return "KILL_SWITCH_ACTIVE";

    case ExecutionRiskRejectReason::InvalidOrder:
      return "INVALID_ORDER";

    case ExecutionRiskRejectReason::ChildQuantityExceeded:
      return "CHILD_QUANTITY_EXCEEDED";

    case ExecutionRiskRejectReason::ParentQuantityExceeded:
      return "PARENT_QUANTITY_EXCEEDED";

    case ExecutionRiskRejectReason::NotionalExceeded:
      return "NOTIONAL_EXCEEDED";

    case ExecutionRiskRejectReason::ParticipationExceeded:
      return "PARTICIPATION_EXCEEDED";

    case ExecutionRiskRejectReason::PriceCollarExceeded:
      return "PRICE_COLLAR_EXCEEDED";
  }

  return "UNKNOWN";
}

ExecutionRiskManager::ExecutionRiskManager(
    ExecutionRiskLimits limits
)
    : limits_(std::move(limits)) {
  validate_limits(limits_);
}

ExecutionRiskDecision ExecutionRiskManager::check(
    const ExecutionRiskCheck& request
) const {
  if (limits_.kill_switch_active) {
    return reject(
        ExecutionRiskRejectReason::KillSwitchActive,
        "execution kill switch is active"
    );
  }

  if (!std::isfinite(request.parent_quantity) ||
      !std::isfinite(request.child_quantity) ||
      !std::isfinite(request.order_price) ||
      !std::isfinite(request.reference_price) ||
      !std::isfinite(request.observed_market_volume) ||
      request.parent_quantity <= 0.0 ||
      request.child_quantity <= 0.0 ||
      request.order_price <= 0.0 ||
      request.reference_price <= 0.0 ||
      request.observed_market_volume < 0.0) {
    return reject(
        ExecutionRiskRejectReason::InvalidOrder,
        "execution risk request contains invalid values"
    );
  }

  if (limits_.max_parent_quantity > epsilon &&
      request.parent_quantity >
          limits_.max_parent_quantity + epsilon) {
    return reject(
        ExecutionRiskRejectReason::ParentQuantityExceeded,
        "parent quantity exceeds configured limit"
    );
  }

  if (limits_.max_child_quantity > epsilon &&
      request.child_quantity >
          limits_.max_child_quantity + epsilon) {
    return reject(
        ExecutionRiskRejectReason::ChildQuantityExceeded,
        "child quantity exceeds configured limit"
    );
  }

  const double notional =
      request.child_quantity *
      request.order_price;

  if (limits_.max_order_notional > epsilon &&
      notional >
          limits_.max_order_notional + epsilon) {
    return reject(
        ExecutionRiskRejectReason::NotionalExceeded,
        "child order notional exceeds configured limit"
    );
  }

  if (request.observed_market_volume > epsilon) {
    const double participation_rate =
        request.child_quantity /
        request.observed_market_volume;

    if (participation_rate >
        limits_.max_participation_rate + epsilon) {
      return reject(
          ExecutionRiskRejectReason::ParticipationExceeded,
          "child order exceeds maximum participation rate"
      );
    }
  }

  const double price_difference =
      request.side == RouteSide::Buy
          ? request.order_price -
                request.reference_price
          : request.reference_price -
                request.order_price;

  const double adverse_difference =
      price_difference > 0.0
          ? price_difference
          : 0.0;

  const double price_distance_bps =
      adverse_difference /
      request.reference_price *
      10000.0;

  if (price_distance_bps >
      limits_.price_collar_bps + epsilon) {
    return reject(
        ExecutionRiskRejectReason::PriceCollarExceeded,
        "child order exceeds configured price collar"
    );
  }

  return ExecutionRiskDecision{
      .accepted = true,
      .reason = ExecutionRiskRejectReason::None,
      .message = "accepted"
  };
}

void ExecutionRiskManager::set_limits(
    const ExecutionRiskLimits& limits
) {
  validate_limits(limits);
  limits_ = limits;
}

const ExecutionRiskLimits&
ExecutionRiskManager::limits() const {
  return limits_;
}

void ExecutionRiskManager::activate_kill_switch() {
  limits_.kill_switch_active = true;
}

void ExecutionRiskManager::deactivate_kill_switch() {
  limits_.kill_switch_active = false;
}

void ExecutionRiskManager::validate_limits(
    const ExecutionRiskLimits& limits
) {
  const auto non_negative =
      [](double value) {
        return std::isfinite(value) &&
               value >= 0.0;
      };

  if (!non_negative(
          limits.max_parent_quantity
      )) {
    throw std::invalid_argument(
        "maximum parent quantity must be non-negative"
    );
  }

  if (!non_negative(
          limits.max_child_quantity
      )) {
    throw std::invalid_argument(
        "maximum child quantity must be non-negative"
    );
  }

  if (!non_negative(
          limits.max_order_notional
      )) {
    throw std::invalid_argument(
        "maximum order notional must be non-negative"
    );
  }

  if (!std::isfinite(
          limits.max_participation_rate
      ) ||
      limits.max_participation_rate <= 0.0 ||
      limits.max_participation_rate > 1.0) {
    throw std::invalid_argument(
        "maximum participation rate must be in (0, 1]"
    );
  }

  if (!non_negative(
          limits.price_collar_bps
      )) {
    throw std::invalid_argument(
        "price collar must be non-negative"
    );
  }
}

}  // namespace minimatch
