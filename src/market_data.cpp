#include "minimatch/market_data.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace minimatch {

namespace {

constexpr double epsilon = 1e-12;

void validate_identity(
    const std::string& venue,
    const std::string& symbol
) {
  if (venue.empty()) {
    throw std::invalid_argument(
        "market-data venue cannot be empty"
    );
  }

  if (symbol.empty()) {
    throw std::invalid_argument(
        "market-data symbol cannot be empty"
    );
  }
}

void validate_price(double price) {
  if (!std::isfinite(price) ||
      price <= 0.0) {
    throw std::invalid_argument(
        "market-data price must be positive"
    );
  }
}

void validate_quantity(double quantity) {
  if (!std::isfinite(quantity) ||
      quantity < 0.0) {
    throw std::invalid_argument(
        "market-data quantity must be non-negative"
    );
  }
}

template <typename Map>
std::vector<MarketDataLevel> copy_depth(
    const Map& levels,
    std::size_t depth
) {
  std::vector<MarketDataLevel> result;

  result.reserve(
      std::min(depth, levels.size())
  );

  for (const auto& [price, quantity] :
       levels) {
    if (result.size() >= depth) {
      break;
    }

    result.push_back(
        MarketDataLevel{
            .price = price,
            .quantity = quantity
        }
    );
  }

  return result;
}

}  // namespace

std::string to_string(
    MarketDataSide side
) {
  switch (side) {
    case MarketDataSide::Bid:
      return "BID";

    case MarketDataSide::Ask:
      return "ASK";
  }

  return "UNKNOWN";
}

std::string to_string(
    MarketDataUpdateType type
) {
  switch (type) {
    case MarketDataUpdateType::Snapshot:
      return "SNAPSHOT";

    case MarketDataUpdateType::Upsert:
      return "UPSERT";

    case MarketDataUpdateType::Delete:
      return "DELETE";
  }

  return "UNKNOWN";
}

MarketDataDecision Level2Book::apply_snapshot(
    const MarketDataSnapshot& snapshot
) {
  validate_identity(
      snapshot.venue,
      snapshot.symbol
  );

  std::map<
      double,
      double,
      std::greater<double>
  > next_bids;

  std::map<double, double> next_asks;

  for (const auto& level : snapshot.bids) {
    validate_price(level.price);
    validate_quantity(level.quantity);

    if (level.quantity > epsilon) {
      next_bids[level.price] =
          level.quantity;
    }
  }

  for (const auto& level : snapshot.asks) {
    validate_price(level.price);
    validate_quantity(level.quantity);

    if (level.quantity > epsilon) {
      next_asks[level.price] =
          level.quantity;
    }
  }

  if (!next_bids.empty() &&
      !next_asks.empty() &&
      next_bids.begin()->first >=
          next_asks.begin()->first) {
    throw std::invalid_argument(
        "snapshot contains crossed market"
    );
  }

  const bool was_recovering =
      !recovery_buffer_.empty() ||
      missing_sequence_ != 0;

  std::vector<MarketDataUpdate>
      buffered_updates;

  buffered_updates.reserve(
      recovery_buffer_.size()
  );

  for (const auto& [sequence, update] :
       recovery_buffer_) {
    static_cast<void>(sequence);
    buffered_updates.push_back(update);
  }

  recovery_buffer_.clear();

  bids_ = std::move(next_bids);
  asks_ = std::move(next_asks);

  sequence_ = snapshot.sequence;
  timestamp_ns_ = snapshot.timestamp_ns;
  synchronized_ = true;
  missing_sequence_ = 0;

  for (const auto& update :
       buffered_updates) {
    if (update.sequence <= sequence_) {
      continue;
    }

    static_cast<void>(apply(update));
  }

  if (was_recovering && synchronized_) {
    ++recovery_count_;
  }

  return MarketDataDecision{
      .accepted = true,
      .sequence_gap = !synchronized_,
      .expected_sequence =
          synchronized_
              ? sequence_ + 1
              : missing_sequence_,
      .received_sequence =
          snapshot.sequence,
      .message =
          synchronized_
              ? (
                    was_recovering
                        ? "snapshot accepted and recovery completed"
                        : "snapshot accepted"
                )
              : "snapshot accepted but buffered replay still contains a gap"
  };
}


MarketDataDecision Level2Book::apply(
    const MarketDataUpdate& update
) {
  validate_identity(
      update.venue,
      update.symbol
  );

  validate_price(update.price);
  validate_quantity(update.quantity);

  if (update.sequence == 0) {
    throw std::invalid_argument(
        "update sequence must be positive"
    );
  }

  if (update.sequence <= sequence_) {
    return MarketDataDecision{
        .accepted = false,
        .sequence_gap = false,
        .expected_sequence =
            sequence_ + 1,
        .received_sequence =
            update.sequence,
        .message =
            "stale or duplicate update ignored"
    };
  }

  if (!synchronized_) {
    if (
        recovery_buffer_.size() >=
            max_recovery_buffer_ &&
        !recovery_buffer_.contains(
            update.sequence
        )
    ) {
      return MarketDataDecision{
          .accepted = false,
          .sequence_gap = true,
          .expected_sequence =
              missing_sequence_,
          .received_sequence =
              update.sequence,
          .message =
              "recovery buffer capacity exceeded"
      };
    }

    recovery_buffer_.try_emplace(
        update.sequence,
        update
    );

    return MarketDataDecision{
        .accepted = false,
        .sequence_gap = true,
        .expected_sequence =
            missing_sequence_ != 0
                ? missing_sequence_
                : sequence_ + 1,
        .received_sequence =
            update.sequence,
        .message =
            "incremental buffered during recovery"
    };
  }

  const std::uint64_t expected =
      sequence_ + 1;

  if (update.sequence > expected) {
    synchronized_ = false;
    missing_sequence_ = expected;
    ++sequence_gap_count_;

    recovery_buffer_.try_emplace(
        update.sequence,
        update
    );

    return MarketDataDecision{
        .accepted = false,
        .sequence_gap = true,
        .expected_sequence = expected,
        .received_sequence =
            update.sequence,
        .message =
            "market-data sequence gap detected"
    };
  }

  const bool remove =
      update.type ==
          MarketDataUpdateType::Delete ||
      update.quantity <= epsilon;

  if (update.side ==
      MarketDataSide::Bid) {
    if (remove) {
      bids_.erase(update.price);
    } else {
      bids_[update.price] =
          update.quantity;
    }
  } else {
    if (remove) {
      asks_.erase(update.price);
    } else {
      asks_[update.price] =
          update.quantity;
    }
  }

  if (!bids_.empty() &&
      !asks_.empty() &&
      bids_.begin()->first >=
          asks_.begin()->first) {
    synchronized_ = false;

    return MarketDataDecision{
        .accepted = false,
        .sequence_gap = false,
        .expected_sequence = expected,
        .received_sequence =
            update.sequence,
        .message =
            "incremental update crossed the market"
    };
  }

  sequence_ = update.sequence;
  timestamp_ns_ = update.timestamp_ns;

  return MarketDataDecision{
      .accepted = true,
      .sequence_gap = false,
      .expected_sequence = expected,
      .received_sequence =
          update.sequence,
      .message =
          "incremental update accepted"
  };
}


MarketDataDecision Level2Book::apply_batch(
    const std::vector<MarketDataUpdate>& updates
) {
  if (updates.empty()) {
    return MarketDataDecision{
        .accepted = true,
        .sequence_gap = false,
        .expected_sequence = sequence_,
        .received_sequence = sequence_,
        .message = "empty update batch accepted"
    };
  }

  const auto& first = updates.front();

  validate_identity(
      first.venue,
      first.symbol
  );

  if (!synchronized_) {
    return MarketDataDecision{
        .accepted = false,
        .sequence_gap = true,
        .expected_sequence = sequence_ + 1,
        .received_sequence = first.sequence,
        .message =
            "snapshot required before incrementals"
    };
  }

  const std::uint64_t expected =
      sequence_ + 1;

  if (first.sequence != expected) {
    synchronized_ = false;

    return MarketDataDecision{
        .accepted = false,
        .sequence_gap = true,
        .expected_sequence = expected,
        .received_sequence = first.sequence,
        .message =
            "market-data sequence gap detected"
    };
  }

  auto next_bids = bids_;
  auto next_asks = asks_;

  std::uint64_t latest_timestamp =
      first.timestamp_ns;

  for (const auto& update : updates) {
    validate_identity(
        update.venue,
        update.symbol
    );

    if (update.venue != first.venue ||
        update.symbol != first.symbol) {
      throw std::invalid_argument(
          "market-data batch mixes books"
      );
    }

    if (update.sequence != first.sequence) {
      throw std::invalid_argument(
          "market-data batch mixes sequences"
      );
    }

    validate_price(update.price);
    validate_quantity(update.quantity);

    latest_timestamp =
        std::max(
            latest_timestamp,
            update.timestamp_ns
        );

    const bool remove =
        update.type ==
            MarketDataUpdateType::Delete ||
        update.quantity <= epsilon;

    if (update.side == MarketDataSide::Bid) {
      if (remove) {
        next_bids.erase(update.price);
      } else {
        next_bids[update.price] =
            update.quantity;
      }
    } else {
      if (remove) {
        next_asks.erase(update.price);
      } else {
        next_asks[update.price] =
            update.quantity;
      }
    }
  }

  if (!next_bids.empty() &&
      !next_asks.empty() &&
      next_bids.begin()->first >=
          next_asks.begin()->first) {
    synchronized_ = false;

    return MarketDataDecision{
        .accepted = false,
        .sequence_gap = false,
        .expected_sequence = expected,
        .received_sequence = first.sequence,
        .message =
            "incremental batch crossed the market"
    };
  }

  bids_ = std::move(next_bids);
  asks_ = std::move(next_asks);

  sequence_ = first.sequence;
  timestamp_ns_ = latest_timestamp;

  return MarketDataDecision{
      .accepted = true,
      .sequence_gap = false,
      .expected_sequence = expected,
      .received_sequence = first.sequence,
      .message =
          "incremental update batch accepted"
  };
}

std::optional<MarketDataLevel>
Level2Book::best_bid() const {
  if (bids_.empty()) {
    return std::nullopt;
  }

  return MarketDataLevel{
      .price = bids_.begin()->first,
      .quantity = bids_.begin()->second
  };
}

std::optional<MarketDataLevel>
Level2Book::best_ask() const {
  if (asks_.empty()) {
    return std::nullopt;
  }

  return MarketDataLevel{
      .price = asks_.begin()->first,
      .quantity = asks_.begin()->second
  };
}

std::vector<MarketDataLevel>
Level2Book::bids(std::size_t depth) const {
  return copy_depth(bids_, depth);
}

std::vector<MarketDataLevel>
Level2Book::asks(std::size_t depth) const {
  return copy_depth(asks_, depth);
}

std::uint64_t Level2Book::sequence() const {
  return sequence_;
}

std::uint64_t Level2Book::timestamp_ns() const {
  return timestamp_ns_;
}

bool Level2Book::synchronized() const {
  return synchronized_;
}

bool Level2Book::recovering() const {
  return !synchronized_ &&
         missing_sequence_ != 0;
}

std::uint64_t
Level2Book::missing_sequence() const {
  return missing_sequence_;
}

std::size_t
Level2Book::buffered_update_count() const {
  return recovery_buffer_.size();
}

std::uint64_t
Level2Book::sequence_gap_count() const {
  return sequence_gap_count_;
}

std::uint64_t
Level2Book::recovery_count() const {
  return recovery_count_;
}


void Level2Book::clear() {
  bids_.clear();
  asks_.clear();

  recovery_buffer_.clear();

  sequence_ = 0;
  timestamp_ns_ = 0;

  synchronized_ = false;
  missing_sequence_ = 0;
  sequence_gap_count_ = 0;
  recovery_count_ = 0;
}

MarketDataDecision
ConsolidatedMarketData::apply_snapshot(
    const MarketDataSnapshot& snapshot
) {
  return books_[
      key(snapshot.venue, snapshot.symbol)
  ].apply_snapshot(snapshot);
}

MarketDataDecision
ConsolidatedMarketData::apply(
    const MarketDataUpdate& update
) {
  return books_[
      key(update.venue, update.symbol)
  ].apply(update);
}


MarketDataDecision
ConsolidatedMarketData::apply_batch(
    const std::vector<MarketDataUpdate>& updates
) {
  if (updates.empty()) {
    return MarketDataDecision{
        .accepted = true,
        .message = "empty update batch accepted"
    };
  }

  const auto& first = updates.front();

  return books_[
      key(first.venue, first.symbol)
  ].apply_batch(updates);
}

std::optional<Level2Book>
ConsolidatedMarketData::find_book(
    const std::string& venue,
    const std::string& symbol
) const {
  const auto iterator =
      books_.find(key(venue, symbol));

  if (iterator == books_.end()) {
    return std::nullopt;
  }

  return iterator->second;
}

BestBidOffer
ConsolidatedMarketData::consolidated_bbo(
    const std::string& symbol
) const {
  BestBidOffer result;

  for (const auto& [book_key, book] :
       books_) {
    static_cast<void>(book_key);

    if (!book.synchronized()) {
      continue;
    }

    const auto separator =
        book_key.find('\n');

    if (separator == std::string::npos) {
      continue;
    }

    const std::string venue =
        book_key.substr(0, separator);

    const std::string stored_symbol =
        book_key.substr(separator + 1);

    if (stored_symbol != symbol) {
      continue;
    }

    const auto bid = book.best_bid();
    const auto ask = book.best_ask();

    if (bid.has_value() &&
        (result.bid_venue.empty() ||
         bid->price > result.bid_price)) {
      result.bid_price = bid->price;
      result.bid_quantity = bid->quantity;
      result.bid_venue = venue;
    }

    if (ask.has_value() &&
        (result.ask_venue.empty() ||
         ask->price < result.ask_price)) {
      result.ask_price = ask->price;
      result.ask_quantity = ask->quantity;
      result.ask_venue = venue;
    }

    result.valid =
        !result.bid_venue.empty() &&
        !result.ask_venue.empty();
  }

  if (result.valid) {
    result.spread =
        result.ask_price -
        result.bid_price;

    result.locked =
        result.spread == 0.0;

    result.crossed =
        result.spread < 0.0;

    if (result.crossed) {
      result.valid = false;
      result.midpoint = 0.0;
    } else {
      result.midpoint =
          (result.bid_price +
           result.ask_price) /
          2.0;
    }
  }

  return result;
}

std::vector<std::string>
ConsolidatedMarketData::venues_for_symbol(
    const std::string& symbol
) const {
  std::vector<std::string> result;

  for (const auto& [book_key, book] :
       books_) {
    static_cast<void>(book);

    const auto separator =
        book_key.find('\n');

    if (separator == std::string::npos) {
      continue;
    }

    if (book_key.substr(separator + 1) ==
        symbol) {
      result.push_back(
          book_key.substr(0, separator)
      );
    }
  }

  std::sort(
      result.begin(),
      result.end()
  );

  return result;
}

std::string ConsolidatedMarketData::key(
    const std::string& venue,
    const std::string& symbol
) {
  validate_identity(venue, symbol);

  return venue + "\n" + symbol;
}

}  // namespace minimatch
