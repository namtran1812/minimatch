#pragma once

#include "minimatch/router.hpp"

#include <cstdint>
#include <string>

namespace minimatch {

enum class ExecutionRiskRejectReason {
  None,
  KillSwitchActive,
  InvalidOrder,
  ChildQuantityExceeded,
  ParentQuantityExceeded,
  NotionalExceeded,
  ParticipationExceeded,
  PriceCollarExceeded
};

struct ExecutionRiskLimits {
  bool kill_switch_active{false};

  double max_parent_quantity{0.0};
  double max_child_quantity{0.0};
  double max_order_notional{0.0};

  // Decimal fraction: 0.10 means 10%.
  // Zero disables the participation limit.
  double max_participation_rate{0.0};

  // Maximum distance from reference price.
  double price_collar_bps{10000.0};
};

struct ExecutionRiskCheck {
  RouteSide side{RouteSide::Buy};

  double parent_quantity{0.0};
  double child_quantity{0.0};

  double order_price{0.0};
  double reference_price{0.0};

  double observed_market_volume{0.0};
};

struct ExecutionRiskDecision {
  bool accepted{false};

  ExecutionRiskRejectReason reason{
      ExecutionRiskRejectReason::None
  };

  std::string message;
};

class ExecutionRiskManager {
 public:
  explicit ExecutionRiskManager(
      ExecutionRiskLimits limits = {}
  );

  [[nodiscard]] ExecutionRiskDecision check(
      const ExecutionRiskCheck& request
  ) const;

  void set_limits(
      const ExecutionRiskLimits& limits
  );

  [[nodiscard]] const ExecutionRiskLimits&
  limits() const;

  void activate_kill_switch();
  void deactivate_kill_switch();

 private:
  ExecutionRiskLimits limits_;

  static void validate_limits(
      const ExecutionRiskLimits& limits
  );
};

std::string to_string(
    ExecutionRiskRejectReason reason
);

}  // namespace minimatch
