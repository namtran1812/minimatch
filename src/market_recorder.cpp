#include "minimatch/market_recorder.hpp"

#include <iostream>

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace minimatch {

namespace {

constexpr std::array<char, 8> file_magic{
    'M', 'M', 'D', 'A', 'T', 'A', '0', '1'
};

constexpr std::uint16_t file_version = 1;
constexpr std::uint32_t maximum_string_size =
    1024 * 1024;

constexpr std::uint32_t maximum_level_count =
    10'000'000;

std::uint64_t now_ns() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<
          std::chrono::nanoseconds
      >(
          std::chrono::system_clock::now()
              .time_since_epoch()
      ).count()
  );
}

template <typename Value>
void append_scalar(
    std::vector<std::byte>& output,
    Value value
) {
  static_assert(
      std::is_trivially_copyable_v<Value>
  );

  const auto* bytes =
      reinterpret_cast<const std::byte*>(
          &value
      );

  output.insert(
      output.end(),
      bytes,
      bytes + sizeof(Value)
  );
}

void append_string(
    std::vector<std::byte>& output,
    const std::string& value
) {
  if (value.size() >
      maximum_string_size) {
    throw std::invalid_argument(
        "market-data string exceeds maximum size"
    );
  }

  append_scalar(
      output,
      static_cast<std::uint32_t>(
          value.size()
      )
  );

  const auto* bytes =
      reinterpret_cast<const std::byte*>(
          value.data()
      );

  output.insert(
      output.end(),
      bytes,
      bytes + value.size()
  );
}

void append_level(
    std::vector<std::byte>& output,
    const MarketDataLevel& level
) {
  append_scalar(output, level.price);
  append_scalar(output, level.quantity);
}

void append_levels(
    std::vector<std::byte>& output,
    const std::vector<
        MarketDataLevel
    >& levels
) {
  if (levels.size() >
      maximum_level_count) {
    throw std::invalid_argument(
        "market-data level count exceeds maximum"
    );
  }

  append_scalar(
      output,
      static_cast<std::uint32_t>(
          levels.size()
      )
  );

  for (const auto& level : levels) {
    append_level(output, level);
  }
}

void append_update(
    std::vector<std::byte>& output,
    const MarketDataUpdate& update
) {
  append_string(output, update.venue);
  append_string(output, update.symbol);

  append_scalar(output, update.sequence);
  append_scalar(output, update.timestamp_ns);

  append_scalar(
      output,
      static_cast<std::uint8_t>(
          update.side
      )
  );

  append_scalar(
      output,
      static_cast<std::uint8_t>(
          update.type
      )
  );

  append_scalar(output, update.price);
  append_scalar(output, update.quantity);
}

std::vector<std::byte>
encode_snapshot(
    const MarketDataSnapshot& snapshot
) {
  std::vector<std::byte> payload;

  payload.reserve(
      128 +
      (
          snapshot.bids.size() +
          snapshot.asks.size()
      ) *
          sizeof(MarketDataLevel)
  );

  append_string(payload, snapshot.venue);
  append_string(payload, snapshot.symbol);

  append_scalar(payload, snapshot.sequence);
  append_scalar(
      payload,
      snapshot.timestamp_ns
  );

  append_levels(payload, snapshot.bids);
  append_levels(payload, snapshot.asks);

  return payload;
}

std::vector<std::byte>
encode_update_batch(
    const std::vector<
        MarketDataUpdate
    >& updates
) {
  if (updates.empty()) {
    throw std::invalid_argument(
        "cannot record an empty market-data batch"
    );
  }

  if (updates.size() >
      maximum_level_count) {
    throw std::invalid_argument(
        "market-data batch exceeds maximum size"
    );
  }

  std::vector<std::byte> payload;

  append_scalar(
      payload,
      static_cast<std::uint32_t>(
          updates.size()
      )
  );

  for (const auto& update : updates) {
    append_update(payload, update);
  }

  return payload;
}

template <typename Value>
Value read_scalar(
    const std::vector<std::byte>& input,
    std::size_t& offset
) {
  static_assert(
      std::is_trivially_copyable_v<Value>
  );

  if (
      offset + sizeof(Value) >
      input.size()
  ) {
    throw std::runtime_error(
        "truncated market-data payload"
    );
  }

  Value value{};

  std::memcpy(
      &value,
      input.data() + offset,
      sizeof(Value)
  );

  offset += sizeof(Value);

  return value;
}

std::string read_string(
    const std::vector<std::byte>& input,
    std::size_t& offset
) {
  const auto size =
      read_scalar<std::uint32_t>(
          input,
          offset
      );

  if (size > maximum_string_size ||
      offset + size > input.size()) {
    throw std::runtime_error(
        "invalid market-data string length"
    );
  }

  const auto* begin =
      reinterpret_cast<const char*>(
          input.data() + offset
      );

  std::string value(
      begin,
      begin + size
  );

  offset += size;

  return value;
}

MarketDataLevel read_level(
    const std::vector<std::byte>& input,
    std::size_t& offset
) {
  return MarketDataLevel{
      .price =
          read_scalar<double>(
              input,
              offset
          ),
      .quantity =
          read_scalar<double>(
              input,
              offset
          )
  };
}

std::vector<MarketDataLevel>
read_levels(
    const std::vector<std::byte>& input,
    std::size_t& offset
) {
  const auto count =
      read_scalar<std::uint32_t>(
          input,
          offset
      );

  if (count > maximum_level_count) {
    throw std::runtime_error(
        "invalid market-data level count"
    );
  }

  std::vector<MarketDataLevel> levels;
  levels.reserve(count);

  for (std::uint32_t index = 0;
       index < count;
       ++index) {
    levels.push_back(
        read_level(input, offset)
    );
  }

  return levels;
}

MarketDataUpdate read_update(
    const std::vector<std::byte>& input,
    std::size_t& offset
) {
  const std::string venue =
      read_string(input, offset);

  const std::string symbol =
      read_string(input, offset);

  const std::uint64_t sequence =
      read_scalar<std::uint64_t>(
          input,
          offset
      );

  const std::uint64_t timestamp_ns =
      read_scalar<std::uint64_t>(
          input,
          offset
      );

  const auto side_value =
      read_scalar<std::uint8_t>(
          input,
          offset
      );

  const auto type_value =
      read_scalar<std::uint8_t>(
          input,
          offset
      );

  if (
      side_value >
      static_cast<std::uint8_t>(
          MarketDataSide::Ask
      )
  ) {
    throw std::runtime_error(
        "invalid market-data side"
    );
  }

  if (
      type_value >
      static_cast<std::uint8_t>(
          MarketDataUpdateType::Delete
      )
  ) {
    throw std::runtime_error(
        "invalid market-data update type"
    );
  }

  return MarketDataUpdate{
      .venue = venue,
      .symbol = symbol,
      .sequence = sequence,
      .timestamp_ns = timestamp_ns,
      .side =
          static_cast<MarketDataSide>(
              side_value
          ),
      .type =
          static_cast<
              MarketDataUpdateType
          >(type_value),
      .price =
          read_scalar<double>(
              input,
              offset
          ),
      .quantity =
          read_scalar<double>(
              input,
              offset
          )
  };
}

MarketDataSnapshot decode_snapshot(
    const std::vector<std::byte>& payload
) {
  std::size_t offset = 0;

  MarketDataSnapshot snapshot{
      .venue =
          read_string(
              payload,
              offset
          ),
      .symbol =
          read_string(
              payload,
              offset
          ),
      .sequence =
          read_scalar<std::uint64_t>(
              payload,
              offset
          ),
      .timestamp_ns =
          read_scalar<std::uint64_t>(
              payload,
              offset
          ),
      .bids =
          read_levels(
              payload,
              offset
          ),
      .asks =
          read_levels(
              payload,
              offset
          )
  };

  if (offset != payload.size()) {
    throw std::runtime_error(
        "snapshot payload has trailing bytes"
    );
  }

  return snapshot;
}

std::vector<MarketDataUpdate>
decode_update_batch(
    const std::vector<std::byte>& payload
) {
  std::size_t offset = 0;

  const auto count =
      read_scalar<std::uint32_t>(
          payload,
          offset
      );

  if (count == 0 ||
      count > maximum_level_count) {
    throw std::runtime_error(
        "invalid update-batch count"
    );
  }

  std::vector<MarketDataUpdate> updates;
  updates.reserve(count);

  for (std::uint32_t index = 0;
       index < count;
       ++index) {
    updates.push_back(
        read_update(payload, offset)
    );
  }

  if (offset != payload.size()) {
    throw std::runtime_error(
        "update payload has trailing bytes"
    );
  }

  return updates;
}

template <typename Value>
void write_stream_value(
    std::ostream& output,
    Value value
) {
  output.write(
      reinterpret_cast<const char*>(
          &value
      ),
      static_cast<std::streamsize>(
          sizeof(Value)
      )
  );

  if (!output) {
    throw std::runtime_error(
        "failed writing market-data recording"
    );
  }
}

template <typename Value>
Value read_stream_value(
    std::istream& input
) {
  Value value{};

  input.read(
      reinterpret_cast<char*>(&value),
      static_cast<std::streamsize>(
          sizeof(Value)
      )
  );

  if (!input) {
    throw std::runtime_error(
        "truncated market-data recording"
    );
  }

  return value;
}

void hash_bytes(
    std::uint64_t& hash,
    const void* data,
    std::size_t size
) {
  constexpr std::uint64_t prime =
      1099511628211ULL;

  const auto* bytes =
      static_cast<const unsigned char*>(
          data
      );

  for (std::size_t index = 0;
       index < size;
       ++index) {
    hash ^= bytes[index];
    hash *= prime;
  }
}

template <typename Value>
void hash_value(
    std::uint64_t& hash,
    const Value& value
) {
  hash_bytes(
      hash,
      &value,
      sizeof(Value)
  );
}

void hash_string(
    std::uint64_t& hash,
    const std::string& value
) {
  hash_bytes(
      hash,
      value.data(),
      value.size()
  );
}

}  // namespace

MarketRecorder::MarketRecorder(
    const std::string& path,
    bool append
) {
  if (path.empty()) {
    throw std::invalid_argument(
        "market recording path cannot be empty"
    );
  }

  const auto mode =
      std::ios::binary |
      std::ios::out |
      (
          append
              ? std::ios::app
              : std::ios::trunc
      );

  output_.open(path, mode);

  if (!output_) {
    throw std::runtime_error(
        "cannot open market recording: " +
        path
    );
  }

  if (!append) {
    write_file_header();
  } else {
    const auto position =
        output_.tellp();

    if (position == std::streampos(0)) {
      write_file_header();
    }
  }
}

MarketRecorder::~MarketRecorder() {
  try {
    flush();
  } catch (...) {
  }
}

void MarketRecorder::write_file_header() {
  output_.write(
      file_magic.data(),
      static_cast<std::streamsize>(
          file_magic.size()
      )
  );

  write_stream_value(
      output_,
      file_version
  );

  const std::uint16_t reserved = 0;

  write_stream_value(
      output_,
      reserved
  );

  bytes_written_ +=
      file_magic.size() +
      sizeof(file_version) +
      sizeof(reserved);
}

void MarketRecorder::write_record(
    MarketRecordType type,
    std::uint64_t recorded_timestamp_ns,
    const std::vector<std::byte>& payload
) {
  if (payload.size() >
      std::numeric_limits<
          std::uint32_t
      >::max()) {
    throw std::invalid_argument(
        "market-data record payload is too large"
    );
  }

  const auto type_value =
      static_cast<std::uint8_t>(type);

  const std::array<std::uint8_t, 3>
      reserved{0, 0, 0};

  const auto payload_size =
      static_cast<std::uint32_t>(
          payload.size()
      );

  write_stream_value(
      output_,
      type_value
  );

  output_.write(
      reinterpret_cast<const char*>(
          reserved.data()
      ),
      static_cast<std::streamsize>(
          reserved.size()
      )
  );

  write_stream_value(
      output_,
      recorded_timestamp_ns
  );

  write_stream_value(
      output_,
      payload_size
  );

  output_.write(
      reinterpret_cast<const char*>(
          payload.data()
      ),
      static_cast<std::streamsize>(
          payload.size()
      )
  );

  if (!output_) {
    throw std::runtime_error(
        "failed writing market-data payload"
    );
  }

  ++records_written_;

  bytes_written_ +=
      sizeof(type_value) +
      reserved.size() +
      sizeof(recorded_timestamp_ns) +
      sizeof(payload_size) +
      payload.size();
}

void MarketRecorder::write_snapshot(
    const MarketDataSnapshot& snapshot,
    std::uint64_t recorded_timestamp_ns
) {
  if (recorded_timestamp_ns == 0) {
    recorded_timestamp_ns =
        snapshot.timestamp_ns != 0
            ? snapshot.timestamp_ns
            : now_ns();
  }

  write_record(
      MarketRecordType::Snapshot,
      recorded_timestamp_ns,
      encode_snapshot(snapshot)
  );
}

void MarketRecorder::write_update_batch(
    const std::vector<
        MarketDataUpdate
    >& updates,
    std::uint64_t recorded_timestamp_ns
) {
  if (updates.empty()) {
    throw std::invalid_argument(
        "cannot record empty market-data batch"
    );
  }

  if (recorded_timestamp_ns == 0) {
    recorded_timestamp_ns =
        updates.front().timestamp_ns != 0
            ? updates.front().timestamp_ns
            : now_ns();
  }

  write_record(
      MarketRecordType::UpdateBatch,
      recorded_timestamp_ns,
      encode_update_batch(updates)
  );
}

void MarketRecorder::write_update(
    const std::vector<
        MarketDataUpdate
    >& updates
) {
  write_update_batch(updates);
}

void MarketRecorder::flush() {
  if (!output_.is_open()) {
    return;
  }

  output_.flush();

  if (!output_) {
    throw std::runtime_error(
        "failed flushing market recording"
    );
  }
}

std::size_t
MarketRecorder::records_written() const {
  return records_written_;
}

std::uint64_t
MarketRecorder::bytes_written() const {
  return bytes_written_;
}

MarketRecordReader::MarketRecordReader(
    const std::string& path
) {
  if (path.empty()) {
    throw std::invalid_argument(
        "market recording path cannot be empty"
    );
  }

  input_.open(
      path,
      std::ios::binary
  );

  if (!input_) {
    throw std::runtime_error(
        "cannot open market recording: " +
        path
    );
  }

  read_file_header();
}

void MarketRecordReader::read_file_header() {
  std::array<char, 8> magic{};

  input_.read(
      magic.data(),
      static_cast<std::streamsize>(
          magic.size()
      )
  );

  if (!input_ || magic != file_magic) {
    throw std::runtime_error(
        "invalid market recording header"
    );
  }

  const auto version =
      read_stream_value<
          std::uint16_t
      >(input_);

  static_cast<void>(
      read_stream_value<
          std::uint16_t
      >(input_)
  );

  if (version != file_version) {
    throw std::runtime_error(
        "unsupported market recording version"
    );
  }
}

bool MarketRecordReader::next(
    MarketRecord& record
) {
  const int next_byte = input_.peek();

  if (next_byte ==
      std::char_traits<char>::eof()) {
    return false;
  }

  const auto type_value =
      read_stream_value<
          std::uint8_t
      >(input_);

  std::array<std::uint8_t, 3>
      reserved{};

  input_.read(
      reinterpret_cast<char*>(
          reserved.data()
      ),
      static_cast<std::streamsize>(
          reserved.size()
      )
  );

  if (!input_) {
    throw std::runtime_error(
        "truncated market-data record header"
    );
  }

  const auto timestamp_ns =
      read_stream_value<
          std::uint64_t
      >(input_);

  const auto payload_size =
      read_stream_value<
          std::uint32_t
      >(input_);

  std::vector<std::byte> payload(
      payload_size
  );

  input_.read(
      reinterpret_cast<char*>(
          payload.data()
      ),
      static_cast<std::streamsize>(
          payload.size()
      )
  );

  if (!input_) {
    

if (input_.eof()) {
      // A recorder process may terminate while writing the final
      // record. All records before this point are complete and
      // replayable, so treat an incomplete tail payload as EOF.
      input_.clear();
      return false;
    }

    throw std::runtime_error(
        "truncated market-data record payload"
    );
  }

  record = MarketRecord{};
  record.recorded_timestamp_ns =
      timestamp_ns;

  if (
      type_value ==
      static_cast<std::uint8_t>(
          MarketRecordType::Snapshot
      )
  ) {
    record.type =
        MarketRecordType::Snapshot;

    record.snapshot =
        decode_snapshot(payload);
  } else if (
      type_value ==
      static_cast<std::uint8_t>(
          MarketRecordType::UpdateBatch
      )
  ) {
    record.type =
        MarketRecordType::UpdateBatch;

    record.updates =
        decode_update_batch(payload);
  } else {
    throw std::runtime_error(
        "unsupported market-data record type"
    );
  }

  ++records_read_;

  return true;
}

void MarketRecordReader::rewind() {
  input_.clear();
  input_.seekg(0);

  if (!input_) {
    throw std::runtime_error(
        "cannot rewind market recording"
    );
  }

  records_read_ = 0;
  read_file_header();
}

std::size_t
MarketRecordReader::records_read() const {
  return records_read_;
}

MarketReplayResult replay_market_records(
    MarketRecordReader& reader,
    ConsolidatedMarketData& market,
    const MarketReplayOptions& options
) {
  if (!std::isfinite(options.speed) ||
      options.speed < 0.0) {
    throw std::invalid_argument(
        "replay speed must be non-negative"
    );
  }

  MarketReplayResult result;

  MarketRecord record;

  std::uint64_t previous_timestamp = 0;

  const auto wall_start =
      std::chrono::steady_clock::now();

  while (reader.next(record)) {
    ++result.records_read;

    if (
        options.start_timestamp_ns
            .has_value() &&
        record.recorded_timestamp_ns <
            *options.start_timestamp_ns
    ) {
      continue;
    }

    if (
        options.end_timestamp_ns
            .has_value() &&
        record.recorded_timestamp_ns >
            *options.end_timestamp_ns
    ) {
      break;
    }

    if (
        options.maximum_records > 0 &&
        result.records_applied >=
            options.maximum_records
    ) {
      break;
    }

    if (
        options.speed > 0.0 &&
        previous_timestamp > 0 &&
        record.recorded_timestamp_ns >
            previous_timestamp
    ) {
      const auto recorded_delta =
          record.recorded_timestamp_ns -
          previous_timestamp;

      const auto scaled_delta =
          static_cast<std::uint64_t>(
              static_cast<double>(
                  recorded_delta
              ) /
              options.speed
          );

      std::this_thread::sleep_for(
          std::chrono::nanoseconds(
              scaled_delta
          )
      );
    }

    MarketDataDecision decision;

    if (
        record.type ==
        MarketRecordType::Snapshot
    ) {
      decision =
          market.apply_snapshot(
              record.snapshot
          );

      if (decision.accepted) {
        ++result.snapshots_applied;
      }
    } else {
      decision =
          market.apply_batch(
              record.updates
          );

      if (decision.accepted) {
        ++result.update_batches_applied;
      }
    }

    if (!decision.accepted) {
      ++result.rejected_records;
    } else {
      ++result.records_applied;

      if (result.first_timestamp_ns == 0) {
        result.first_timestamp_ns =
            record.recorded_timestamp_ns;
      }

      result.last_timestamp_ns =
          record.recorded_timestamp_ns;
    }

    previous_timestamp =
        record.recorded_timestamp_ns;
  }

  const auto wall_end =
      std::chrono::steady_clock::now();

  result.elapsed_wall_time_ns =
      static_cast<std::uint64_t>(
          std::chrono::duration_cast<
              std::chrono::nanoseconds
          >(
              wall_end - wall_start
          ).count()
      );

  if (result.elapsed_wall_time_ns > 0) {
    result.records_per_second =
        static_cast<double>(
            result.records_applied
        ) /
        (
            static_cast<double>(
                result.elapsed_wall_time_ns
            ) /
            1'000'000'000.0
        );
  }

  return result;
}

std::uint64_t market_state_checksum(
    const ConsolidatedMarketData& market,
    const std::string& symbol,
    std::size_t depth
) {
  constexpr std::uint64_t offset_basis =
      14695981039346656037ULL;

  std::uint64_t hash = offset_basis;

  const auto venues =
      market.venues_for_symbol(symbol);

  for (const auto& venue : venues) {
    hash_string(hash, venue);
    hash_string(hash, symbol);

    const auto book =
        market.find_book(
            venue,
            symbol
        );

    if (!book.has_value()) {
      continue;
    }

    const auto sequence =
        book->sequence();

    const auto timestamp =
        book->timestamp_ns();

    const bool synchronized =
        book->synchronized();

    hash_value(hash, sequence);
    hash_value(hash, timestamp);
    hash_value(hash, synchronized);

    const auto bids = book->bids(depth);
    const auto asks = book->asks(depth);

    for (const auto& level : bids) {
      const std::uint8_t side = 0;
      hash_value(hash, side);
      hash_value(hash, level.price);
      hash_value(hash, level.quantity);
    }

    for (const auto& level : asks) {
      const std::uint8_t side = 1;
      hash_value(hash, side);
      hash_value(hash, level.price);
      hash_value(hash, level.quantity);
    }
  }

  return hash;
}

}  // namespace minimatch
