#include "minimatch/itch.hpp"

#include <gtest/gtest.h>

using namespace minimatch;

TEST(Itch, AddOrderDefaults) {
  ItchAddOrder msg;

  EXPECT_EQ(msg.message_type, ItchMessageType::AddOrder);
  EXPECT_EQ(msg.side, 'B');
  EXPECT_EQ(msg.shares, 0u);
  EXPECT_EQ(msg.price, 0u);
}

TEST(Itch, RecognizesValidSides) {
  EXPECT_TRUE(is_valid_itch_side('B'));
  EXPECT_TRUE(is_valid_itch_side('S'));
  EXPECT_FALSE(is_valid_itch_side('X'));
}

TEST(Itch, WritesAndReadsStockSymbol) {
  ItchAddOrder msg;

  write_itch_stock(msg.stock, "AAPL");

  EXPECT_EQ(read_itch_stock(msg.stock), "AAPL");
}

TEST(Itch, TruncatesLongStockSymbol) {
  ItchAddOrder msg;

  write_itch_stock(msg.stock, "LONGSYMBOL");

  EXPECT_EQ(read_itch_stock(msg.stock), "LONGSYMB");
}

TEST(Itch, HandlesEmptyStockSymbol) {
  ItchAddOrder msg;

  write_itch_stock(msg.stock, "");

  EXPECT_EQ(read_itch_stock(msg.stock), "");
}

TEST(Itch, ConvertsPriceToFixedPoint) {
  EXPECT_EQ(encode_itch_price(189.25), 1892500U);
  EXPECT_DOUBLE_EQ(decode_itch_price(1892500U), 189.25);
}

TEST(Itch, RejectsNegativePrice) {
  EXPECT_FALSE(is_valid_itch_price(-0.01));
  EXPECT_TRUE(is_valid_itch_price(0.0));
  EXPECT_TRUE(is_valid_itch_price(189.25));
}
