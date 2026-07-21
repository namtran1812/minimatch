#include "minimatch/portfolio_risk.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace minimatch {

PortfolioSummary
PortfolioRiskManager::summarize(
    const std::vector<
        PositionState
    >& positions
) const {
  PortfolioSummary result;

  result.position_count =
      positions.size();

  for (
      const auto& position :
      positions
  ) {
    const double exposure =
        position.net_quantity *
        position.mark_price;

    const double absolute_exposure =
        std::abs(
            exposure
        );

    result.gross_exposure +=
        absolute_exposure;

    result.net_exposure +=
        exposure;

    result.realized_pnl +=
        position.realized_pnl;

    result.unrealized_pnl +=
        position.unrealized_pnl;

    if (
        absolute_exposure >
        result.largest_position_exposure
    ) {
      result.largest_position_exposure =
          absolute_exposure;

      result.largest_position_symbol =
          position.symbol;
    }
  }

  result.total_pnl =
      result.realized_pnl +
      result.unrealized_pnl;

  if (
      result.gross_exposure >
      1e-12
  ) {
    result
        .largest_concentration_percent =
        result
            .largest_position_exposure /
        result.gross_exposure *
        100.0;
  }

  return result;
}

PortfolioRiskStatus
PortfolioRiskManager::evaluate(
    const std::vector<
        PositionState
    >& positions
) const {
  PortfolioRiskStatus result;

  result.summary =
      summarize(
          positions
      );

  result.limits =
      limits_;

  result.gross_exposure_breached =
      limits_.max_gross_exposure >
          0.0 &&
      result.summary.gross_exposure >
          limits_.max_gross_exposure;

  result.net_exposure_breached =
      limits_.max_net_exposure >
          0.0 &&
      std::abs(
          result.summary.net_exposure
      ) >
          limits_.max_net_exposure;

  result.concentration_breached =
      limits_
              .max_concentration_percent >
          0.0 &&
      result.summary
              .largest_concentration_percent >
          limits_
              .max_concentration_percent;

  result.daily_loss_breached =
      limits_.max_daily_loss >
          0.0 &&
      result.summary.total_pnl <
          -limits_.max_daily_loss;

  result.breached =
      result.gross_exposure_breached ||
      result.net_exposure_breached ||
      result.concentration_breached ||
      result.daily_loss_breached;

  return result;
}

void PortfolioRiskManager::set_limits(
    const PortfolioRiskLimits& limits
) {
  if (
      limits.max_gross_exposure <
          0.0 ||
      limits.max_net_exposure <
          0.0 ||
      limits.max_concentration_percent <
          0.0 ||
      limits.max_daily_loss <
          0.0
  ) {
    throw std::invalid_argument(
        "portfolio risk limits must be non-negative"
    );
  }

  limits_ =
      limits;
}

}  // namespace minimatch
