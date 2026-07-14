#pragma once

#include <vector>

namespace minimatch {

enum class ExecutionAlgorithm {
    Market,
    TWAP,
    VWAP,
    POV,
    Iceberg
};

struct Slice {
    double quantity{};
    double delay_seconds{};
};

std::vector<Slice>
build_execution_schedule(
    ExecutionAlgorithm algo,
    double quantity,
    int slices,
    double duration_seconds);

}
