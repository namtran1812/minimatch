#pragma once

#include "minimatch/kraken_l2.hpp"
#include "minimatch/market_data.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace minimatch {

struct KrakenFeedConfig {
  std::string exchange_symbol{"BTC/USD"};
  std::string normalized_symbol{"BTC-USD"};

  std::size_t depth{10};

  std::string websocket_host{
      "ws.kraken.com"
  };

  std::string websocket_port{"443"};
  std::string websocket_target{"/v2"};

  std::chrono::seconds reconnect_delay{
      std::chrono::seconds(3)
  };
};

struct KrakenFeedStats {
  std::size_t connections{0};
  std::size_t snapshots{0};

  std::size_t received_messages{0};
  std::size_t accepted_updates{0};
  std::size_t ignored_messages{0};
  std::size_t rejected_messages{0};

  std::size_t reconnects{0};

  bool synchronized{false};

  std::uint64_t local_sequence{0};
  std::uint64_t latest_checksum{0};
};

using KrakenSnapshotHandler =
    std::function<void(
        const MarketDataSnapshot&
    )>;

using KrakenUpdateHandler =
    std::function<void(
        const std::vector<
            MarketDataUpdate
        >&
    )>;

using KrakenStatusHandler =
    std::function<void(
        const KrakenFeedStats&
    )>;

class KrakenFeed {
 public:
  explicit KrakenFeed(
      KrakenFeedConfig config = {}
  );

  void run(
      std::atomic<bool>& running,
      const KrakenSnapshotHandler&
          on_snapshot,
      const KrakenUpdateHandler&
          on_updates,
      const KrakenStatusHandler&
          on_status = {}
  );

  [[nodiscard]]
  const KrakenFeedConfig& config() const;

 private:
  KrakenFeedConfig config_;
};

}  // namespace minimatch
