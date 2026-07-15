#include "minimatch/latency_stats.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <utility>

namespace minimatch {

namespace {

std::uint64_t percentile(
    const std::vector<std::uint64_t>& sorted,
    double quantile
) {
  if (sorted.empty()) {
    return 0;
  }

  const double raw_index =
      quantile *
      static_cast<double>(
          sorted.size() - 1
      );

  const auto index =
      static_cast<std::size_t>(
          raw_index
      );

  return sorted[index];
}

}  // namespace

std::uint64_t steady_now_ns() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<
          std::chrono::nanoseconds
      >(
          std::chrono::steady_clock::
              now().time_since_epoch()
      ).count()
  );
}

LatencyStats::LatencyStats(
    std::size_t maximum_samples
)
    : maximum_samples_(
          maximum_samples
      ) {
  if (maximum_samples_ == 0) {
    throw std::invalid_argument(
        "maximum latency samples must be positive"
    );
  }
}

void LatencyStats::record(
    const std::string& name,
    std::uint64_t latency_ns
) {
  if (name.empty()) {
    throw std::invalid_argument(
        "latency series name cannot be empty"
    );
  }

  std::lock_guard<std::mutex> lock(
      mutex_
  );

  auto& series = series_[name];

  ++series.count;
  series.total_ns += latency_ns;

  if (
      series.count == 1 ||
      latency_ns < series.minimum_ns
  ) {
    series.minimum_ns = latency_ns;
  }

  series.maximum_ns =
      std::max(
          series.maximum_ns,
          latency_ns
      );

  if (
      series.samples.size() <
      maximum_samples_
  ) {
    series.samples.push_back(
        latency_ns
    );
  } else {
    series.samples[
        series.next_index
    ] = latency_ns;

    series.next_index =
        (
            series.next_index + 1
        ) %
        maximum_samples_;
  }
}

LatencySnapshot
LatencyStats::build_snapshot(
    const std::string& name,
    const Series& series
) const {
  std::vector<std::uint64_t> sorted =
      series.samples;

  std::sort(
      sorted.begin(),
      sorted.end()
  );

  return LatencySnapshot{
      .name = name,
      .count = series.count,
      .minimum_ns = series.minimum_ns,
      .maximum_ns = series.maximum_ns,
      .average_ns =
          series.count == 0
              ? 0.0
              : static_cast<double>(
                    series.total_ns
                ) /
                    static_cast<double>(
                        series.count
                    ),
      .p50_ns = percentile(
          sorted,
          0.50
      ),
      .p95_ns = percentile(
          sorted,
          0.95
      ),
      .p99_ns = percentile(
          sorted,
          0.99
      )
  };
}

LatencySnapshot LatencyStats::snapshot(
    const std::string& name
) const {
  std::lock_guard<std::mutex> lock(
      mutex_
  );

  const auto iterator =
      series_.find(name);

  if (iterator == series_.end()) {
    return LatencySnapshot{
        .name = name
    };
  }

  return build_snapshot(
      iterator->first,
      iterator->second
  );
}

std::vector<LatencySnapshot>
LatencyStats::snapshots() const {
  std::lock_guard<std::mutex> lock(
      mutex_
  );

  std::vector<LatencySnapshot> result;

  result.reserve(series_.size());

  for (const auto& [name, series] :
       series_) {
    result.push_back(
        build_snapshot(
            name,
            series
        )
    );
  }

  std::sort(
      result.begin(),
      result.end(),
      [](
          const LatencySnapshot& left,
          const LatencySnapshot& right
      ) {
        return left.name < right.name;
      }
  );

  return result;
}

void LatencyStats::reset(
    const std::string& name
) {
  std::lock_guard<std::mutex> lock(
      mutex_
  );

  series_.erase(name);
}

void LatencyStats::reset_all() {
  std::lock_guard<std::mutex> lock(
      mutex_
  );

  series_.clear();
}

ScopedLatencyRecorder::
ScopedLatencyRecorder(
    LatencyStats& stats,
    std::string name
)
    : stats_(stats),
      name_(std::move(name)),
      start_ns_(steady_now_ns()) {
}

ScopedLatencyRecorder::
~ScopedLatencyRecorder() {
  const auto end_ns =
      steady_now_ns();

  stats_.record(
      name_,
      end_ns >= start_ns_
          ? end_ns - start_ns_
          : 0
  );
}

}  // namespace minimatch
