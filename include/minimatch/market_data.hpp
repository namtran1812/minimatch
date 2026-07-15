#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace minimatch {

enum class MarketDataSide {
  Bid,
  Ask
};

enum class MarketDataUpdateType {
  Snapshot,
  Upsert,
  Delete
};

struct MarketDataLevel {
  double price{0.0};
  double quantity{0.0};
};

struct MarketDataUpdate {
  std::string venue;
  std::string symbol;

  std::uint64_t sequence{0};
  std::uint64_t timestamp_ns{0};

  MarketDataSide side{MarketDataSide::Bid};
  MarketDataUpdateType type{
      MarketDataUpdateType::Upsert
  };

  double price{0.0};
  double quantity{0.0};
};

struct MarketDataSnapshot {
  std::string venue;
  std::string symbol;

  std::uint64_t sequence{0};
  std::uint64_t timestamp_ns{0};

  std::vector<MarketDataLevel> bids;
  std::vector<MarketDataLevel> asks;
};

struct MarketDataDecision {
  bool accepted{false};
  bool sequence_gap{false};

  std::uint64_t expected_sequence{0};
  std::uint64_t received_sequence{0};

  std::string message;
};

struct BestBidOffer {
  bool valid{false};

  double bid_price{0.0};
  double bid_quantity{0.0};
  std::string bid_venue;

  double ask_price{0.0};
  double ask_quantity{0.0};
  std::string ask_venue;

  double midpoint{0.0};
  double spread{0.0};
};

class Level2Book {
 public:
  MarketDataDecision apply_snapshot(
      const MarketDataSnapshot& snapshot
  );

  MarketDataDecision apply(
      const MarketDataUpdate& update
  );

  MarketDataDecision apply_batch(
      const std::vector<MarketDataUpdate>& updates
  );

  [[nodiscard]] std::optional<MarketDataLevel>
  best_bid() const;

  [[nodiscard]] std::optional<MarketDataLevel>
  best_ask() const;

  [[nodiscard]] std::vector<MarketDataLevel>
  bids(std::size_t depth) const;

  [[nodiscard]] std::vector<MarketDataLevel>
  asks(std::size_t depth) const;

  [[nodiscard]] std::uint64_t sequence() const;
  [[nodiscard]] std::uint64_t timestamp_ns() const;

  [[nodiscard]] bool synchronized() const;

  void clear();

 private:
  std::map<
      double,
      double,
      std::greater<double>
  > bids_;

  std::map<double, double> asks_;

  std::uint64_t sequence_{0};
  std::uint64_t timestamp_ns_{0};

  bool synchronized_{false};
};

class ConsolidatedMarketData {
 public:
  MarketDataDecision apply_snapshot(
      const MarketDataSnapshot& snapshot
  );

  MarketDataDecision apply(
      const MarketDataUpdate& update
  );

  MarketDataDecision apply_batch(
      const std::vector<MarketDataUpdate>& updates
  );

  [[nodiscard]] std::optional<Level2Book>
  find_book(
      const std::string& venue,
      const std::string& symbol
  ) const;

  [[nodiscard]] BestBidOffer consolidated_bbo(
      const std::string& symbol
  ) const;

  [[nodiscard]] std::vector<std::string>
  venues_for_symbol(
      const std::string& symbol
  ) const;

 private:
  std::unordered_map<
      std::string,
      Level2Book
  > books_;

  static std::string key(
      const std::string& venue,
      const std::string& symbol
  );
};

std::string to_string(MarketDataSide side);

std::string to_string(
    MarketDataUpdateType type
);

}  // namespace minimatch
