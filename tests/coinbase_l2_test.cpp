#include "minimatch/coinbase_l2.hpp"

#include <gtest/gtest.h>

namespace {

using minimatch::MarketDataSide;
using minimatch::MarketDataUpdateType;
using minimatch::coinbase_heartbeat_subscription;
using minimatch::coinbase_level2_subscription;
using minimatch::parse_coinbase_level2_message;

TEST(CoinbaseLevel2, ParsesSnapshot) {
  const std::string json = R"JSON(
{
  "channel": "l2_data",
  "timestamp": "2026-07-14T12:00:00Z",
  "sequence_num": 10,
  "events": [
    {
      "type": "snapshot",
      "product_id": "BTC-USD",
      "updates": [
        {
          "side": "bid",
          "event_time": "2026-07-14T12:00:00Z",
          "price_level": "62640.00",
          "new_quantity": "1.50"
        },
        {
          "side": "offer",
          "event_time": "2026-07-14T12:00:00Z",
          "price_level": "62640.10",
          "new_quantity": "2.25"
        }
      ]
    }
  ]
}
)JSON";

  const auto result =
      parse_coinbase_level2_message(
          json,
          1000
      );

  ASSERT_TRUE(result.valid)
      << result.error;

  EXPECT_TRUE(result.snapshot);
  EXPECT_FALSE(result.heartbeat);
  EXPECT_EQ(result.sequence, 10U);
  EXPECT_EQ(result.product_id, "BTC-USD");

  ASSERT_EQ(
      result.book_snapshot.bids.size(),
      1U
  );

  ASSERT_EQ(
      result.book_snapshot.asks.size(),
      1U
  );

  EXPECT_DOUBLE_EQ(
      result.book_snapshot.bids[0].price,
      62640.00
  );

  EXPECT_DOUBLE_EQ(
      result.book_snapshot.asks[0].quantity,
      2.25
  );
}

TEST(CoinbaseLevel2, ParsesIncrementalUpdates) {
  const std::string json = R"JSON(
{
  "channel": "l2_data",
  "sequence_num": 11,
  "events": [
    {
      "type": "update",
      "product_id": "BTC-USD",
      "updates": [
        {
          "side": "bid",
          "event_time": "2026-07-14T12:00:01Z",
          "price_level": "62640.01",
          "new_quantity": "0.75"
        },
        {
          "side": "offer",
          "event_time": "2026-07-14T12:00:01Z",
          "price_level": "62640.10",
          "new_quantity": "0"
        }
      ]
    }
  ]
}
)JSON";

  const auto result =
      parse_coinbase_level2_message(
          json,
          2000
      );

  ASSERT_TRUE(result.valid)
      << result.error;

  EXPECT_FALSE(result.snapshot);

  ASSERT_EQ(result.updates.size(), 2U);

  EXPECT_EQ(
      result.updates[0].side,
      MarketDataSide::Bid
  );

  EXPECT_EQ(
      result.updates[0].type,
      MarketDataUpdateType::Upsert
  );

  EXPECT_EQ(
      result.updates[1].side,
      MarketDataSide::Ask
  );

  EXPECT_EQ(
      result.updates[1].type,
      MarketDataUpdateType::Delete
  );
}

TEST(CoinbaseLevel2, ParsesHeartbeat) {
  const std::string json = R"JSON(
{
  "channel": "heartbeats",
  "sequence_num": 12,
  "events": [
    {
      "heartbeat_counter": "100"
    }
  ]
}
)JSON";

  const auto result =
      parse_coinbase_level2_message(
          json,
          3000
      );

  EXPECT_TRUE(result.valid);
  EXPECT_TRUE(result.heartbeat);
}

TEST(CoinbaseLevel2, RejectsUnsupportedChannel) {
  const auto result =
      parse_coinbase_level2_message(
          R"JSON({"channel":"ticker"})JSON",
          100
      );

  EXPECT_FALSE(result.valid);
  EXPECT_EQ(
      result.error,
      "unsupported Coinbase channel"
  );
}

TEST(CoinbaseLevel2, BuildsSubscriptions) {
  EXPECT_EQ(
      coinbase_level2_subscription(
          "BTC-USD"
      ),
      "{\"type\":\"subscribe\","
      "\"product_ids\":[\"BTC-USD\"],"
      "\"channel\":\"level2\"}"
  );

  EXPECT_EQ(
      coinbase_heartbeat_subscription(),
      "{\"type\":\"subscribe\","
      "\"channel\":\"heartbeats\"}"
  );
}

}  // namespace
