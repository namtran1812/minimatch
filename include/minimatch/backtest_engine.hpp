#pragma once

#include "minimatch/execution_algo.hpp"
#include "minimatch/market_replay.hpp"
#include "minimatch/router.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace minimatch {

struct BacktestRequest {
  std::string symbol;
  RouteSide side{RouteSide::Buy};

  double quantity{0.0};

  ExecutionScheduleRequest schedule;

  double taker_fee_bps{0.0};
  double latency_ms{0.0};
  double latency_cost_bps_per_ms{0.0};
};

struct BacktestFill {
  std::size_t slice_index{0};
  std::uint64_t timestamp_ns{0};

  double requested_quantity{0.0};
  double filled_quantity{0.0};
  double remaining_quantity{0.0};

  double price{0.0};
  double notional{0.0};
  double fee{0.0};
};

struct BacktestResult {
  bool complete{false};

  double requested_quantity{0.0};
  double filled_quantity{0.0};
  double remaining_quantity{0.0};

  double arrival_price{0.0};
  double average_fill_price{0.0};
  double market_vwap{0.0};

  double implementation_shortfall_bps{0.0};
  double vwap_slippage_bps{0.0};

  double total_notional{0.0};
  double total_fees{0.0};

  std::vector<BacktestFill> fills;
};

BacktestResult run_historical_backtest(
    const BacktestRequest& request,
    const MarketReplay& replay
);

}  // namespace minimatch
