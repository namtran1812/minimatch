#include "minimatch/execution_algo.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace minimatch {

namespace {

constexpr double epsilon = 1e-12;

void validate_common(
    const ExecutionScheduleRequest& request
) {
  if (!std::isfinite(request.quantity) ||
      request.quantity <= 0.0) {
    throw std::invalid_argument(
        "execution quantity must be positive"
    );
  }

  if (request.slices <= 0) {
    throw std::invalid_argument(
        "slice count must be positive"
    );
  }

  if (!std::isfinite(request.duration_seconds) ||
      request.duration_seconds < 0.0) {
    throw std::invalid_argument(
        "duration must be non-negative"
    );
  }
}

double interval_seconds(
    double duration_seconds,
    std::size_t slice_count
) {
  if (slice_count <= 1) {
    return 0.0;
  }

  return duration_seconds /
      static_cast<double>(slice_count);
}

std::vector<Slice> build_market_schedule(
    const ExecutionScheduleRequest& request
) {
  return {
      Slice{
          request.quantity,
          0.0
      }
  };
}

std::vector<Slice> build_twap_schedule(
    const ExecutionScheduleRequest& request
) {
  const std::size_t count =
      static_cast<std::size_t>(request.slices);

  const double interval =
      interval_seconds(
          request.duration_seconds,
          count
      );

  const double base_quantity =
      request.quantity /
      static_cast<double>(count);

  std::vector<Slice> schedule;
  schedule.reserve(count);

  double scheduled = 0.0;

  for (std::size_t index = 0;
       index < count;
       ++index) {
    const double quantity =
        index + 1 == count
            ? request.quantity - scheduled
            : base_quantity;

    schedule.push_back(
        Slice{
            quantity,
            static_cast<double>(index) *
                interval
        }
    );

    scheduled += quantity;
  }

  return schedule;
}

std::vector<Slice> build_vwap_schedule(
    const ExecutionScheduleRequest& request
) {
  const std::size_t count =
      static_cast<std::size_t>(request.slices);

  if (request.volume_profile.size() != count) {
    throw std::invalid_argument(
        "VWAP volume profile must match slice count"
    );
  }

  for (double weight : request.volume_profile) {
    if (!std::isfinite(weight) || weight < 0.0) {
      throw std::invalid_argument(
          "VWAP volume weights must be non-negative"
      );
    }
  }

  const double total_weight = std::accumulate(
      request.volume_profile.begin(),
      request.volume_profile.end(),
      0.0
  );

  if (total_weight <= epsilon) {
    throw std::invalid_argument(
        "VWAP volume profile must have positive weight"
    );
  }

  const double interval =
      interval_seconds(
          request.duration_seconds,
          count
      );

  std::vector<Slice> schedule;
  schedule.reserve(count);

  double scheduled = 0.0;

  for (std::size_t index = 0;
       index < count;
       ++index) {
    const double quantity =
        index + 1 == count
            ? request.quantity - scheduled
            : request.quantity *
                  request.volume_profile[index] /
                  total_weight;

    schedule.push_back(
        Slice{
            quantity,
            static_cast<double>(index) *
                interval
        }
    );

    scheduled += quantity;
  }

  return schedule;
}

std::vector<Slice> build_pov_schedule(
    const ExecutionScheduleRequest& request
) {
  if (!std::isfinite(request.participation_rate) ||
      request.participation_rate <= 0.0 ||
      request.participation_rate > 1.0) {
    throw std::invalid_argument(
        "POV participation rate must be in (0, 1]"
    );
  }

  if (request.volume_profile.empty()) {
    throw std::invalid_argument(
        "POV requires an expected volume profile"
    );
  }

  for (double volume : request.volume_profile) {
    if (!std::isfinite(volume) || volume < 0.0) {
      throw std::invalid_argument(
          "POV market volume must be non-negative"
      );
    }
  }

  const double interval =
      interval_seconds(
          request.duration_seconds,
          request.volume_profile.size()
      );

  std::vector<Slice> schedule;
  schedule.reserve(request.volume_profile.size());

  double remaining = request.quantity;

  for (std::size_t index = 0;
       index < request.volume_profile.size();
       ++index) {
    if (remaining <= epsilon) {
      break;
    }

    const double allowed_quantity =
        request.volume_profile[index] *
        request.participation_rate;

    const double quantity =
        std::min(remaining, allowed_quantity);

    if (quantity <= epsilon) {
      continue;
    }

    schedule.push_back(
        Slice{
            quantity,
            static_cast<double>(index) *
                interval
        }
    );

    remaining -= quantity;
  }

  return schedule;
}

std::vector<Slice> build_iceberg_schedule(
    const ExecutionScheduleRequest& request
) {
  if (!std::isfinite(request.displayed_quantity) ||
      request.displayed_quantity <= 0.0) {
    throw std::invalid_argument(
        "iceberg displayed quantity must be positive"
    );
  }

  const std::size_t count =
      static_cast<std::size_t>(
          std::ceil(
              request.quantity /
              request.displayed_quantity
          )
      );

  const double interval =
      interval_seconds(
          request.duration_seconds,
          count
      );

  std::vector<Slice> schedule;
  schedule.reserve(count);

  double remaining = request.quantity;

  for (std::size_t index = 0;
       index < count;
       ++index) {
    const double quantity =
        std::min(
            remaining,
            request.displayed_quantity
        );

    schedule.push_back(
        Slice{
            quantity,
            static_cast<double>(index) *
                interval
        }
    );

    remaining -= quantity;
  }

  return schedule;
}

}  // namespace

std::vector<Slice> build_execution_schedule(
    const ExecutionScheduleRequest& request
) {
  validate_common(request);

  switch (request.algorithm) {
    case ExecutionAlgorithm::Market:
      return build_market_schedule(request);

    case ExecutionAlgorithm::TWAP:
      return build_twap_schedule(request);

    case ExecutionAlgorithm::VWAP:
      return build_vwap_schedule(request);

    case ExecutionAlgorithm::POV:
      return build_pov_schedule(request);

    case ExecutionAlgorithm::Iceberg:
      return build_iceberg_schedule(request);
  }

  throw std::invalid_argument(
      "unsupported execution algorithm"
  );
}

std::vector<Slice> build_execution_schedule(
    ExecutionAlgorithm algorithm,
    double quantity,
    int slices,
    double duration_seconds
) {
  return build_execution_schedule(
      ExecutionScheduleRequest{
          .algorithm = algorithm,
          .quantity = quantity,
          .slices = slices,
          .duration_seconds = duration_seconds
      }
  );
}

}  // namespace minimatch
