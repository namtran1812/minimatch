#include "minimatch/market_replay.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <vector>

namespace {

using minimatch::MarketEventType;
using minimatch::MarketReplay;
using minimatch::MarketReplayEvent;

std::filesystem::path replay_csv_path() {
  return std::filesystem::temp_directory_path() /
      "minimatch_market_replay_test.csv";
}

void write_csv(
    const std::string& contents
) {
  std::ofstream output(replay_csv_path());
  output << contents;
}

void remove_csv() {
  std::filesystem::remove(
      replay_csv_path()
  );
}

TEST(MarketReplay, SortsEventsByTimestamp) {
  MarketReplay replay(
      {
          MarketReplayEvent{
              .timestamp_ns = 300,
              .type = MarketEventType::Trade,
              .symbol = "btcusd",
              .venue = "coinbase",
              .trade_price = 101.0,
              .trade_quantity = 0.2
          },
          MarketReplayEvent{
              .timestamp_ns = 100,
              .type = MarketEventType::Quote,
              .symbol = "btcusd",
              .venue = "coinbase",
              .bid_price = 99.0,
              .bid_quantity = 1.0,
              .ask_price = 100.0,
              .ask_quantity = 1.0
          }
      }
  );

  ASSERT_EQ(replay.size(), 2U);
  EXPECT_EQ(
      replay.events()[0].timestamp_ns,
      100U
  );
  EXPECT_EQ(
      replay.events()[1].timestamp_ns,
      300U
  );
}

TEST(MarketReplay, IteratesDeterministically) {
  MarketReplay replay(
      {
          MarketReplayEvent{
              .timestamp_ns = 100,
              .type = MarketEventType::Quote,
              .symbol = "btcusd",
              .venue = "coinbase",
              .bid_price = 99.0,
              .bid_quantity = 1.0,
              .ask_price = 100.0,
              .ask_quantity = 1.0
          },
          MarketReplayEvent{
              .timestamp_ns = 200,
              .type = MarketEventType::Trade,
              .symbol = "btcusd",
              .venue = "coinbase",
              .trade_price = 100.0,
              .trade_quantity = 0.5
          }
      }
  );

  ASSERT_TRUE(replay.peek().has_value());
  EXPECT_EQ(replay.peek()->timestamp_ns, 100U);

  ASSERT_TRUE(replay.next().has_value());
  EXPECT_EQ(replay.position(), 1U);

  ASSERT_TRUE(replay.next().has_value());
  EXPECT_TRUE(replay.finished());
  EXPECT_FALSE(replay.next().has_value());

  replay.reset();

  EXPECT_EQ(replay.position(), 0U);
  EXPECT_FALSE(replay.finished());
}

TEST(MarketReplay, SeeksByIndex) {
  MarketReplay replay(
      {
          MarketReplayEvent{
              .timestamp_ns = 100,
              .type = MarketEventType::Trade,
              .symbol = "btcusd",
              .venue = "coinbase",
              .trade_price = 100.0,
              .trade_quantity = 1.0
          },
          MarketReplayEvent{
              .timestamp_ns = 200,
              .type = MarketEventType::Trade,
              .symbol = "btcusd",
              .venue = "coinbase",
              .trade_price = 101.0,
              .trade_quantity = 1.0
          }
      }
  );

  EXPECT_TRUE(replay.seek_index(1));
  EXPECT_EQ(replay.peek()->timestamp_ns, 200U);

  EXPECT_TRUE(replay.seek_index(2));
  EXPECT_TRUE(replay.finished());

  EXPECT_FALSE(replay.seek_index(3));
}

TEST(MarketReplay, SeeksByTimestamp) {
  MarketReplay replay(
      {
          MarketReplayEvent{
              .timestamp_ns = 100,
              .type = MarketEventType::Trade,
              .symbol = "btcusd",
              .venue = "coinbase",
              .trade_price = 100.0,
              .trade_quantity = 1.0
          },
          MarketReplayEvent{
              .timestamp_ns = 300,
              .type = MarketEventType::Trade,
              .symbol = "btcusd",
              .venue = "coinbase",
              .trade_price = 101.0,
              .trade_quantity = 1.0
          }
      }
  );

  EXPECT_TRUE(replay.seek_timestamp(200));
  EXPECT_EQ(replay.peek()->timestamp_ns, 300U);

  EXPECT_FALSE(replay.seek_timestamp(400));
  EXPECT_TRUE(replay.finished());
}

TEST(MarketReplay, CalculatesStatistics) {
  MarketReplay replay(
      {
          MarketReplayEvent{
              .timestamp_ns = 100,
              .type = MarketEventType::Quote,
              .symbol = "btcusd",
              .venue = "coinbase",
              .bid_price = 99.0,
              .bid_quantity = 1.0,
              .ask_price = 100.0,
              .ask_quantity = 1.0
          },
          MarketReplayEvent{
              .timestamp_ns = 200,
              .type = MarketEventType::Trade,
              .symbol = "btcusd",
              .venue = "coinbase",
              .trade_price = 100.0,
              .trade_quantity = 0.5
          }
      }
  );

  EXPECT_EQ(replay.stats().event_count, 2U);
  EXPECT_EQ(replay.stats().quote_count, 1U);
  EXPECT_EQ(replay.stats().trade_count, 1U);
  EXPECT_EQ(
      replay.stats().first_timestamp_ns,
      100U
  );
  EXPECT_EQ(
      replay.stats().last_timestamp_ns,
      200U
  );
}

TEST(MarketReplay, LoadsCsvFile) {
  write_csv(
      "timestamp_ns,type,symbol,venue,"
      "bid_price,bid_quantity,"
      "ask_price,ask_quantity,"
      "trade_price,trade_quantity\n"
      "100,QUOTE,btcusd,coinbase,"
      "99.0,1.0,100.0,2.0,,\n"
      "200,TRADE,btcusd,coinbase,"
      ",,,,100.0,0.5\n"
  );

  const auto replay =
      MarketReplay::from_csv(
          replay_csv_path().string()
      );

  ASSERT_EQ(replay.size(), 2U);
  EXPECT_EQ(
      replay.events()[0].type,
      MarketEventType::Quote
  );
  EXPECT_EQ(
      replay.events()[1].type,
      MarketEventType::Trade
  );

  remove_csv();
}

TEST(MarketReplay, RejectsInvalidCsvHeader) {
  write_csv(
      "bad,header\n"
      "1,2\n"
  );

  EXPECT_THROW(
      MarketReplay::from_csv(
          replay_csv_path().string()
      ),
      std::runtime_error
  );

  remove_csv();
}

TEST(MarketReplay, RejectsCrossedQuote) {
  EXPECT_THROW(
      MarketReplay(
          {
              MarketReplayEvent{
                  .timestamp_ns = 100,
                  .type = MarketEventType::Quote,
                  .symbol = "btcusd",
                  .venue = "coinbase",
                  .bid_price = 101.0,
                  .bid_quantity = 1.0,
                  .ask_price = 100.0,
                  .ask_quantity = 1.0
              }
          }
      ),
      std::invalid_argument
  );
}

}  // namespace
