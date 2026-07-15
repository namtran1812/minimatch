#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace minimatch {

enum class VenueHealthStatus {
  Unknown,
  Healthy,
  Delayed,
  Stale,
  Disconnected
};

struct VenueHealthConfig {
  std::uint64_t delayed_after_ns{
      500'000'000ULL
  };

  std::uint64_t stale_after_ns{
      2'000'000'000ULL
  };

  std::uint64_t disconnected_after_ns{
      10'000'000'000ULL
  };

  std::uint64_t rate_window_ns{
      1'000'000'000ULL
  };
};

struct VenueHealthSnapshot {
  std::string venue;

  VenueHealthStatus status{
      VenueHealthStatus::Unknown
  };

  bool synchronized{false};

  std::uint64_t last_message_ns{0};
  std::uint64_t quote_age_ns{0};

  std::uint64_t message_count{0};
  std::uint64_t snapshot_count{0};
  std::uint64_t update_count{0};

  std::uint64_t reconnect_count{0};
  std::uint64_t rejected_count{0};
  std::uint64_t sequence_gap_count{0};
  std::uint64_t checksum_error_count{0};

  double messages_per_second{0.0};
};

class VenueHealthMonitor {
 public:
  explicit VenueHealthMonitor(
      VenueHealthConfig config = {}
  );

  void record_snapshot(
      const std::string& venue,
      std::uint64_t timestamp_ns
  );

  void record_update(
      const std::string& venue,
      std::uint64_t timestamp_ns
  );

  void record_reconnect(
      const std::string& venue
  );

  void record_rejection(
      const std::string& venue
  );

  void record_sequence_gap(
      const std::string& venue
  );

  void record_checksum_error(
      const std::string& venue
  );

  void set_synchronized(
      const std::string& venue,
      bool synchronized
  );

  [[nodiscard]]
  VenueHealthSnapshot snapshot(
      const std::string& venue,
      std::uint64_t now_ns
  ) const;

  [[nodiscard]]
  std::vector<VenueHealthSnapshot>
  snapshots(
      std::uint64_t now_ns
  ) const;

  [[nodiscard]]
  bool routable(
      const std::string& venue,
      std::uint64_t now_ns
  ) const;

 private:
  struct VenueState {
    bool synchronized{false};

    std::uint64_t first_window_message_ns{0};
    std::uint64_t window_message_count{0};

    std::uint64_t last_message_ns{0};

    std::uint64_t message_count{0};
    std::uint64_t snapshot_count{0};
    std::uint64_t update_count{0};

    std::uint64_t reconnect_count{0};
    std::uint64_t rejected_count{0};
    std::uint64_t sequence_gap_count{0};
    std::uint64_t checksum_error_count{0};
  };

  void record_message(
      const std::string& venue,
      std::uint64_t timestamp_ns,
      bool snapshot
  );

  [[nodiscard]]
  VenueHealthSnapshot build_snapshot(
      const std::string& venue,
      const VenueState& state,
      std::uint64_t now_ns
  ) const;

  VenueHealthConfig config_;

  mutable std::mutex mutex_;

  std::unordered_map<
      std::string,
      VenueState
  > states_;
};

[[nodiscard]]
std::string to_string(
    VenueHealthStatus status
);

}  // namespace minimatch
