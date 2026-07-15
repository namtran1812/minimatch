#include "minimatch/kraken_l2.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace minimatch {

namespace {

using boost::property_tree::ptree;

double parse_nonnegative_number(
    const ptree& node,
    const std::string& field
) {
  const double value =
      node.get<double>(field);

  if (!std::isfinite(value) ||
      value < 0.0) {
    throw std::invalid_argument(
        "invalid Kraken numeric field: " +
        field
    );
  }

  return value;
}

std::vector<MarketDataLevel>
parse_levels(
    const ptree& data,
    const std::string& field
) {
  std::vector<MarketDataLevel> levels;

  const auto child =
      data.get_child_optional(field);

  if (!child.has_value()) {
    return levels;
  }

  for (const auto& entry : *child) {
    const auto& level = entry.second;

    const double price =
        parse_nonnegative_number(
            level,
            "price"
        );

    const double quantity =
        parse_nonnegative_number(
            level,
            "qty"
        );

    if (price <= 0.0) {
      throw std::invalid_argument(
          "Kraken price must be positive"
      );
    }

    levels.push_back(
        MarketDataLevel{
            .price = price,
            .quantity = quantity
        }
    );
  }

  return levels;
}

void append_normalized_updates(
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
            .venue = "KRAKEN",
            .symbol = symbol,
            .sequence = sequence,
            .timestamp_ns = timestamp_ns,
            .side = side,
            .type =
                level.quantity == 0.0
                    ? MarketDataUpdateType::Delete
                    : MarketDataUpdateType::Upsert,
            .price = level.price,
            .quantity = level.quantity
        }
    );
  }
}

}  // namespace

KrakenBookMessage
parse_kraken_book_message(
    const std::string& json,
    const std::string& expected_symbol,
    std::uint64_t received_timestamp_ns
) {
  KrakenBookMessage result;
  result.received_timestamp_ns =
      received_timestamp_ns;

  try {
    std::istringstream input(json);
    ptree root;

    boost::property_tree::read_json(
        input,
        root
    );

    const std::string channel =
        root.get<std::string>(
            "channel",
            ""
        );

    const std::string type =
        root.get<std::string>(
            "type",
            ""
        );

    if (channel != "book") {
      result.valid = true;
      result.ignored = true;
      return result;
    }

    if (
        type != "snapshot" &&
        type != "update"
    ) {
      result.valid = true;
      result.ignored = true;
      return result;
    }

    const auto data_array =
        root.get_child_optional("data");

    if (!data_array.has_value() ||
        data_array->empty()) {
      result.error =
          "Kraken book message has no data";

      return result;
    }

    const auto& data =
        data_array->front().second;

    result.symbol =
        data.get<std::string>(
            "symbol"
        );

    if (
        !expected_symbol.empty() &&
        result.symbol != expected_symbol
    ) {
      result.error =
          "Kraken symbol mismatch";

      return result;
    }

    result.snapshot =
        type == "snapshot";

    result.exchange_timestamp =
        data.get<std::string>(
            "timestamp",
            ""
        );

    result.checksum =
        data.get<std::uint64_t>(
            "checksum",
            0
        );

    result.bids =
        parse_levels(
            data,
            "bids"
        );

    result.asks =
        parse_levels(
            data,
            "asks"
        );

    result.valid = true;
    return result;
  } catch (const std::exception& error) {
    result.error = error.what();
    return result;
  }
}

std::string build_kraken_subscription(
    const std::string& symbol,
    std::size_t depth
) {
  if (symbol.empty()) {
    throw std::invalid_argument(
        "Kraken symbol cannot be empty"
    );
  }

  if (
      depth != 10 &&
      depth != 25 &&
      depth != 100 &&
      depth != 500 &&
      depth != 1000
  ) {
    throw std::invalid_argument(
        "unsupported Kraken book depth"
    );
  }

  std::ostringstream output;

  output
      << '{'
      << "\"method\":\"subscribe\","
      << "\"params\":{"
      << "\"channel\":\"book\","
      << "\"symbol\":[\""
      << symbol
      << "\"],"
      << "\"depth\":"
      << depth
      << ",\"snapshot\":true"
      << "}}";

  return output.str();
}

KrakenBookNormalizer::
KrakenBookNormalizer(
    std::string normalized_symbol
)
    : normalized_symbol_(
          std::move(normalized_symbol)
      ) {
  if (normalized_symbol_.empty()) {
    throw std::invalid_argument(
        "Kraken normalized symbol cannot be empty"
    );
  }
}

void KrakenBookNormalizer::reset() {
  sequence_ = 0;
  synchronized_ = false;
}

MarketDataSnapshot
KrakenBookNormalizer::normalize_snapshot(
    const KrakenBookMessage& message
) {
  if (!message.valid ||
      message.ignored ||
      !message.snapshot) {
    throw std::invalid_argument(
        "valid Kraken snapshot required"
    );
  }

  sequence_ = 0;
  synchronized_ = true;

  return MarketDataSnapshot{
      .venue = "KRAKEN",
      .symbol = normalized_symbol_,
      .sequence = sequence_,
      .timestamp_ns =
          message.received_timestamp_ns,
      .bids = message.bids,
      .asks = message.asks
  };
}

std::vector<MarketDataUpdate>
KrakenBookNormalizer::normalize_update(
    const KrakenBookMessage& message
) {
  if (!message.valid ||
      message.ignored ||
      message.snapshot) {
    throw std::invalid_argument(
        "valid Kraken update required"
    );
  }

  if (!synchronized_) {
    throw std::runtime_error(
        "Kraken snapshot required before updates"
    );
  }

  ++sequence_;

  std::vector<MarketDataUpdate> updates;

  updates.reserve(
      message.bids.size() +
      message.asks.size()
  );

  append_normalized_updates(
      updates,
      message.bids,
      normalized_symbol_,
      sequence_,
      message.received_timestamp_ns,
      MarketDataSide::Bid
  );

  append_normalized_updates(
      updates,
      message.asks,
      normalized_symbol_,
      sequence_,
      message.received_timestamp_ns,
      MarketDataSide::Ask
  );

  return updates;
}

bool KrakenBookNormalizer::
synchronized() const {
  return synchronized_;
}

std::uint64_t
KrakenBookNormalizer::sequence() const {
  return sequence_;
}

}  // namespace minimatch
