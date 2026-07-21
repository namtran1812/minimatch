#pragma once

#include "minimatch/execution_algo.hpp"
#include "minimatch/execution_engine.hpp"
#include "minimatch/oms.hpp"
#include "minimatch/router.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace minimatch {

struct LiveAlgoRequest {
  std::string symbol;

  RouteSide side{
      RouteSide::Buy
  };

  double quantity{0.0};

  ExecutionScheduleRequest schedule;

  double limit_price{0.0};
  double max_slippage_bps{10000.0};

  std::size_t max_venue_count{
      100
  };

  bool all_or_none{false};

  ExecutionSimulationConfig
      simulation;
};

struct LiveAlgoSliceResult {
  std::size_t slice_index{0};

  double requested_quantity{0.0};
  double routed_quantity{0.0};
  double filled_quantity{0.0};

  double notional{0.0};
  double fees{0.0};

  std::size_t child_count{0};
};

struct LiveAlgoResult {
  ParentOrderId parent_order_id{0};

  bool complete{false};

  double requested_quantity{0.0};
  double filled_quantity{0.0};
  double remaining_quantity{0.0};

  double average_fill_price{0.0};

  double total_notional{0.0};
  double total_fees{0.0};

  std::size_t accepted_child_count{0};
  std::size_t rejected_child_count{0};

  std::vector<LiveAlgoSliceResult>
      slices;
};

using LiveQuoteProvider =
    std::function<
        std::vector<VenueQuote>(
            const std::string&
        )
    >;

LiveAlgoResult execute_live_algorithm(
    const LiveAlgoRequest& request,
    OrderManagementSystem& oms,
    const LiveQuoteProvider&
        quote_provider,
    std::uint64_t timestamp_ns
);

}  // namespace minimatch
