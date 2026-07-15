#include "minimatch/market_data.hpp"

#include <gtest/gtest.h>

namespace {

using minimatch::ConsolidatedMarketData;
using minimatch::Level2Book;
using minimatch::MarketDataLevel;
using minimatch::MarketDataSide;
using minimatch::MarketDataSnapshot;
using minimatch::MarketDataUpdate;
using minimatch::MarketDataUpdateType;

MarketDataSnapshot snapshot(
    const std::string& venue,
    std::uint64_t sequence
) {
  return MarketDataSnapshot{
      .venue = venue,
      .symbol = "BTCUSD",
      .sequence = sequence,
      .timestamp_ns = 100,
      .bids = {
          MarketDataLevel{100.0, 2.0},
          MarketDataLevel{99.5, 3.0}
      },
      .asks = {
          MarketDataLevel{100.5, 4.0},
          MarketDataLevel{101.0, 5.0}
      }
  };
}

TEST(MarketData, AppliesSnapshot) {
  Level2Book book;

  const auto result =
      book.apply_snapshot(
          snapshot("COINBASE", 10)
      );

  EXPECT_TRUE(result.accepted);
  EXPECT_TRUE(book.synchronized());
  EXPECT_EQ(book.sequence(), 10U);

  ASSERT_TRUE(book.best_bid().has_value());
  ASSERT_TRUE(book.best_ask().has_value());

  EXPECT_DOUBLE_EQ(
      book.best_bid()->price,
      100.0
  );

  EXPECT_DOUBLE_EQ(
      book.best_ask()->price,
      100.5
  );
}

TEST(MarketData, AppliesIncrementalUpdate) {
  Level2Book book;

  book.apply_snapshot(
      snapshot("COINBASE", 10)
  );

  const auto result =
      book.apply(
          MarketDataUpdate{
              .venue = "COINBASE",
              .symbol = "BTCUSD",
              .sequence = 11,
              .timestamp_ns = 200,
              .side = MarketDataSide::Bid,
              .type =
                  MarketDataUpdateType::Upsert,
              .price = 100.25,
              .quantity = 1.5
          }
      );

  EXPECT_TRUE(result.accepted);
  EXPECT_EQ(book.sequence(), 11U);
  EXPECT_DOUBLE_EQ(
      book.best_bid()->price,
      100.25
  );
}

TEST(MarketData, DeletesPriceLevel) {
  Level2Book book;

  book.apply_snapshot(
      snapshot("COINBASE", 10)
  );

  EXPECT_TRUE(
      book.apply(
          MarketDataUpdate{
              .venue = "COINBASE",
              .symbol = "BTCUSD",
              .sequence = 11,
              .timestamp_ns = 200,
              .side = MarketDataSide::Bid,
              .type =
                  MarketDataUpdateType::Delete,
              .price = 100.0,
              .quantity = 0.0
          }
      ).accepted
  );

  EXPECT_DOUBLE_EQ(
      book.best_bid()->price,
      99.5
  );
}

TEST(MarketData, DetectsSequenceGap) {
  Level2Book book;

  book.apply_snapshot(
      snapshot("COINBASE", 10)
  );

  const auto result =
      book.apply(
          MarketDataUpdate{
              .venue = "COINBASE",
              .symbol = "BTCUSD",
              .sequence = 13,
              .timestamp_ns = 200,
              .side = MarketDataSide::Bid,
              .type =
                  MarketDataUpdateType::Upsert,
              .price = 100.25,
              .quantity = 1.0
          }
      );

  EXPECT_FALSE(result.accepted);
  EXPECT_TRUE(result.sequence_gap);
  EXPECT_EQ(result.expected_sequence, 11U);
  EXPECT_EQ(result.received_sequence, 13U);
  EXPECT_FALSE(book.synchronized());
}

TEST(MarketData, RequiresSnapshotBeforeIncrementals) {
  Level2Book book;

  const auto result =
      book.apply(
          MarketDataUpdate{
              .venue = "COINBASE",
              .symbol = "BTCUSD",
              .sequence = 1,
              .timestamp_ns = 100,
              .side = MarketDataSide::Ask,
              .type =
                  MarketDataUpdateType::Upsert,
              .price = 101.0,
              .quantity = 2.0
          }
      );

  EXPECT_FALSE(result.accepted);
  EXPECT_TRUE(result.sequence_gap);
}

TEST(MarketData, ReturnsRequestedDepth) {
  Level2Book book;

  book.apply_snapshot(
      snapshot("COINBASE", 10)
  );

  const auto bids = book.bids(1);
  const auto asks = book.asks(2);

  ASSERT_EQ(bids.size(), 1U);
  ASSERT_EQ(asks.size(), 2U);

  EXPECT_DOUBLE_EQ(bids[0].price, 100.0);
  EXPECT_DOUBLE_EQ(asks[0].price, 100.5);
}

TEST(MarketData, RejectsCrossedSnapshot) {
  Level2Book book;

  auto crossed =
      snapshot("COINBASE", 10);

  crossed.asks[0].price = 99.0;

  EXPECT_THROW(
      book.apply_snapshot(crossed),
      std::invalid_argument
  );
}

TEST(MarketData, BuildsConsolidatedBbo) {
  ConsolidatedMarketData market;

  auto coinbase =
      snapshot("COINBASE", 10);

  auto kraken =
      snapshot("KRAKEN", 20);

  kraken.bids[0] =
      MarketDataLevel{100.2, 4.0};

  kraken.asks[0] =
      MarketDataLevel{100.4, 5.0};

  market.apply_snapshot(coinbase);
  market.apply_snapshot(kraken);

  const auto bbo =
      market.consolidated_bbo("BTCUSD");

  EXPECT_TRUE(bbo.valid);

  EXPECT_EQ(
      bbo.bid_venue,
      "KRAKEN"
  );

  EXPECT_EQ(
      bbo.ask_venue,
      "KRAKEN"
  );

  EXPECT_DOUBLE_EQ(
      bbo.bid_price,
      100.2
  );

  EXPECT_DOUBLE_EQ(
      bbo.ask_price,
      100.4
  );

  EXPECT_NEAR(
      bbo.spread,
      0.2,
      1e-9
  );

  EXPECT_NEAR(
      bbo.midpoint,
      100.3,
      1e-9
  );
}

TEST(MarketData, ExcludesUnsynchronizedVenue) {
  ConsolidatedMarketData market;

  market.apply_snapshot(
      snapshot("COINBASE", 10)
  );

  market.apply_snapshot(
      snapshot("KRAKEN", 20)
  );

  market.apply(
      MarketDataUpdate{
          .venue = "KRAKEN",
          .symbol = "BTCUSD",
          .sequence = 25,
          .timestamp_ns = 200,
          .side = MarketDataSide::Bid,
          .type =
              MarketDataUpdateType::Upsert,
          .price = 100.25,
          .quantity = 3.0
      }
  );

  const auto bbo =
      market.consolidated_bbo("BTCUSD");

  EXPECT_TRUE(bbo.valid);
  EXPECT_EQ(
      bbo.bid_venue,
      "COINBASE"
  );
  EXPECT_EQ(
      bbo.ask_venue,
      "COINBASE"
  );
}

TEST(MarketData, ListsVenuesForSymbol) {
  ConsolidatedMarketData market;

  market.apply_snapshot(
      snapshot("KRAKEN", 10)
  );

  market.apply_snapshot(
      snapshot("COINBASE", 10)
  );

  const auto venues =
      market.venues_for_symbol("BTCUSD");

  ASSERT_EQ(venues.size(), 2U);
  EXPECT_EQ(venues[0], "COINBASE");
  EXPECT_EQ(venues[1], "KRAKEN");
}

}  // namespace
