#include "minimatch/kraken_l2.hpp"

#include <algorithm>

#include <gtest/gtest.h>

namespace {

using minimatch::KrakenBookNormalizer;
using minimatch::MarketDataSide;
using minimatch::MarketDataUpdateType;
using minimatch::build_kraken_subscription;
using minimatch::parse_kraken_book_message;

TEST(KrakenLevel2, BuildsSubscription) {
  const auto message =
      build_kraken_subscription(
          "BTC/USD",
          10
      );

  EXPECT_NE(
      message.find("\"channel\":\"book\""),
      std::string::npos
  );

  EXPECT_NE(
      message.find("\"BTC/USD\""),
      std::string::npos
  );

  EXPECT_NE(
      message.find("\"snapshot\":true"),
      std::string::npos
  );
}

TEST(KrakenLevel2, RejectsUnsupportedDepth) {
  EXPECT_THROW(
      build_kraken_subscription(
          "BTC/USD",
          20
      ),
      std::invalid_argument
  );
}

TEST(KrakenLevel2, IgnoresStatusMessage) {
  const auto result =
      parse_kraken_book_message(
          R"JSON(
{
  "channel": "status",
  "type": "update",
  "data": [
    {
      "system": "online"
    }
  ]
}
)JSON",
          "BTC/USD",
          1000
      );

  EXPECT_TRUE(result.valid);
  EXPECT_TRUE(result.ignored);
}

TEST(KrakenLevel2, ParsesSnapshot) {
  const auto result =
      parse_kraken_book_message(
          R"JSON(
{
  "channel": "book",
  "type": "snapshot",
  "data": [
    {
      "symbol": "BTC/USD",
      "bids": [
        {
          "price": 65001.4,
          "qty": 0.00076922
        },
        {
          "price": 65001.1,
          "qty": 0.000051
        }
      ],
      "asks": [
        {
          "price": 65001.5,
          "qty": 0.5269444
        }
      ],
      "checksum": 2459808650,
      "timestamp":
        "2026-07-15T16:52:41.220181Z"
    }
  ]
}
)JSON",
          "BTC/USD",
          2000
      );

  ASSERT_TRUE(result.valid)
      << result.error;

  EXPECT_FALSE(result.ignored);
  EXPECT_TRUE(result.snapshot);

  EXPECT_EQ(
      result.symbol,
      "BTC/USD"
  );

  EXPECT_EQ(
      result.checksum,
      2459808650U
  );

  ASSERT_EQ(result.bids.size(), 2U);
  ASSERT_EQ(result.asks.size(), 1U);

  EXPECT_DOUBLE_EQ(
      result.bids[0].price,
      65001.4
  );

  EXPECT_DOUBLE_EQ(
      result.asks[0].quantity,
      0.5269444
  );
}

TEST(KrakenLevel2, ParsesUpdate) {
  const auto result =
      parse_kraken_book_message(
          R"JSON(
{
  "channel": "book",
  "type": "update",
  "data": [
    {
      "symbol": "BTC/USD",
      "bids": [
        {
          "price": 64989.2,
          "qty": 0.91026849
        }
      ],
      "asks": [
        {
          "price": 65010.0,
          "qty": 0.0
        }
      ],
      "checksum": 2110840482,
      "timestamp":
        "2026-07-15T16:52:41.496492Z"
    }
  ]
}
)JSON",
          "BTC/USD",
          3000
      );

  ASSERT_TRUE(result.valid)
      << result.error;

  EXPECT_FALSE(result.snapshot);

  ASSERT_EQ(result.bids.size(), 1U);
  ASSERT_EQ(result.asks.size(), 1U);

  EXPECT_DOUBLE_EQ(
      result.asks[0].quantity,
      0.0
  );
}

TEST(KrakenLevel2, NormalizesSnapshotAndUpdate) {
  KrakenBookNormalizer normalizer(
      "BTC-USD"
  );

  const auto snapshot_message =
      parse_kraken_book_message(
          R"JSON(
{
  "channel":"book",
  "type":"snapshot",
  "data":[
    {
      "symbol":"BTC/USD",
      "bids":[
        {"price":100.0,"qty":1.0}
      ],
      "asks":[
        {"price":101.0,"qty":2.0}
      ],
      "checksum":0,
      "timestamp":"test"
    }
  ]
}
)JSON",
          "BTC/USD",
          1000
      );

  const auto snapshot =
      normalizer.normalize_snapshot(
          snapshot_message
      );

  EXPECT_EQ(snapshot.venue, "KRAKEN");
  EXPECT_EQ(snapshot.symbol, "BTC-USD");
  EXPECT_EQ(snapshot.sequence, 0U);
  EXPECT_TRUE(normalizer.synchronized());

  const auto update_message =
      parse_kraken_book_message(
          R"JSON(
{
  "channel":"book",
  "type":"update",
  "data":[
    {
      "symbol":"BTC/USD",
      "bids":[
        {"price":100.5,"qty":3.0}
      ],
      "asks":[
        {"price":101.0,"qty":0.0}
      ],
      "checksum":0,
      "timestamp":"test"
    }
  ]
}
)JSON",
          "BTC/USD",
          2000
      );

  const auto updates =
      normalizer.normalize_update(
          update_message
      );

  ASSERT_EQ(updates.size(), 2U);

  const auto bid_update =
      std::find_if(
          updates.begin(),
          updates.end(),
          [](
              const minimatch::
                  MarketDataUpdate& update
          ) {
            return
                update.side ==
                    MarketDataSide::Bid &&
                update.type ==
                    MarketDataUpdateType::
                        Upsert &&
                update.price == 100.5;
          }
      );

  const auto ask_delete =
      std::find_if(
          updates.begin(),
          updates.end(),
          [](
              const minimatch::
                  MarketDataUpdate& update
          ) {
            return
                update.side ==
                    MarketDataSide::Ask &&
                update.type ==
                    MarketDataUpdateType::
                        Delete &&
                update.price == 101.0;
          }
      );

  ASSERT_NE(
      bid_update,
      updates.end()
  );

  ASSERT_NE(
      ask_delete,
      updates.end()
  );

  EXPECT_EQ(
      bid_update->venue,
      "KRAKEN"
  );

  EXPECT_EQ(
      bid_update->symbol,
      "BTC-USD"
  );

  EXPECT_EQ(
      bid_update->sequence,
      1U
  );

  EXPECT_DOUBLE_EQ(
      bid_update->quantity,
      3.0
  );

  EXPECT_EQ(
      ask_delete->sequence,
      1U
  );

  EXPECT_DOUBLE_EQ(
      ask_delete->quantity,
      0.0
  );
}

TEST(KrakenLevel2, RequiresSnapshotBeforeUpdate) {
  KrakenBookNormalizer normalizer(
      "BTC-USD"
  );

  const auto update =
      parse_kraken_book_message(
          R"JSON(
{
  "channel":"book",
  "type":"update",
  "data":[
    {
      "symbol":"BTC/USD",
      "bids":[
        {"price":100.0,"qty":1.0}
      ],
      "asks":[],
      "checksum":1,
      "timestamp":"test"
    }
  ]
}
)JSON",
          "BTC/USD",
          1000
      );

  EXPECT_THROW(
      normalizer.normalize_update(update),
      std::runtime_error
  );
}

TEST(KrakenLevel2, RejectsSymbolMismatch) {
  const auto result =
      parse_kraken_book_message(
          R"JSON(
{
  "channel":"book",
  "type":"snapshot",
  "data":[
    {
      "symbol":"ETH/USD",
      "bids":[],
      "asks":[],
      "checksum":1,
      "timestamp":"test"
    }
  ]
}
)JSON",
          "BTC/USD",
          1000
      );

  EXPECT_FALSE(result.valid);
  EXPECT_EQ(
      result.error,
      "Kraken symbol mismatch"
  );
}

}  // namespace
