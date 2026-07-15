#include "minimatch/market_recorder.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

namespace {

using minimatch::ConsolidatedMarketData;
using minimatch::MarketDataLevel;
using minimatch::MarketDataSide;
using minimatch::MarketDataSnapshot;
using minimatch::MarketDataUpdate;
using minimatch::MarketDataUpdateType;
using minimatch::MarketRecord;
using minimatch::MarketRecordReader;
using minimatch::MarketRecorder;
using minimatch::MarketReplayOptions;
using minimatch::MarketRecordType;
using minimatch::market_state_checksum;
using minimatch::replay_market_records;

std::filesystem::path recording_path() {
  return
      std::filesystem::temp_directory_path() /
      "minimatch_market_record_test.bin";
}

void remove_recording() {
  std::filesystem::remove(
      recording_path()
  );
}

MarketDataSnapshot sample_snapshot() {
  return MarketDataSnapshot{
      .venue = "COINBASE",
      .symbol = "BTC-USD",
      .sequence = 0,
      .timestamp_ns = 1'000,
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

std::vector<MarketDataUpdate>
sample_updates() {
  return {
      MarketDataUpdate{
          .venue = "COINBASE",
          .symbol = "BTC-USD",
          .sequence = 1,
          .timestamp_ns = 2'000,
          .side = MarketDataSide::Bid,
          .type =
              MarketDataUpdateType::Upsert,
          .price = 100.25,
          .quantity = 1.5
      },
      MarketDataUpdate{
          .venue = "COINBASE",
          .symbol = "BTC-USD",
          .sequence = 1,
          .timestamp_ns = 2'000,
          .side = MarketDataSide::Ask,
          .type =
              MarketDataUpdateType::Delete,
          .price = 100.5,
          .quantity = 0.0
      },
      MarketDataUpdate{
          .venue = "COINBASE",
          .symbol = "BTC-USD",
          .sequence = 1,
          .timestamp_ns = 2'000,
          .side = MarketDataSide::Ask,
          .type =
              MarketDataUpdateType::Upsert,
          .price = 100.4,
          .quantity = 2.0
      }
  };
}

TEST(MarketRecorder, WritesAndReadsSnapshot) {
  remove_recording();

  {
    MarketRecorder recorder(
        recording_path().string()
    );

    recorder.write_snapshot(
        sample_snapshot(),
        10'000
    );

    recorder.flush();

    EXPECT_EQ(
        recorder.records_written(),
        1U
    );

    EXPECT_GT(
        recorder.bytes_written(),
        0U
    );
  }

  MarketRecordReader reader(
      recording_path().string()
  );

  MarketRecord record;

  ASSERT_TRUE(reader.next(record));

  EXPECT_EQ(
      record.type,
      MarketRecordType::Snapshot
  );

  EXPECT_EQ(
      record.recorded_timestamp_ns,
      10'000U
  );

  EXPECT_EQ(
      record.snapshot.venue,
      "COINBASE"
  );

  EXPECT_EQ(
      record.snapshot.symbol,
      "BTC-USD"
  );

  ASSERT_EQ(
      record.snapshot.bids.size(),
      2U
  );

  EXPECT_DOUBLE_EQ(
      record.snapshot.bids[0].price,
      100.0
  );

  EXPECT_FALSE(reader.next(record));

  remove_recording();
}

TEST(MarketRecorder, WritesAndReadsUpdateBatch) {
  remove_recording();

  {
    MarketRecorder recorder(
        recording_path().string()
    );

    recorder.write_update_batch(
        sample_updates(),
        20'000
    );
  }

  MarketRecordReader reader(
      recording_path().string()
  );

  MarketRecord record;

  ASSERT_TRUE(reader.next(record));

  EXPECT_EQ(
      record.type,
      MarketRecordType::UpdateBatch
  );

  ASSERT_EQ(
      record.updates.size(),
      3U
  );

  EXPECT_EQ(
      record.updates[1].type,
      MarketDataUpdateType::Delete
  );

  EXPECT_DOUBLE_EQ(
      record.updates[2].price,
      100.4
  );

  remove_recording();
}

TEST(MarketRecorder, ReplaysDeterministically) {
  remove_recording();

  {
    MarketRecorder recorder(
        recording_path().string()
    );

    recorder.write_snapshot(
        sample_snapshot(),
        1'000
    );

    recorder.write_update_batch(
        sample_updates(),
        2'000
    );
  }

  ConsolidatedMarketData first_market;

  MarketRecordReader first_reader(
      recording_path().string()
  );

  const auto first_result =
      replay_market_records(
          first_reader,
          first_market,
          MarketReplayOptions{
              .speed = 0.0
          }
      );

  EXPECT_EQ(
      first_result.records_applied,
      2U
  );

  EXPECT_EQ(
      first_result.snapshots_applied,
      1U
  );

  EXPECT_EQ(
      first_result.update_batches_applied,
      1U
  );

  EXPECT_EQ(
      first_result.rejected_records,
      0U
  );

  const auto first_checksum =
      market_state_checksum(
          first_market,
          "BTC-USD"
      );

  ConsolidatedMarketData second_market;

  MarketRecordReader second_reader(
      recording_path().string()
  );

  const auto second_result =
      replay_market_records(
          second_reader,
          second_market
      );

  EXPECT_EQ(
      second_result.records_applied,
      2U
  );

  const auto second_checksum =
      market_state_checksum(
          second_market,
          "BTC-USD"
      );

  EXPECT_EQ(
      first_checksum,
      second_checksum
  );

  const auto book =
      first_market.find_book(
          "COINBASE",
          "BTC-USD"
      );

  ASSERT_TRUE(book.has_value());
  ASSERT_TRUE(book->best_bid().has_value());
  ASSERT_TRUE(book->best_ask().has_value());

  EXPECT_DOUBLE_EQ(
      book->best_bid()->price,
      100.25
  );

  EXPECT_DOUBLE_EQ(
      book->best_ask()->price,
      100.4
  );

  remove_recording();
}

TEST(MarketRecorder, SupportsTimestampRange) {
  remove_recording();

  {
    MarketRecorder recorder(
        recording_path().string()
    );

    recorder.write_snapshot(
        sample_snapshot(),
        1'000
    );

    recorder.write_update_batch(
        sample_updates(),
        2'000
    );
  }

  ConsolidatedMarketData market;

  MarketRecordReader reader(
      recording_path().string()
  );

  const auto result =
      replay_market_records(
          reader,
          market,
          MarketReplayOptions{
              .speed = 0.0,
              .start_timestamp_ns = 2'000
          }
      );

  EXPECT_EQ(result.records_read, 2U);
  EXPECT_EQ(result.records_applied, 0U);
  EXPECT_EQ(result.rejected_records, 1U);

  remove_recording();
}

TEST(MarketRecorder, RewindsReader) {
  remove_recording();

  {
    MarketRecorder recorder(
        recording_path().string()
    );

    recorder.write_snapshot(
        sample_snapshot()
    );
  }

  MarketRecordReader reader(
      recording_path().string()
  );

  MarketRecord record;

  ASSERT_TRUE(reader.next(record));
  EXPECT_FALSE(reader.next(record));

  reader.rewind();

  EXPECT_EQ(reader.records_read(), 0U);
  EXPECT_TRUE(reader.next(record));

  remove_recording();
}

TEST(MarketRecorder, RejectsCorruptHeader) {
  remove_recording();

  {
    std::ofstream output(
        recording_path(),
        std::ios::binary
    );

    output << "NOT-A-MARKET-FILE";
  }

  EXPECT_THROW(
      MarketRecordReader(
          recording_path().string()
      ),
      std::runtime_error
  );

  remove_recording();
}

}  // namespace
