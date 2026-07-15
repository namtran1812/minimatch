#pragma once

#include "minimatch/market_data.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace minimatch {

struct BinanceDepthSnapshot {
  bool valid{false};

  std::string symbol;
  std::uint64_t last_update_id{0};
  std::uint64_t received_timestamp_ns{0};

  std::vector<MarketDataLevel> bids;
  std::vector<MarketDataLevel> asks;

  std::string error;
};

struct BinanceDepthUpdate {
  bool valid{false};
  bool ignored{false};

  std::string symbol;

  std::uint64_t event_timestamp_ms{0};
  std::uint64_t first_update_id{0};
  std::uint64_t final_update_id{0};

  std::uint64_t received_timestamp_ns{0};

  std::vector<MarketDataLevel> bids;
  std::vector<MarketDataLevel> asks;

  std::string error;
};

enum class BinanceSyncStatus {
  WaitingForSnapshot,
  Synchronizing,
  Synchronized,
  Stale
};

struct BinanceSyncResult {
  bool accepted{false};
  bool ignored{false};
  bool snapshot_required{false};

  BinanceSyncStatus status{
      BinanceSyncStatus::WaitingForSnapshot
  };

  std::vector<MarketDataUpdate> updates;

  std::string message;
};

[[nodiscard]] BinanceDepthSnapshot
parse_binance_depth_snapshot(
    const std::string& json,
    const std::string& symbol,
    std::uint64_t received_timestamp_ns
);

[[nodiscard]] BinanceDepthUpdate
parse_binance_depth_update(
    const std::string& json,
    const std::string& symbol,
    std::uint64_t received_timestamp_ns
);

class BinanceBookSynchronizer {
 public:
  explicit BinanceBookSynchronizer(
      std::string normalized_symbol
  );

  void reset();

  [[nodiscard]] BinanceSyncResult
  apply_snapshot(
      const BinanceDepthSnapshot& snapshot
  );

  [[nodiscard]] BinanceSyncResult
  apply_update(
      const BinanceDepthUpdate& update
  );

  [[nodiscard]] bool synchronized() const;

  [[nodiscard]] std::uint64_t
  last_update_id() const;

 private:
  std::string symbol_;

  BinanceSyncStatus status_{
      BinanceSyncStatus::WaitingForSnapshot
  };

  std::uint64_t last_update_id_{0};
  std::uint64_t normalized_sequence_{0};
};

}  // namespace minimatch
