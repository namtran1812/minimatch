#pragma once

#include "minimatch/oms.hpp"
#include "minimatch/router.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace minimatch {

struct ExecutionMarkout {
  std::uint64_t horizon_ns{0};
  double reference_price{0.0};
  double markout_bps{0.0};
};

struct ExecutionQuality {
  ParentOrderId parent_order_id{0};
  std::string symbol;

  RouteSide side{
      RouteSide::Buy
  };

  double requested_quantity{0.0};
  double filled_quantity{0.0};

  double arrival_price{0.0};
  double average_fill_price{0.0};

  double total_notional{0.0};
  double total_fees{0.0};

  double implementation_shortfall_bps{
      0.0
  };

  double fee_adjusted_shortfall_bps{
      0.0
  };

  std::vector<ExecutionMarkout>
      markouts;
};

class ExecutionAnalytics {
 public:
  ExecutionQuality analyze(
      const ParentOrder& parent,
      const std::vector<
          OmsExecutionReport
      >& fills,
      double arrival_price,
      const std::vector<
          ExecutionMarkout
      >& markouts = {}
  ) const;
};

}  // namespace minimatch
