#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace minimatch {

struct LatencySnapshot {
  std::string name;

  std::uint64_t count{0};
  std::uint64_t minimum_ns{0};
  std::uint64_t maximum_ns{0};

  double average_ns{0.0};

  std::uint64_t p50_ns{0};
  std::uint64_t p95_ns{0};
  std::uint64_t p99_ns{0};
};

class LatencyStats {
 public:
  explicit LatencyStats(
      std::size_t maximum_samples = 4096
  );

  void record(
      const std::string& name,
      std::uint64_t latency_ns
  );

  [[nodiscard]]
  LatencySnapshot snapshot(
      const std::string& name
  ) const;

  [[nodiscard]]
  std::vector<LatencySnapshot>
  snapshots() const;

  void reset(
      const std::string& name
  );

  void reset_all();

 private:
  struct Series {
    std::uint64_t count{0};
    std::uint64_t total_ns{0};
    std::uint64_t minimum_ns{0};
    std::uint64_t maximum_ns{0};

    std::size_t next_index{0};

    std::vector<std::uint64_t> samples;
  };

  [[nodiscard]]
  LatencySnapshot build_snapshot(
      const std::string& name,
      const Series& series
  ) const;

  std::size_t maximum_samples_;

  mutable std::mutex mutex_;

  std::unordered_map<
      std::string,
      Series
  > series_;
};

class ScopedLatencyRecorder {
 public:
  ScopedLatencyRecorder(
      LatencyStats& stats,
      std::string name
  );

  ~ScopedLatencyRecorder();

  ScopedLatencyRecorder(
      const ScopedLatencyRecorder&
  ) = delete;

  ScopedLatencyRecorder& operator=(
      const ScopedLatencyRecorder&
  ) = delete;

 private:
  LatencyStats& stats_;
  std::string name_;
  std::uint64_t start_ns_{0};
};

[[nodiscard]]
std::uint64_t steady_now_ns();

}  // namespace minimatch
