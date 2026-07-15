#pragma once

#include "minimatch/market_data.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace minimatch {

struct KrakenChecksumLevel {
  double price{0.0};
  double quantity{0.0};

  std::string price_text;
  std::string quantity_text;
};

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

  std::vector<KrakenChecksumLevel>
      checksum_bids;

  std::vector<KrakenChecksumLevel>
      checksum_asks;

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
  KrakenBookNormalizer(
      std::string normalized_symbol,
      std::size_t depth = 10
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

  [[nodiscard]] bool
  checksum_valid(
      std::uint64_t expected_checksum
  ) const;

 private:
  void apply_levels(
      const std::vector<KrakenChecksumLevel>&
          levels,
      MarketDataSide side
  );

  void trim_to_depth();

  [[nodiscard]] std::uint64_t
  calculate_checksum() const;

  std::string normalized_symbol_;
  std::size_t depth_{10};

  std::uint64_t sequence_{0};
  bool synchronized_{false};

  struct StoredLevel {
    double quantity{0.0};

    std::string price_text;
    std::string quantity_text;
  };

  std::map<
      double,
      StoredLevel,
      std::greater<double>
  > bids_;

  std::map<
      double,
      StoredLevel
  > asks_;
};

}  // namespace minimatch
