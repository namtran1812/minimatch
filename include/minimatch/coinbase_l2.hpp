#pragma once

#include "minimatch/market_data.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace minimatch {

struct CoinbaseLevel2Message {
  bool valid{false};
  bool snapshot{false};
  bool heartbeat{false};

  std::string channel;
  std::string product_id;

  std::uint64_t sequence{0};
  std::uint64_t received_timestamp_ns{0};

  MarketDataSnapshot book_snapshot;
  std::vector<MarketDataUpdate> updates;

  std::string error;
};

[[nodiscard]] CoinbaseLevel2Message
parse_coinbase_level2_message(
    const std::string& json,
    std::uint64_t received_timestamp_ns
);

[[nodiscard]] std::string
coinbase_level2_subscription(
    const std::string& product_id
);

[[nodiscard]] std::string
coinbase_heartbeat_subscription();

}  // namespace minimatch
