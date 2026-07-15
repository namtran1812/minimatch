#include "minimatch/binance_l2.hpp"

#include <gtest/gtest.h>

namespace {

using minimatch::BinanceBookSynchronizer;
using minimatch::BinanceSyncStatus;
using minimatch::MarketDataSide;
using minimatch::MarketDataUpdateType;
using minimatch::parse_binance_depth_snapshot;
using minimatch::parse_binance_depth_update;

TEST(BinanceLevel2, ParsesSnapshot) {
  const auto snapshot =
      parse_binance_depth_snapshot(
          R"JSON(
{
  "lastUpdateId": 100,
  "bids": [
    ["64824.10", "1.25"],
    ["64824.00", "2.00"]
  ],
  "asks": [
    ["64824.20", "1.50"],
    ["64824.30", "2.50"]
  ]
}
)JSON",
          "BTCUSDT",
          1000
      );

  ASSERT_TRUE(snapshot.valid)
      << snapshot.error;

  EXPECT_EQ(
      snapshot.last_update_id,
      100U
  );

  ASSERT_EQ(snapshot.bids.size(), 2U);
  ASSERT_EQ(snapshot.asks.size(), 2U);

  EXPECT_DOUBLE_EQ(
      snapshot.bids[0].price,
      64824.10
  );

  EXPECT_DOUBLE_EQ(
      snapshot.asks[0].quantity,
      1.50
  );
}

TEST(BinanceLevel2, ParsesDepthUpdate) {
  const auto update =
      parse_binance_depth_update(
          R"JSON(
{
  "e": "depthUpdate",
  "E": 2000,
  "s": "BTCUSDT",
  "U": 101,
  "u": 102,
  "b": [
    ["64824.15", "0.80"]
  ],
  "a": [
    ["64824.20", "0.00"]
  ]
}
)JSON",
          "BTCUSDT",
          3000
      );

  ASSERT_TRUE(update.valid)
      << update.error;

  EXPECT_FALSE(update.ignored);
  EXPECT_EQ(update.first_update_id, 101U);
  EXPECT_EQ(update.final_update_id, 102U);

  ASSERT_EQ(update.bids.size(), 1U);
  ASSERT_EQ(update.asks.size(), 1U);

  EXPECT_DOUBLE_EQ(
      update.asks[0].quantity,
      0.0
  );
}

TEST(BinanceLevel2, IgnoresOtherMessages) {
  const auto update =
      parse_binance_depth_update(
          R"JSON({"result":null,"id":1})JSON",
          "BTCUSDT",
          100
      );

  EXPECT_TRUE(update.valid);
  EXPECT_TRUE(update.ignored);
}

TEST(BinanceLevel2, BridgesSnapshot) {
  BinanceBookSynchronizer synchronizer(
      "BTC-USD"
  );

  const auto snapshot =
      parse_binance_depth_snapshot(
          R"JSON(
{
  "lastUpdateId": 100,
  "bids": [["100.00","1.0"]],
  "asks": [["101.00","1.0"]]
}
)JSON",
          "BTCUSDT",
          1000
      );

  EXPECT_TRUE(
      synchronizer
          .apply_snapshot(snapshot)
          .accepted
  );

  const auto update =
      parse_binance_depth_update(
          R"JSON(
{
  "e":"depthUpdate",
  "E":2000,
  "s":"BTCUSDT",
  "U":99,
  "u":101,
  "b":[["100.10","2.0"]],
  "a":[["101.00","0.0"]]
}
)JSON",
          "BTCUSDT",
          2000
      );

  const auto result =
      synchronizer.apply_update(update);

  ASSERT_TRUE(result.accepted)
      << result.message;

  EXPECT_TRUE(
      synchronizer.synchronized()
  );

  ASSERT_EQ(result.updates.size(), 2U);

  EXPECT_EQ(
      result.updates[0].side,
      MarketDataSide::Bid
  );

  EXPECT_EQ(
      result.updates[1].type,
      MarketDataUpdateType::Delete
  );
}

TEST(BinanceLevel2, IgnoresStaleUpdate) {
  BinanceBookSynchronizer synchronizer(
      "BTC-USD"
  );

  synchronizer.apply_snapshot(
      parse_binance_depth_snapshot(
          R"JSON(
{
  "lastUpdateId":100,
  "bids":[["100.00","1.0"]],
  "asks":[["101.00","1.0"]]
}
)JSON",
          "BTCUSDT",
          1000
      )
  );

  const auto result =
      synchronizer.apply_update(
          parse_binance_depth_update(
              R"JSON(
{
  "e":"depthUpdate",
  "E":2000,
  "s":"BTCUSDT",
  "U":90,
  "u":100,
  "b":[],
  "a":[]
}
)JSON",
              "BTCUSDT",
              2000
          )
      );

  EXPECT_TRUE(result.accepted);
  EXPECT_TRUE(result.ignored);
}

TEST(BinanceLevel2, DetectsSequenceGap) {
  BinanceBookSynchronizer synchronizer(
      "BTC-USD"
  );

  synchronizer.apply_snapshot(
      parse_binance_depth_snapshot(
          R"JSON(
{
  "lastUpdateId":100,
  "bids":[["100.00","1.0"]],
  "asks":[["101.00","1.0"]]
}
)JSON",
          "BTCUSDT",
          1000
      )
  );

  const auto result =
      synchronizer.apply_update(
          parse_binance_depth_update(
              R"JSON(
{
  "e":"depthUpdate",
  "E":2000,
  "s":"BTCUSDT",
  "U":105,
  "u":106,
  "b":[],
  "a":[]
}
)JSON",
              "BTCUSDT",
              2000
          )
      );

  EXPECT_FALSE(result.accepted);
  EXPECT_TRUE(result.snapshot_required);
  EXPECT_EQ(
      result.status,
      BinanceSyncStatus::Stale
  );
}

}  // namespace
