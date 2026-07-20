#pragma once

#include "minimatch/exchange.hpp"
#include "minimatch/oms.hpp"
#include "minimatch/risk.hpp"
#include "minimatch/replay_control.hpp"
#include "minimatch/latency_stats.hpp"

#include <cstdint>
#include <memory>
#include <mutex>

namespace minimatch {

struct ApiState {
    Exchange exchange;
    OrderManagementSystem oms;
    RiskEngine risk;
    ReplayController replay;
    LatencyStats latency;

    mutable std::mutex mutex;

    std::uint64_t processed_commands{0};
    std::uint64_t dropped_commands{0};
    std::uint64_t max_queue_depth{0};
};

} // namespace minimatch
