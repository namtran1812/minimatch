#pragma once

#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace minimatch {

struct MidpointObservation {
  std::uint64_t timestamp_ns{0};
  double midpoint{0.0};
};

class MidpointHistory {
 public:
  explicit MidpointHistory(
      std::uint64_t retention_ns =
          300'000'000'000ULL
  );

  void record(
      const std::string& symbol,
      std::uint64_t timestamp_ns,
      double midpoint
  );

  [[nodiscard]]
  std::optional<MidpointObservation>
  at_or_after(
      const std::string& symbol,
      std::uint64_t timestamp_ns
  ) const;

  [[nodiscard]]
  std::optional<MidpointObservation>
  at_or_before(
      const std::string& symbol,
      std::uint64_t timestamp_ns
  ) const;

  [[nodiscard]]
  std::size_t size(
      const std::string& symbol
  ) const;

  [[nodiscard]]
  std::optional<MidpointObservation>
  first(
      const std::string& symbol
  ) const;

  [[nodiscard]]
  std::optional<MidpointObservation>
  last(
      const std::string& symbol
  ) const;

 private:
  std::uint64_t retention_ns_;

  mutable std::mutex mutex_;

  std::unordered_map<
      std::string,
      std::deque<MidpointObservation>
  > observations_;
};

}  // namespace minimatch
