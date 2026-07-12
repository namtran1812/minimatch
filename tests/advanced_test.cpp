#include "minimatch/exchange.hpp"
#include <gtest/gtest.h>
#include <cstdio>
#include <vector>
using namespace minimatch;

TEST(AdvancedOrderBook, FillOrKillRejectsWithoutMutation) {
  OrderBook book(32, 1);
  book.submit({1, 1, Side::Sell, OrderType::Limit, 100, 5, 1});
  const auto before = book.state_hash();
  EXPECT_FALSE(book.submit({2, 2, Side::Buy, OrderType::FOK, 100, 10, 1}));
  EXPECT_EQ(book.state_hash(), before);
  EXPECT_EQ(book.quantity_at(Side::Sell, 100), 5);
}

TEST(AdvancedOrderBook, FillOrKillExecutesCompletely) {
  OrderBook book(32, 1);
  std::vector<Trade> trades;
  book.on_trade([&](const Trade& trade) { trades.push_back(trade); });
  book.submit({1, 1, Side::Sell, OrderType::Limit, 100, 5, 1});
  book.submit({2, 1, Side::Sell, OrderType::Limit, 101, 5, 1});
  EXPECT_TRUE(book.submit({3, 2, Side::Buy, OrderType::FOK, 101, 10, 1}));
  EXPECT_EQ(trades.size(), 2);
  EXPECT_EQ(book.active_orders(), 0);
}

TEST(AdvancedOrderBook, PostOnlyRejectsCrossingOrder) {
  OrderBook book(16, 1);
  book.submit({1, 1, Side::Sell, OrderType::Limit, 100, 5, 1});
  EXPECT_FALSE(book.submit({2, 2, Side::Buy, OrderType::PostOnly, 100, 3, 1}));
  EXPECT_EQ(book.quantity_at(Side::Sell, 100), 5);
  EXPECT_EQ(book.quantity_at(Side::Buy, 100), 0);
}

TEST(AdvancedOrderBook, QuantityReductionPreservesPriority) {
  OrderBook book(16, 1);
  std::vector<Trade> trades;
  book.on_trade([&](const Trade& trade) { trades.push_back(trade); });
  book.submit({1, 1, Side::Sell, OrderType::Limit, 100, 10, 1});
  book.submit({2, 1, Side::Sell, OrderType::Limit, 100, 10, 1});
  EXPECT_TRUE(book.replace({1, 1, 100, 5, 1}));
  book.submit({3, 2, Side::Buy, OrderType::Market, 0, 7, 1});
  ASSERT_EQ(trades.size(), 2);
  EXPECT_EQ(trades[0].maker_order_id, 1);
  EXPECT_EQ(trades[0].quantity, 5);
  EXPECT_EQ(trades[1].maker_order_id, 2);
}

TEST(AdvancedOrderBook, PriceChangeLosesPriority) {
  OrderBook book(16, 1);
  std::vector<Trade> trades;
  book.on_trade([&](const Trade& trade) { trades.push_back(trade); });
  book.submit({1, 1, Side::Sell, OrderType::Limit, 100, 5, 1});
  book.submit({2, 1, Side::Sell, OrderType::Limit, 101, 5, 1});
  EXPECT_TRUE(book.replace({1, 1, 101, 5, 1}));
  book.submit({3, 2, Side::Buy, OrderType::Market, 0, 5, 1});
  ASSERT_EQ(trades.size(), 1);
  EXPECT_EQ(trades[0].maker_order_id, 2);
}

TEST(AdvancedExchange, MultiSymbolIsolation) {
  Exchange exchange(32);
  exchange.submit({1, 1, Side::Buy, OrderType::Limit, 100, 5, 1});
  exchange.submit({2, 1, Side::Buy, OrderType::Limit, 200, 7, 2});
  ASSERT_NE(exchange.find_book(1), nullptr);
  ASSERT_NE(exchange.find_book(2), nullptr);
  EXPECT_EQ(exchange.find_book(1)->best_bid(), 100);
  EXPECT_EQ(exchange.find_book(2)->best_bid(), 200);
  EXPECT_EQ(exchange.active_orders(), 2);
}

TEST(AdvancedExchange, SnapshotRoundTrip) {
  const char* path = "advanced_test.snapshot";
  Exchange before(64);
  before.submit({1, 1, Side::Buy, OrderType::Limit, 99, 10, 1});
  before.submit({2, 2, Side::Sell, OrderType::Limit, 101, 8, 1});
  before.submit({3, 3, Side::Buy, OrderType::Limit, 199, 4, 2});
  before.write_snapshot(path);
  Exchange after(64);
  after.load_snapshot(path);
  EXPECT_EQ(after.state_hash(), before.state_hash());
  EXPECT_EQ(after.active_orders(), before.active_orders());
  std::remove(path);
}

TEST(AdvancedOrderBook, DepthIncludesOrderCount) {
  OrderBook book(16, 1);
  book.submit({1, 1, Side::Buy, OrderType::Limit, 100, 5, 1});
  book.submit({2, 2, Side::Buy, OrderType::Limit, 100, 7, 1});
  const auto depth = book.depth(Side::Buy, 5);
  ASSERT_EQ(depth.size(), 1);
  EXPECT_EQ(depth[0].quantity, 12);
  EXPECT_EQ(depth[0].order_count, 2);
}
