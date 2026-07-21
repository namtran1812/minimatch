#pragma once

#include "minimatch/position_manager.hpp"

#include <string>
#include <vector>

namespace minimatch {

struct PortfolioSummary {
  double gross_exposure{0.0};
  double net_exposure{0.0};

  double realized_pnl{0.0};
  double unrealized_pnl{0.0};
  double total_pnl{0.0};

  std::size_t position_count{0};

  std::string largest_position_symbol;
  double largest_position_exposure{0.0};
  double largest_concentration_percent{0.0};
};

struct PortfolioRiskLimits {
  double max_gross_exposure{1'000'000.0};
  double max_net_exposure{500'000.0};
  double max_concentration_percent{50.0};
  double max_daily_loss{50'000.0};
};

struct PortfolioRiskStatus {
  PortfolioSummary summary;
  PortfolioRiskLimits limits;

  bool gross_exposure_breached{false};
  bool net_exposure_breached{false};
  bool concentration_breached{false};
  bool daily_loss_breached{false};

  bool breached{false};
};

class PortfolioRiskManager {
 public:
  [[nodiscard]]
  PortfolioSummary summarize(
      const std::vector<
          PositionState
      >& positions
  ) const;

  [[nodiscard]]
  PortfolioRiskStatus evaluate(
      const std::vector<
          PositionState
      >& positions
  ) const;

  void set_limits(
      const PortfolioRiskLimits& limits
  );

  [[nodiscard]]
  PortfolioRiskLimits limits() const noexcept {
    return limits_;
  }

 private:
  PortfolioRiskLimits limits_;
};

}  // namespace minimatch
