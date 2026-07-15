#include "minimatch/venue_health.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace minimatch {

namespace {

void validate_config(
    const VenueHealthConfig& config
) {
  if (
      config.delayed_after_ns == 0 ||
      config.stale_after_ns <
          config.delayed_after_ns ||
      config.disconnected_after_ns <
          config.stale_after_ns ||
      config.rate_window_ns == 0
  ) {
    throw std::invalid_argument(
        "invalid venue health configuration"
    );
  }
}

}  // namespace

std::string to_string(
    VenueHealthStatus status
) {
  switch (status) {
    case VenueHealthStatus::Unknown:
      return "unknown";

    case VenueHealthStatus::Healthy:
      return "healthy";

    case VenueHealthStatus::Delayed:
      return "delayed";

    case VenueHealthStatus::Stale:
      return "stale";

    case VenueHealthStatus::Disconnected:
      return "disconnected";
  }

  return "unknown";
}

VenueHealthMonitor::VenueHealthMonitor(
    VenueHealthConfig config
)
    : config_(config) {
  validate_config(config_);
}

void VenueHealthMonitor::record_message(
    const std::string& venue,
    std::uint64_t timestamp_ns,
    bool snapshot
) {
  if (venue.empty() ||
      timestamp_ns == 0) {
    throw std::invalid_argument(
        "venue and timestamp are required"
    );
  }

  std::lock_guard<std::mutex> lock(
      mutex_
  );

  auto& state = states_[venue];

  if (
      state.first_window_message_ns == 0 ||
      timestamp_ns <
          state.first_window_message_ns ||
      timestamp_ns -
              state.first_window_message_ns >
          config_.rate_window_ns
  ) {
    state.first_window_message_ns =
        timestamp_ns;

    state.window_message_count = 0;
  }

  ++state.window_message_count;
  ++state.message_count;

  state.last_message_ns =
      std::max(
          state.last_message_ns,
          timestamp_ns
      );

  if (snapshot) {
    ++state.snapshot_count;
  } else {
    ++state.update_count;
  }
}

void VenueHealthMonitor::record_snapshot(
    const std::string& venue,
    std::uint64_t timestamp_ns
) {
  record_message(
      venue,
      timestamp_ns,
      true
  );
}

void VenueHealthMonitor::record_update(
    const std::string& venue,
    std::uint64_t timestamp_ns
) {
  record_message(
      venue,
      timestamp_ns,
      false
  );
}

void VenueHealthMonitor::record_reconnect(
    const std::string& venue
) {
  std::lock_guard<std::mutex> lock(
      mutex_
  );

  ++states_[venue].reconnect_count;
}

void VenueHealthMonitor::record_rejection(
    const std::string& venue
) {
  std::lock_guard<std::mutex> lock(
      mutex_
  );

  ++states_[venue].rejected_count;
}

void VenueHealthMonitor::record_sequence_gap(
    const std::string& venue
) {
  std::lock_guard<std::mutex> lock(
      mutex_
  );

  ++states_[venue].sequence_gap_count;
}

void VenueHealthMonitor::record_checksum_error(
    const std::string& venue
) {
  std::lock_guard<std::mutex> lock(
      mutex_
  );

  ++states_[venue].checksum_error_count;
}

void VenueHealthMonitor::set_synchronized(
    const std::string& venue,
    bool synchronized
) {
  std::lock_guard<std::mutex> lock(
      mutex_
  );

  states_[venue].synchronized =
      synchronized;
}

VenueHealthSnapshot
VenueHealthMonitor::build_snapshot(
    const std::string& venue,
    const VenueState& state,
    std::uint64_t now_ns
) const {
  const std::uint64_t quote_age_ns =
      state.last_message_ns == 0 ||
              now_ns < state.last_message_ns
          ? 0
          : now_ns - state.last_message_ns;

  VenueHealthStatus status =
      VenueHealthStatus::Unknown;

  if (state.last_message_ns != 0) {
    if (!state.synchronized ||
        quote_age_ns >=
            config_.disconnected_after_ns) {
      status =
          VenueHealthStatus::Disconnected;
    } else if (
        quote_age_ns >=
        config_.stale_after_ns
    ) {
      status = VenueHealthStatus::Stale;
    } else if (
        quote_age_ns >=
        config_.delayed_after_ns
    ) {
      status = VenueHealthStatus::Delayed;
    } else {
      status = VenueHealthStatus::Healthy;
    }
  }

  double messages_per_second = 0.0;

  if (
      state.first_window_message_ns != 0 &&
      now_ns >
          state.first_window_message_ns
  ) {
    const double elapsed_seconds =
        static_cast<double>(
            now_ns -
            state.first_window_message_ns
        ) /
        1'000'000'000.0;

    if (elapsed_seconds > 0.0) {
      messages_per_second =
          static_cast<double>(
              state.window_message_count
          ) /
          elapsed_seconds;
    }
  }

  return VenueHealthSnapshot{
      .venue = venue,
      .status = status,
      .synchronized = state.synchronized,
      .last_message_ns =
          state.last_message_ns,
      .quote_age_ns = quote_age_ns,
      .message_count =
          state.message_count,
      .snapshot_count =
          state.snapshot_count,
      .update_count =
          state.update_count,
      .reconnect_count =
          state.reconnect_count,
      .rejected_count =
          state.rejected_count,
      .sequence_gap_count =
          state.sequence_gap_count,
      .checksum_error_count =
          state.checksum_error_count,
      .messages_per_second =
          messages_per_second
  };
}

VenueHealthSnapshot
VenueHealthMonitor::snapshot(
    const std::string& venue,
    std::uint64_t now_ns
) const {
  std::lock_guard<std::mutex> lock(
      mutex_
  );

  const auto iterator =
      states_.find(venue);

  if (iterator == states_.end()) {
    return VenueHealthSnapshot{
        .venue = venue
    };
  }

  return build_snapshot(
      iterator->first,
      iterator->second,
      now_ns
  );
}

std::vector<VenueHealthSnapshot>
VenueHealthMonitor::snapshots(
    std::uint64_t now_ns
) const {
  std::lock_guard<std::mutex> lock(
      mutex_
  );

  std::vector<VenueHealthSnapshot>
      result;

  result.reserve(states_.size());

  for (const auto& [venue, state] :
       states_) {
    result.push_back(
        build_snapshot(
            venue,
            state,
            now_ns
        )
    );
  }

  std::sort(
      result.begin(),
      result.end(),
      [](
          const VenueHealthSnapshot& left,
          const VenueHealthSnapshot& right
      ) {
        return left.venue < right.venue;
      }
  );

  return result;
}

bool VenueHealthMonitor::routable(
    const std::string& venue,
    std::uint64_t now_ns
) const {
  const auto current =
      snapshot(
          venue,
          now_ns
      );

  return
      current.synchronized &&
      (
          current.status ==
              VenueHealthStatus::Healthy ||
          current.status ==
              VenueHealthStatus::Delayed
      );
}

}  // namespace minimatch
