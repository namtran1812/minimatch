#include "minimatch/midpoint_history.hpp"

#include <algorithm>
#include <cmath>

namespace minimatch {

MidpointHistory::MidpointHistory(
    std::uint64_t retention_ns
) : retention_ns_(
        retention_ns
    ) {}

void MidpointHistory::record(
    const std::string& symbol,
    std::uint64_t timestamp_ns,
    double midpoint
) {
  if (
      timestamp_ns == 0 ||
      !std::isfinite(midpoint) ||
      midpoint <= 0.0
  ) {
    return;
  }

  std::lock_guard lock(
      mutex_
  );

  auto& history =
      observations_[symbol];

  history.push_back(
      {
          .timestamp_ns =
              timestamp_ns,
          .midpoint =
              midpoint
      }
  );

  const auto cutoff =
      timestamp_ns >
              retention_ns_
          ? timestamp_ns -
                retention_ns_
          : 0;

  while (
      !history.empty() &&
      history.front()
              .timestamp_ns <
          cutoff
  ) {
    history.pop_front();
  }
}

std::optional<MidpointObservation>
MidpointHistory::at_or_after(
    const std::string& symbol,
    std::uint64_t timestamp_ns
) const {
  std::lock_guard lock(
      mutex_
  );

  const auto iterator =
      observations_.find(
          symbol
      );

  if (
      iterator ==
      observations_.end()
  ) {
    return std::nullopt;
  }

  const auto& history =
      iterator->second;

  const auto found =
      std::lower_bound(
          history.begin(),
          history.end(),
          timestamp_ns,
          [](
              const MidpointObservation&
                  observation,
              std::uint64_t value
          ) {
            return observation
                       .timestamp_ns <
                   value;
          }
      );

  if (
      found ==
      history.end()
  ) {
    return std::nullopt;
  }

  return *found;
}

std::size_t MidpointHistory::size(
    const std::string& symbol
) const {
  std::lock_guard lock(
      mutex_
  );

  const auto iterator =
      observations_.find(
          symbol
      );

  return iterator ==
                 observations_.end()
             ? 0
             : iterator->second.size();
}

}  // namespace minimatch

namespace minimatch {

std::optional<MidpointObservation>
MidpointHistory::first(
    const std::string& symbol
) const {
  std::lock_guard lock(
      mutex_
  );

  const auto iterator =
      observations_.find(
          symbol
      );

  if (
      iterator ==
          observations_.end() ||
      iterator->second.empty()
  ) {
    return std::nullopt;
  }

  return iterator
      ->second
      .front();
}

std::optional<MidpointObservation>
MidpointHistory::last(
    const std::string& symbol
) const {
  std::lock_guard lock(
      mutex_
  );

  const auto iterator =
      observations_.find(
          symbol
      );

  if (
      iterator ==
          observations_.end() ||
      iterator->second.empty()
  ) {
    return std::nullopt;
  }

  return iterator
      ->second
      .back();
}

}  // namespace minimatch

namespace minimatch {

std::optional<MidpointObservation>
MidpointHistory::at_or_before(
    const std::string& symbol,
    std::uint64_t timestamp_ns
) const {
  std::lock_guard lock(
      mutex_
  );

  const auto iterator =
      observations_.find(
          symbol
      );

  if (
      iterator ==
          observations_.end() ||
      iterator->second.empty()
  ) {
    return std::nullopt;
  }

  const auto& history =
      iterator->second;

  const auto found =
      std::upper_bound(
          history.begin(),
          history.end(),
          timestamp_ns,
          [](
              std::uint64_t value,
              const MidpointObservation&
                  observation
          ) {
            return value <
                observation.timestamp_ns;
          }
      );

  if (
      found ==
      history.begin()
  ) {
    return std::nullopt;
  }

  return *std::prev(
      found
  );
}

}  // namespace minimatch
