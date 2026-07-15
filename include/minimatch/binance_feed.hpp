#pragma once

#include "minimatch/binance_l2.hpp"
#include "minimatch/market_data.hpp"

#include <atomic>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace minimatch {

struct BinanceFeedConfig {
  std::string exchange_symbol{"BTCUSDT"};
  std::string normalized_symbol{"BTC-USD"};

  std::size_t snapshot_depth{5000};

  std::string websocket_host{
      "data-stream.binance.vision"
  };

  std::string websocket_port{"443"};

  std::string rest_host{
      "data-api.binance.vision"
  };

  std::string rest_port{"443"};
};

struct BinanceFeedStats {
  std::size_t connections{0};
  std::size_t snapshots{0};
  std::size_t received_messages{0};
  std::size_t accepted_updates{0};
  std::size_t ignored_updates{0};
  std::size_t rejected_updates{0};
  std::size_t reconnects{0};

  bool synchronized{false};
  std::uint64_t last_update_id{0};
};

using BinanceSnapshotHandler =
    std::function<void(
        const MarketDataSnapshot&
    )>;

using BinanceUpdateHandler =
    std::function<void(
        const std::vector<
            MarketDataUpdate
        >&
    )>;

using BinanceStatusHandler =
    std::function<void(
        const BinanceFeedStats&
    )>;

class BinanceFeed {
 public:
  explicit BinanceFeed(
      BinanceFeedConfig config
  );

  void run(
      std::atomic<bool>& running,
      const BinanceSnapshotHandler&
          on_snapshot,
      const BinanceUpdateHandler&
          on_updates,
      const BinanceStatusHandler&
          on_status = {}
  );

  [[nodiscard]] const BinanceFeedConfig&
  config() const;

 private:
  BinanceFeedConfig config_;
};

}  // namespace minimatch
