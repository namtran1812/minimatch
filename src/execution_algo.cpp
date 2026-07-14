#include "minimatch/execution_algo.hpp"

namespace minimatch {

std::vector<Slice>
build_execution_schedule(
    ExecutionAlgorithm,
    double quantity,
    int slices,
    double duration_seconds)
{
    std::vector<Slice> result;

    if (slices <= 0)
        return result;

    double q = quantity / slices;
    double dt = duration_seconds / slices;

    for (int i = 0; i < slices; ++i)
        result.push_back({q, i * dt});

    return result;
}

}
