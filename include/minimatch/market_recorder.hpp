#pragma once

#include "minimatch/market_data.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace minimatch {

enum class MarketRecordType : std::uint8_t {
  Snapshot = 1,
  UpdateBatch = 2
};

struct MarketRecord {
  MarketRecordType type{
      MarketRecordType::Snapshot
  };

  std::uint64_t recorded_timestamp_ns{0};

  MarketDataSnapshot snapshot;
  std::vector<MarketDataUpdate> updates;
};

struct MarketReplayOptions {
  // 0.0 means replay without sleeping.
  double speed{0.0};

  std::optional<std::uint64_t>
      start_timestamp_ns;

  std::optional<std::uint64_t>
      end_timestamp_ns;

  std::size_t maximum_records{0};
};

struct MarketReplayResult {
  std::size_t records_read{0};
  std::size_t records_applied{0};

  std::size_t snapshots_applied{0};
  std::size_t update_batches_applied{0};

  std::size_t rejected_records{0};

  std::uint64_t first_timestamp_ns{0};
  std::uint64_t last_timestamp_ns{0};

  std::uint64_t elapsed_wall_time_ns{0};

  double records_per_second{0.0};
};

class MarketRecorder {
 public:
  explicit MarketRecorder(
      const std::string& path,
      bool append = false
  );

  ~MarketRecorder();

  MarketRecorder(
      const MarketRecorder&
  ) = delete;

  MarketRecorder& operator=(
      const MarketRecorder&
  ) = delete;

  void write_snapshot(
      const MarketDataSnapshot& snapshot,
      std::uint64_t
          recorded_timestamp_ns = 0
  );

  void write_update_batch(
      const std::vector<
          MarketDataUpdate
      >& updates,
      std::uint64_t
          recorded_timestamp_ns = 0
  );

  // Compatibility with the earlier skeleton.
  void write_update(
      const std::vector<
          MarketDataUpdate
      >& updates
  );

  void flush();

  [[nodiscard]] std::size_t
  records_written() const;

  [[nodiscard]] std::uint64_t
  bytes_written() const;

 private:
  std::ofstream output_;

  std::size_t records_written_{0};
  std::uint64_t bytes_written_{0};

  void write_file_header();

  void write_record(
      MarketRecordType type,
      std::uint64_t recorded_timestamp_ns,
      const std::vector<std::byte>& payload
  );
};

class MarketRecordReader {
 public:
  explicit MarketRecordReader(
      const std::string& path
  );

  MarketRecordReader(
      const MarketRecordReader&
  ) = delete;

  MarketRecordReader& operator=(
      const MarketRecordReader&
  ) = delete;

  [[nodiscard]] bool next(
      MarketRecord& record
  );

  void rewind();

  [[nodiscard]] std::size_t
  records_read() const;

 private:
  std::ifstream input_;

  std::size_t records_read_{0};

  void read_file_header();
};

[[nodiscard]] MarketReplayResult
replay_market_records(
    MarketRecordReader& reader,
    ConsolidatedMarketData& market,
    const MarketReplayOptions& options = {}
);

[[nodiscard]] std::uint64_t
market_state_checksum(
    const ConsolidatedMarketData& market,
    const std::string& symbol,
    std::size_t depth = 20
);

}  // namespace minimatch
