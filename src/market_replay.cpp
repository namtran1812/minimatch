#include "minimatch/market_replay.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace minimatch {

namespace {

std::vector<std::string> split_csv_line(
    const std::string& line
) {
  std::vector<std::string> fields;
  std::string field;
  bool quoted = false;

  for (std::size_t index = 0;
       index < line.size();
       ++index) {
    const char value = line[index];

    if (value == '"') {
      if (quoted &&
          index + 1 < line.size() &&
          line[index + 1] == '"') {
        field.push_back('"');
        ++index;
      } else {
        quoted = !quoted;
      }

      continue;
    }

    if (value == ',' && !quoted) {
      fields.push_back(field);
      field.clear();
      continue;
    }

    field.push_back(value);
  }

  fields.push_back(field);
  return fields;
}

double parse_double(
    const std::string& value,
    const char* field_name
) {
  try {
    std::size_t consumed = 0;

    const double result =
        std::stod(value, &consumed);

    if (consumed != value.size() ||
        !std::isfinite(result)) {
      throw std::invalid_argument(
          "invalid floating-point value"
      );
    }

    return result;
  } catch (const std::exception&) {
    throw std::runtime_error(
        std::string("invalid ") +
        field_name +
        ": " +
        value
    );
  }
}

std::uint64_t parse_timestamp(
    const std::string& value
) {
  try {
    std::size_t consumed = 0;

    const auto result =
        std::stoull(value, &consumed);

    if (consumed != value.size()) {
      throw std::invalid_argument(
          "invalid timestamp"
      );
    }

    return static_cast<std::uint64_t>(
        result
    );
  } catch (const std::exception&) {
    throw std::runtime_error(
        "invalid timestamp_ns: " + value
    );
  }
}

MarketEventType parse_event_type(
    const std::string& value
) {
  if (value == "QUOTE" ||
      value == "quote") {
    return MarketEventType::Quote;
  }

  if (value == "TRADE" ||
      value == "trade") {
    return MarketEventType::Trade;
  }

  throw std::runtime_error(
      "unsupported market event type: " +
      value
  );
}

void validate_event(
    const MarketReplayEvent& event
) {
  if (event.timestamp_ns == 0) {
    throw std::invalid_argument(
        "market event timestamp must be positive"
    );
  }

  if (event.symbol.empty()) {
    throw std::invalid_argument(
        "market event symbol cannot be empty"
    );
  }

  if (event.venue.empty()) {
    throw std::invalid_argument(
        "market event venue cannot be empty"
    );
  }

  if (event.type == MarketEventType::Quote) {
    if (!std::isfinite(event.bid_price) ||
        !std::isfinite(event.ask_price) ||
        !std::isfinite(event.bid_quantity) ||
        !std::isfinite(event.ask_quantity) ||
        event.bid_price <= 0.0 ||
        event.ask_price <= 0.0 ||
        event.bid_quantity < 0.0 ||
        event.ask_quantity < 0.0 ||
        event.ask_price < event.bid_price) {
      throw std::invalid_argument(
          "invalid quote event"
      );
    }

    return;
  }

  if (!std::isfinite(event.trade_price) ||
      !std::isfinite(event.trade_quantity) ||
      event.trade_price <= 0.0 ||
      event.trade_quantity <= 0.0) {
    throw std::invalid_argument(
        "invalid trade event"
    );
  }
}

}  // namespace

std::string to_string(
    MarketEventType type
) {
  switch (type) {
    case MarketEventType::Quote:
      return "QUOTE";

    case MarketEventType::Trade:
      return "TRADE";
  }

  return "UNKNOWN";
}

MarketReplay::MarketReplay(
    std::vector<MarketReplayEvent> events
)
    : events_(std::move(events)) {
  normalize();
}

MarketReplay MarketReplay::from_csv(
    const std::string& path
) {
  std::ifstream input(path);

  if (!input) {
    throw std::runtime_error(
        "could not open market replay file: " +
        path
    );
  }

  std::string header;

  if (!std::getline(input, header)) {
    throw std::runtime_error(
        "market replay CSV is empty"
    );
  }

  const std::string expected_header =
      "timestamp_ns,type,symbol,venue,"
      "bid_price,bid_quantity,"
      "ask_price,ask_quantity,"
      "trade_price,trade_quantity";

  if (header != expected_header) {
    throw std::runtime_error(
        "unexpected market replay CSV header"
    );
  }

  std::vector<MarketReplayEvent> events;
  std::string line;
  std::size_t line_number = 1;

  while (std::getline(input, line)) {
    ++line_number;

    if (line.empty()) {
      continue;
    }

    const auto fields =
        split_csv_line(line);

    if (fields.size() != 10) {
      throw std::runtime_error(
          "invalid CSV field count on line " +
          std::to_string(line_number)
      );
    }

    MarketReplayEvent event;

    event.timestamp_ns =
        parse_timestamp(fields[0]);

    event.type =
        parse_event_type(fields[1]);

    event.symbol = fields[2];
    event.venue = fields[3];

    event.bid_price =
        fields[4].empty()
            ? 0.0
            : parse_double(
                  fields[4],
                  "bid_price"
              );

    event.bid_quantity =
        fields[5].empty()
            ? 0.0
            : parse_double(
                  fields[5],
                  "bid_quantity"
              );

    event.ask_price =
        fields[6].empty()
            ? 0.0
            : parse_double(
                  fields[6],
                  "ask_price"
              );

    event.ask_quantity =
        fields[7].empty()
            ? 0.0
            : parse_double(
                  fields[7],
                  "ask_quantity"
              );

    event.trade_price =
        fields[8].empty()
            ? 0.0
            : parse_double(
                  fields[8],
                  "trade_price"
              );

    event.trade_quantity =
        fields[9].empty()
            ? 0.0
            : parse_double(
                  fields[9],
                  "trade_quantity"
              );

    try {
      validate_event(event);
    } catch (const std::exception& error) {
      throw std::runtime_error(
          "invalid market replay event on line " +
          std::to_string(line_number) +
          ": " +
          error.what()
      );
    }

    events.push_back(
        std::move(event)
    );
  }

  return MarketReplay(
      std::move(events)
  );
}

bool MarketReplay::empty() const {
  return events_.empty();
}

std::size_t MarketReplay::size() const {
  return events_.size();
}

std::size_t MarketReplay::position() const {
  return position_;
}

bool MarketReplay::finished() const {
  return position_ >= events_.size();
}

const MarketReplayStats&
MarketReplay::stats() const {
  return stats_;
}

std::optional<MarketReplayEvent>
MarketReplay::peek() const {
  if (finished()) {
    return std::nullopt;
  }

  return events_[position_];
}

std::optional<MarketReplayEvent>
MarketReplay::next() {
  if (finished()) {
    return std::nullopt;
  }

  return events_[position_++];
}

void MarketReplay::reset() {
  position_ = 0;
}

bool MarketReplay::seek_index(
    std::size_t index
) {
  if (index > events_.size()) {
    return false;
  }

  position_ = index;
  return true;
}

bool MarketReplay::seek_timestamp(
    std::uint64_t timestamp_ns
) {
  const auto iterator = std::lower_bound(
      events_.begin(),
      events_.end(),
      timestamp_ns,
      [](const MarketReplayEvent& event,
         std::uint64_t timestamp) {
        return event.timestamp_ns <
            timestamp;
      }
  );

  if (iterator == events_.end()) {
    position_ = events_.size();
    return false;
  }

  position_ = static_cast<std::size_t>(
      std::distance(
          events_.begin(),
          iterator
      )
  );

  return true;
}

const std::vector<MarketReplayEvent>&
MarketReplay::events() const {
  return events_;
}

void MarketReplay::normalize() {
  for (const auto& event : events_) {
    validate_event(event);
  }

  std::stable_sort(
      events_.begin(),
      events_.end(),
      [](const MarketReplayEvent& left,
         const MarketReplayEvent& right) {
        return left.timestamp_ns <
            right.timestamp_ns;
      }
  );

  position_ = 0;
  calculate_stats();
}

void MarketReplay::calculate_stats() {
  stats_ = {};
  stats_.event_count = events_.size();

  for (const auto& event : events_) {
    if (event.type ==
        MarketEventType::Quote) {
      ++stats_.quote_count;
    } else {
      ++stats_.trade_count;
    }
  }

  if (!events_.empty()) {
    stats_.first_timestamp_ns =
        events_.front().timestamp_ns;

    stats_.last_timestamp_ns =
        events_.back().timestamp_ns;
  }
}

}  // namespace minimatch
