#include "minimatch/coinbase_l2.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <cmath>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace minimatch {

namespace {

using boost::property_tree::ptree;

double parse_positive_double(
    const std::string& value,
    const char* field_name,
    bool allow_zero
) {
  std::size_t consumed = 0;

  const double parsed =
      std::stod(value, &consumed);

  if (consumed != value.size() ||
      !std::isfinite(parsed) ||
      parsed < 0.0 ||
      (!allow_zero && parsed <= 0.0)) {
    throw std::invalid_argument(
        std::string("invalid Coinbase ") +
        field_name
    );
  }

  return parsed;
}

MarketDataSide parse_side(
    const std::string& side
) {
  if (side == "bid") {
    return MarketDataSide::Bid;
  }

  if (side == "offer" ||
      side == "ask") {
    return MarketDataSide::Ask;
  }

  throw std::invalid_argument(
      "unsupported Coinbase Level-2 side"
  );
}

std::string escape_json(
    const std::string& value
) {
  std::ostringstream output;

  for (char character : value) {
    switch (character) {
      case '"':
        output << "\\\"";
        break;

      case '\\':
        output << "\\\\";
        break;

      case '\n':
        output << "\\n";
        break;

      case '\r':
        output << "\\r";
        break;

      case '\t':
        output << "\\t";
        break;

      default:
        output << character;
        break;
    }
  }

  return output.str();
}

}  // namespace

CoinbaseLevel2Message
parse_coinbase_level2_message(
    const std::string& json,
    std::uint64_t received_timestamp_ns
) {
  CoinbaseLevel2Message result;

  result.received_timestamp_ns =
      received_timestamp_ns;

  try {
    std::istringstream input(json);
    ptree root;

    boost::property_tree::read_json(
        input,
        root
    );

    result.channel =
        root.get<std::string>(
            "channel",
            ""
        );

    result.sequence =
        root.get<std::uint64_t>(
            "sequence_num",
            0
        );

    if (result.channel == "heartbeats") {
      result.valid = true;
      result.heartbeat = true;
      return result;
    }

    if (result.channel != "l2_data") {
      result.valid = true;
      result.ignored = true;
      return result;
    }

    const auto events =
        root.get_child_optional("events");

    if (!events.has_value() ||
        events->empty()) {
      result.error =
          "Coinbase message has no events";

      return result;
    }

    bool initialized = false;

    for (const auto& event_entry :
         *events) {
      const ptree& event =
          event_entry.second;

      const std::string event_type =
          event.get<std::string>(
              "type",
              ""
          );

      const std::string product_id =
          event.get<std::string>(
              "product_id",
              ""
          );

      if (event_type != "snapshot" &&
          event_type != "update") {
        continue;
      }

      if (product_id.empty()) {
        throw std::invalid_argument(
            "Coinbase event is missing product_id"
        );
      }

      if (!initialized) {
        result.snapshot =
            event_type == "snapshot";

        result.product_id =
            product_id;

        result.book_snapshot =
            MarketDataSnapshot{
                .venue = "COINBASE",
                .symbol = product_id,
                .sequence = result.sequence,
                .timestamp_ns =
                    received_timestamp_ns
            };

        initialized = true;
      }

      if (product_id !=
          result.product_id) {
        throw std::invalid_argument(
            "multiple products in one normalized message"
        );
      }

      const auto updates =
          event.get_child_optional(
              "updates"
          );

      if (!updates.has_value()) {
        continue;
      }

      for (const auto& update_entry :
           *updates) {
        const ptree& update =
            update_entry.second;

        const MarketDataSide side =
            parse_side(
                update.get<std::string>(
                    "side"
                )
            );

        const double price =
            parse_positive_double(
                update.get<std::string>(
                    "price_level"
                ),
                "price_level",
                false
            );

        const double quantity =
            parse_positive_double(
                update.get<std::string>(
                    "new_quantity"
                ),
                "new_quantity",
                true
            );

        if (event_type == "snapshot") {
          MarketDataLevel level{
              .price = price,
              .quantity = quantity
          };

          if (quantity <= 0.0) {
            continue;
          }

          if (side ==
              MarketDataSide::Bid) {
            result.book_snapshot
                .bids.push_back(level);
          } else {
            result.book_snapshot
                .asks.push_back(level);
          }
        } else {
          result.updates.push_back(
              MarketDataUpdate{
                  .venue = "COINBASE",
                  .symbol = product_id,
                  .sequence =
                      result.sequence,
                  .timestamp_ns =
                      received_timestamp_ns,
                  .side = side,
                  .type =
                      quantity <= 0.0
                          ? MarketDataUpdateType::
                                Delete
                          : MarketDataUpdateType::
                                Upsert,
                  .price = price,
                  .quantity = quantity
              }
          );
        }
      }
    }

    if (!initialized) {
      result.error =
          "Coinbase message has no supported events";

      return result;
    }

    result.valid = true;
    return result;
  } catch (const std::exception& error) {
    result.error = error.what();
    return result;
  }
}

std::string coinbase_level2_subscription(
    const std::string& product_id
) {
  if (product_id.empty()) {
    throw std::invalid_argument(
        "Coinbase product id cannot be empty"
    );
  }

  return
      "{\"type\":\"subscribe\","
      "\"product_ids\":[\"" +
      escape_json(product_id) +
      "\"],\"channel\":\"level2\"}";
}

std::string
coinbase_heartbeat_subscription() {
  return
      "{\"type\":\"subscribe\","
      "\"channel\":\"heartbeats\"}";
}

}  // namespace minimatch
