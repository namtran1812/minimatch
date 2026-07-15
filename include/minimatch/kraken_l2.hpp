#pragma once

#include "minimatch/market_data.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace minimatch {

struct KrakenBookMessage {
  bool valid{false};
  bool ignored{false};
  bool snapshot{false};

  std::string symbol;
  std::string exchange_timestamp;

  std::uint64_t received_timestamp_ns{0};
  std::uint64_t checksum{0};

  std::vector<MarketDataLevel> bids;
  std::vector<MarketDataLevel> asks;

  std::string error;
};

[[nodiscard]] KrakenBookMessage
parse_kraken_book_message(
    const std::string& json,
    const std::string& expected_symbol,
    std::uint64_t received_timestamp_ns
);

[[nodiscard]] std::string
build_kraken_subscription(
    const std::string& symbol,
    std::size_t depth
);

class KrakenBookNormalizer {
 public:
  explicit KrakenBookNormalizer(
      std::string normalized_symbol
  );

  void reset();

  [[nodiscard]] MarketDataSnapshot
  normalize_snapshot(
      const KrakenBookMessage& message
  );

  [[nodiscard]] std::vector<MarketDataUpdate>
  normalize_update(
      const KrakenBookMessage& message
  );

  [[nodiscard]] bool synchronized() const;

  [[nodiscard]] std::uint64_t
  sequence() const;

 private:
  std::string normalized_symbol_;
  std::uint64_t sequence_{0};
  bool synchronized_{false};
};

}  // namespace minimatch
