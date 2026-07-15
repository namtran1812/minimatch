#include "minimatch/kraken_l2.hpp"
#include <iterator>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <algorithm>
#include <cmath>
#include <zlib.h>
#include <map>
#include <iomanip>
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

std::vector<KrakenChecksumLevel>
parse_checksum_levels(
    const ptree& data,
    const std::string& field
) {
  std::vector<KrakenChecksumLevel> levels;

  const auto child =
      data.get_child_optional(field);

  if (!child.has_value()) {
    return levels;
  }

  for (const auto& entry : *child) {
    const auto& level = entry.second;

    const std::string price_text =
        level.get<std::string>("price");

    const std::string quantity_text =
        level.get<std::string>("qty");

    const double price =
        std::stod(price_text);

    const double quantity =
        std::stod(quantity_text);

    if (
        !std::isfinite(price) ||
        !std::isfinite(quantity) ||
        price <= 0.0 ||
        quantity < 0.0
    ) {
      throw std::invalid_argument(
          "invalid Kraken book level"
      );
    }

    levels.push_back(
        KrakenChecksumLevel{
            .price = price,
            .quantity = quantity,
            .price_text = price_text,
            .quantity_text = quantity_text
        }
    );
  }

  return levels;
}

std::vector<MarketDataLevel>
to_market_levels(
    const std::vector<
        KrakenChecksumLevel
    >& levels
) {
  std::vector<MarketDataLevel> result;

  result.reserve(levels.size());

  for (const auto& level : levels) {
    result.push_back(
        MarketDataLevel{
            .price = level.price,
            .quantity = level.quantity
        }
    );
  }

  return result;
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

    result.checksum_bids =
        parse_checksum_levels(
            data,
            "bids"
        );

    result.checksum_asks =
        parse_checksum_levels(
            data,
            "asks"
        );

    result.bids =
        to_market_levels(
            result.checksum_bids
        );

    result.asks =
        to_market_levels(
            result.checksum_asks
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
    std::string normalized_symbol,
    std::size_t depth
)
    : normalized_symbol_(
          std::move(normalized_symbol)
      ),
      depth_(depth) {
  if (normalized_symbol_.empty()) {
    throw std::invalid_argument(
        "Kraken normalized symbol cannot be empty"
    );
  }

  if (depth_ == 0) {
    throw std::invalid_argument(
        "Kraken depth must be positive"
    );
  }
}

void KrakenBookNormalizer::reset() {
  sequence_ = 0;
  synchronized_ = false;

  bids_.clear();
  asks_.clear();
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

  bids_.clear();
  asks_.clear();

  apply_levels(
      message.checksum_bids,
      MarketDataSide::Bid
  );

  apply_levels(
      message.checksum_asks,
      MarketDataSide::Ask
  );

  trim_to_depth();

  const std::uint64_t actual_checksum =
      calculate_checksum();

  synchronized_ =
      message.checksum == 0 ||
      actual_checksum ==
          message.checksum;

  if (!synchronized_) {
    throw std::runtime_error(
        "Kraken snapshot checksum mismatch expected=" +
        std::to_string(message.checksum) +
        " actual=" +
        std::to_string(actual_checksum)
    );
  }

  std::vector<MarketDataLevel> bids;
  std::vector<MarketDataLevel> asks;

  for (const auto& [price, level] :
       bids_) {
    bids.push_back(
        MarketDataLevel{
            .price = price,
            .quantity = level.quantity
        }
    );
  }

  for (const auto& [price, level] :
       asks_) {
    asks.push_back(
        MarketDataLevel{
            .price = price,
            .quantity = level.quantity
        }
    );
  }

  return MarketDataSnapshot{
      .venue = "KRAKEN",
      .symbol = normalized_symbol_,
      .sequence = sequence_,
      .timestamp_ns =
          message.received_timestamp_ns,
      .bids = std::move(bids),
      .asks = std::move(asks)
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

  /*
   * Preserve the previously published top-N book. Kraken's
   * depth-limited feed may evict a worst level without sending
   * an explicit zero-quantity deletion for that level.
   */
  const auto previous_bids = bids_;
  const auto previous_asks = asks_;

  apply_levels(
      message.checksum_bids,
      MarketDataSide::Bid
  );

  apply_levels(
      message.checksum_asks,
      MarketDataSide::Ask
  );

  trim_to_depth();

  const std::uint64_t actual_checksum =
      calculate_checksum();

  if (
      message.checksum != 0 &&
      actual_checksum != message.checksum
  ) {
    synchronized_ = false;

    throw std::runtime_error(
        "Kraken order-book checksum mismatch expected=" +
        std::to_string(message.checksum) +
        " actual=" +
        std::to_string(actual_checksum)
    );
  }

  ++sequence_;

  std::vector<MarketDataUpdate> updates;

  updates.reserve(
      previous_bids.size() +
      previous_asks.size() +
      bids_.size() +
      asks_.size()
  );

  const auto append_delete =
      [this, &updates, &message](
          MarketDataSide side,
          double price
      ) {
        updates.push_back(
            MarketDataUpdate{
                .venue = "KRAKEN",
                .symbol = normalized_symbol_,
                .sequence = sequence_,
                .timestamp_ns =
                    message.received_timestamp_ns,
                .side = side,
                .type =
                    MarketDataUpdateType::Delete,
                .price = price,
                .quantity = 0.0
            }
        );
      };

  const auto append_upsert =
      [this, &updates, &message](
          MarketDataSide side,
          double price,
          double quantity
      ) {
        updates.push_back(
            MarketDataUpdate{
                .venue = "KRAKEN",
                .symbol = normalized_symbol_,
                .sequence = sequence_,
                .timestamp_ns =
                    message.received_timestamp_ns,
                .side = side,
                .type =
                    MarketDataUpdateType::Upsert,
                .price = price,
                .quantity = quantity
            }
        );
      };

  /*
   * Delete levels that disappeared, including levels evicted
   * when the top-N book was trimmed.
   */
  for (const auto& [price, level] :
       previous_bids) {
    static_cast<void>(level);

    if (!bids_.contains(price)) {
      append_delete(
          MarketDataSide::Bid,
          price
      );
    }
  }

  for (const auto& [price, level] :
       previous_asks) {
    static_cast<void>(level);

    if (!asks_.contains(price)) {
      append_delete(
          MarketDataSide::Ask,
          price
      );
    }
  }

  /*
   * Publish new levels and quantity changes. Unchanged levels
   * do not need to be resent.
   */
  for (const auto& [price, level] : bids_) {
    const auto previous =
        previous_bids.find(price);

    if (
        previous == previous_bids.end() ||
        previous->second.quantity !=
            level.quantity
    ) {
      append_upsert(
          MarketDataSide::Bid,
          price,
          level.quantity
      );
    }
  }

  for (const auto& [price, level] : asks_) {
    const auto previous =
        previous_asks.find(price);

    if (
        previous == previous_asks.end() ||
        previous->second.quantity !=
            level.quantity
    ) {
      append_upsert(
          MarketDataSide::Ask,
          price,
          level.quantity
      );
    }
  }

  return updates;
}

void KrakenBookNormalizer::apply_levels(
    const std::vector<
        KrakenChecksumLevel
    >& levels,
    MarketDataSide side
) {
  for (const auto& level : levels) {
    if (side == MarketDataSide::Bid) {
      if (level.quantity == 0.0) {
        bids_.erase(level.price);
      } else {
        bids_[level.price] =
            StoredLevel{
                .quantity = level.quantity,
                .price_text = level.price_text,
                .quantity_text =
                    level.quantity_text
            };
      }
    } else {
      if (level.quantity == 0.0) {
        asks_.erase(level.price);
      } else {
        asks_[level.price] =
            StoredLevel{
                .quantity = level.quantity,
                .price_text = level.price_text,
                .quantity_text =
                    level.quantity_text
            };
      }
    }
  }
}

void KrakenBookNormalizer::trim_to_depth() {
  while (bids_.size() > depth_) {
    bids_.erase(
        std::prev(bids_.end())
    );
  }

  while (asks_.size() > depth_) {
    asks_.erase(
        std::prev(asks_.end())
    );
  }
}

std::uint64_t
KrakenBookNormalizer::calculate_checksum() const {
  std::string checksum_input;

  const auto append_decimal =
      [&checksum_input](
          std::string value
      ) {
        value.erase(
            std::remove(
                value.begin(),
                value.end(),
                '.'
            ),
            value.end()
        );

        const auto first_nonzero =
            value.find_first_not_of('0');

        if (
            first_nonzero ==
            std::string::npos
        ) {
          checksum_input += '0';
          return;
        }

        checksum_input +=
            value.substr(first_nonzero);
      };

  std::size_t count = 0;

  // Kraken checksum order is asks first, then bids.
  for (const auto& [price, level] :
       asks_) {
    static_cast<void>(price);

    if (count++ >= 10) {
      break;
    }

    append_decimal(level.price_text);
    append_decimal(level.quantity_text);
  }

  count = 0;

  for (const auto& [price, level] :
       bids_) {
    static_cast<void>(price);

    if (count++ >= 10) {
      break;
    }

    append_decimal(level.price_text);
    append_decimal(level.quantity_text);
  }

  return static_cast<std::uint64_t>(
      crc32(
          0L,
          reinterpret_cast<const Bytef*>(
              checksum_input.data()
          ),
          static_cast<uInt>(
              checksum_input.size()
          )
      )
  );
}

bool KrakenBookNormalizer::checksum_valid(
    std::uint64_t expected_checksum
) const {
  return
      expected_checksum == 0 ||
      calculate_checksum() ==
          expected_checksum;
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
