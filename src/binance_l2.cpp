#include "minimatch/binance_l2.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <algorithm>
#include <cmath>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace minimatch {

namespace {

using boost::property_tree::ptree;

std::string uppercase(
    std::string value
) {
  std::transform(
      value.begin(),
      value.end(),
      value.begin(),
      [](unsigned char character) {
        return static_cast<char>(
            std::toupper(character)
        );
      }
  );

  return value;
}

double parse_number(
    const std::string& value,
    bool allow_zero
) {
  std::size_t consumed = 0;

  const double parsed =
      std::stod(value, &consumed);

  if (
      consumed != value.size() ||
      !std::isfinite(parsed) ||
      parsed < 0.0 ||
      (!allow_zero && parsed <= 0.0)
  ) {
    throw std::invalid_argument(
        "invalid Binance numeric value"
    );
  }

  return parsed;
}

std::vector<MarketDataLevel>
parse_levels(
    const ptree& levels
) {
  std::vector<MarketDataLevel> result;

  for (const auto& level_entry : levels) {
    std::vector<std::string> values;

    for (const auto& value_entry :
         level_entry.second) {
      values.push_back(
          value_entry.second.get_value<
              std::string
          >()
      );
    }

    if (values.size() < 2) {
      throw std::invalid_argument(
          "Binance depth level is incomplete"
      );
    }

    result.push_back(
        MarketDataLevel{
            .price =
                parse_number(
                    values[0],
                    false
                ),
            .quantity =
                parse_number(
                    values[1],
                    true
                )
        }
    );
  }

  return result;
}

void append_updates(
    std::vector<MarketDataUpdate>& output,
    const std::vector<MarketDataLevel>& levels,
    const std::string& symbol,
    std::uint64_t sequence,
    std::uint64_t timestamp_ns,
    MarketDataSide side
) {
  for (const auto& level : levels) {
    output.push_back(
        MarketDataUpdate{
            .venue = "BINANCE",
            .symbol = symbol,
            .sequence = sequence,
            .timestamp_ns = timestamp_ns,
            .side = side,
            .type =
                level.quantity <= 0.0
                    ? MarketDataUpdateType::Delete
                    : MarketDataUpdateType::Upsert,
            .price = level.price,
            .quantity = level.quantity
        }
    );
  }
}

}  // namespace

BinanceDepthSnapshot
parse_binance_depth_snapshot(
    const std::string& json,
    const std::string& symbol,
    std::uint64_t received_timestamp_ns
) {
  BinanceDepthSnapshot result;

  result.symbol = uppercase(symbol);
  result.received_timestamp_ns =
      received_timestamp_ns;

  try {
    std::istringstream input(json);
    ptree root;

    boost::property_tree::read_json(
        input,
        root
    );

    result.last_update_id =
        root.get<std::uint64_t>(
            "lastUpdateId"
        );

    const auto bids =
        root.get_child_optional("bids");

    const auto asks =
        root.get_child_optional("asks");

    if (!bids.has_value() ||
        !asks.has_value()) {
      result.error =
          "Binance snapshot is missing depth";

      return result;
    }

    result.bids = parse_levels(*bids);
    result.asks = parse_levels(*asks);

    result.valid = true;
    return result;
  } catch (const std::exception& error) {
    result.error = error.what();
    return result;
  }
}

BinanceDepthUpdate
parse_binance_depth_update(
    const std::string& json,
    const std::string& symbol,
    std::uint64_t received_timestamp_ns
) {
  BinanceDepthUpdate result;

  result.symbol = uppercase(symbol);
  result.received_timestamp_ns =
      received_timestamp_ns;

  try {
    std::istringstream input(json);
    ptree root;

    boost::property_tree::read_json(
        input,
        root
    );

    const std::string event_type =
        root.get<std::string>(
            "e",
            ""
        );

    if (event_type.empty()) {
      result.valid = true;
      result.ignored = true;
      return result;
    }

    if (event_type != "depthUpdate") {
      result.valid = true;
      result.ignored = true;
      return result;
    }

    const std::string message_symbol =
        uppercase(
            root.get<std::string>("s")
        );

    if (message_symbol !=
        result.symbol) {
      result.error =
          "Binance message symbol mismatch";

      return result;
    }

    result.event_timestamp_ms =
        root.get<std::uint64_t>("E");

    result.first_update_id =
        root.get<std::uint64_t>("U");

    result.final_update_id =
        root.get<std::uint64_t>("u");

    const auto bids =
        root.get_child_optional("b");

    const auto asks =
        root.get_child_optional("a");

    if (!bids.has_value() ||
        !asks.has_value()) {
      result.error =
          "Binance update is missing depth arrays";

      return result;
    }

    result.bids = parse_levels(*bids);
    result.asks = parse_levels(*asks);

    result.valid = true;
    return result;
  } catch (const std::exception& error) {
    result.error = error.what();
    return result;
  }
}

BinanceBookSynchronizer::
BinanceBookSynchronizer(
    std::string normalized_symbol
)
    : symbol_(
          std::move(normalized_symbol)
      ) {
  if (symbol_.empty()) {
    throw std::invalid_argument(
        "Binance normalized symbol cannot be empty"
    );
  }
}

void BinanceBookSynchronizer::reset() {
  status_ =
      BinanceSyncStatus::WaitingForSnapshot;

  last_update_id_ = 0;
  normalized_sequence_ = 0;
}

BinanceSyncResult
BinanceBookSynchronizer::apply_snapshot(
    const BinanceDepthSnapshot& snapshot
) {
  if (!snapshot.valid) {
    return BinanceSyncResult{
        .accepted = false,
        .snapshot_required = true,
        .status = status_,
        .message =
            "invalid Binance snapshot"
    };
  }

  last_update_id_ =
      snapshot.last_update_id;

  normalized_sequence_ = 0;

  status_ =
      BinanceSyncStatus::Synchronizing;

  return BinanceSyncResult{
      .accepted = true,
      .status = status_,
      .message =
          "Binance snapshot accepted"
  };
}

BinanceSyncResult
BinanceBookSynchronizer::apply_update(
    const BinanceDepthUpdate& update
) {
  if (!update.valid) {
    status_ = BinanceSyncStatus::Stale;

    return BinanceSyncResult{
        .accepted = false,
        .snapshot_required = true,
        .status = status_,
        .message =
            "invalid Binance depth update"
    };
  }

  if (update.ignored) {
    return BinanceSyncResult{
        .accepted = true,
        .ignored = true,
        .status = status_,
        .message =
            "ignored non-depth Binance message"
    };
  }

  if (
      status_ ==
      BinanceSyncStatus::WaitingForSnapshot
  ) {
    return BinanceSyncResult{
        .accepted = false,
        .snapshot_required = true,
        .status = status_,
        .message =
            "Binance snapshot required"
    };
  }

  if (
      update.final_update_id <=
      last_update_id_
  ) {
    return BinanceSyncResult{
        .accepted = true,
        .ignored = true,
        .status = status_,
        .message =
            "ignored stale Binance update"
    };
  }

  if (
      status_ ==
      BinanceSyncStatus::Synchronizing
  ) {
    const std::uint64_t expected =
        last_update_id_ + 1;

    if (
        update.first_update_id >
            expected ||
        update.final_update_id <
            expected
    ) {
      status_ =
          BinanceSyncStatus::Stale;

      return BinanceSyncResult{
          .accepted = false,
          .snapshot_required = true,
          .status = status_,
          .message =
              "Binance update does not bridge snapshot"
      };
    }

    status_ =
        BinanceSyncStatus::Synchronized;
  } else {
    const std::uint64_t expected =
        last_update_id_ + 1;

    if (
        update.first_update_id >
        expected
    ) {
      status_ =
          BinanceSyncStatus::Stale;

      return BinanceSyncResult{
          .accepted = false,
          .snapshot_required = true,
          .status = status_,
          .message =
              "Binance sequence gap detected"
      };
    }
  }

  ++normalized_sequence_;

  std::vector<MarketDataUpdate> updates;

  updates.reserve(
      update.bids.size() +
      update.asks.size()
  );

  append_updates(
      updates,
      update.bids,
      symbol_,
      normalized_sequence_,
      update.received_timestamp_ns,
      MarketDataSide::Bid
  );

  append_updates(
      updates,
      update.asks,
      symbol_,
      normalized_sequence_,
      update.received_timestamp_ns,
      MarketDataSide::Ask
  );

  last_update_id_ =
      update.final_update_id;

  return BinanceSyncResult{
      .accepted = true,
      .status = status_,
      .updates = std::move(updates),
      .message =
          "Binance depth update accepted"
  };
}

bool BinanceBookSynchronizer::
synchronized() const {
  return status_ ==
      BinanceSyncStatus::Synchronized;
}

std::uint64_t
BinanceBookSynchronizer::
last_update_id() const {
  return last_update_id_;
}

}  // namespace minimatch
