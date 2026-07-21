#pragma once

#include "minimatch/oms.hpp"
#include "minimatch/router.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace minimatch {

struct PositionState {
  std::string symbol;

  double net_quantity{0.0};
  double average_cost{0.0};

  double realized_pnl{0.0};
  double unrealized_pnl{0.0};

  double mark_price{0.0};
};

class PositionManager {
 public:
  void apply_fill(
      const ParentOrder& parent,
      const OmsExecutionReport& fill
  );

  void mark(
      const std::string& symbol,
      double price
  );

  [[nodiscard]]
  PositionState position(
      const std::string& symbol
  ) const;

  [[nodiscard]]
  std::vector<PositionState>
  positions() const;

 private:
  std::unordered_map<
      std::string,
      PositionState
  > positions_;
};

}  // namespace minimatch
