#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace minimatch {

enum class MarketEventType {
  Quote,
  Trade
};

struct MarketReplayEvent {
  std::uint64_t timestamp_ns{0};
  MarketEventType type{MarketEventType::Quote};

  std::string symbol;
  std::string venue;

  double bid_price{0.0};
  double bid_quantity{0.0};
  double ask_price{0.0};
  double ask_quantity{0.0};

  double trade_price{0.0};
  double trade_quantity{0.0};
};

struct MarketReplayStats {
  std::size_t event_count{0};
  std::size_t quote_count{0};
  std::size_t trade_count{0};

  std::uint64_t first_timestamp_ns{0};
  std::uint64_t last_timestamp_ns{0};
};

class MarketReplay {
 public:
  MarketReplay() = default;

  explicit MarketReplay(
      std::vector<MarketReplayEvent> events
  );

  static MarketReplay from_csv(
      const std::string& path
  );

  [[nodiscard]] bool empty() const;
  [[nodiscard]] std::size_t size() const;
  [[nodiscard]] std::size_t position() const;
  [[nodiscard]] bool finished() const;

  [[nodiscard]] const MarketReplayStats& stats() const;

  [[nodiscard]] std::optional<MarketReplayEvent> peek() const;
  std::optional<MarketReplayEvent> next();

  void reset();

  bool seek_index(std::size_t index);

  bool seek_timestamp(
      std::uint64_t timestamp_ns
  );

  [[nodiscard]] const std::vector<MarketReplayEvent>&
  events() const;

 private:
  std::vector<MarketReplayEvent> events_;
  std::size_t position_{0};
  MarketReplayStats stats_;

  void normalize();
  void calculate_stats();
};

std::string to_string(MarketEventType type);

}  // namespace minimatch
