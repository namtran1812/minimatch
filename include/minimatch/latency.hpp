#pragma once
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace minimatch {
class LatencyRecorder {
 public:
  explicit LatencyRecorder(std::size_t reserve = 0) { samples_.reserve(reserve); }
  void record(std::uint64_t nanoseconds) { samples_.push_back(nanoseconds); }
  [[nodiscard]] std::size_t size() const noexcept { return samples_.size(); }
  void sort() { std::sort(samples_.begin(), samples_.end()); }
  [[nodiscard]] std::uint64_t percentile(double p) const {
    if (samples_.empty()) throw std::runtime_error("no latency samples");
    const auto index = static_cast<std::size_t>(p * static_cast<double>(samples_.size() - 1));
    return samples_[std::min(index, samples_.size() - 1)];
  }
  [[nodiscard]] std::uint64_t min() const { return percentile(0.0); }
  [[nodiscard]] std::uint64_t max() const { return percentile(1.0); }
 private:
  std::vector<std::uint64_t> samples_;
};
}  // namespace minimatch
