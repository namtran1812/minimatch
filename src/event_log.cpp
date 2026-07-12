#include "minimatch/event_log.hpp"
#include <stdexcept>

namespace minimatch {
namespace {
std::uint64_t checksum(const void* data, std::size_t size) {
  const auto* bytes = static_cast<const unsigned char*>(data);
  std::uint64_t hash = 1469598103934665603ULL;
  for (std::size_t i = 0; i < size; ++i) {
    hash ^= bytes[i];
    hash *= 1099511628211ULL;
  }
  return hash;
}
}

EventLogWriter::EventLogWriter(const std::string& path)
    : out_(path, std::ios::binary | std::ios::app) {
  if (!out_) throw std::runtime_error("cannot open event log");
}

template <class T>
void EventLogWriter::write(EventKind kind, const T& value) {
  LogHeader header;
  header.kind = kind;
  header.payload_size = sizeof(T);
  header.event_sequence = ++event_sequence_;
  header.checksum = checksum(&value, sizeof(T));
  out_.write(reinterpret_cast<const char*>(&header), sizeof(header));
  out_.write(reinterpret_cast<const char*>(&value), sizeof(value));
  if (!out_) throw std::runtime_error("event log write failed");
}

void EventLogWriter::append(const OrderRequest& request) { write(EventKind::Submit, request); }
void EventLogWriter::append(const CancelRequest& request) { write(EventKind::Cancel, request); }
void EventLogWriter::append(const ReplaceRequest& request) { write(EventKind::Replace, request); }
void EventLogWriter::flush() { out_.flush(); }

EventLogReader::EventLogReader(const std::string& path) : in_(path, std::ios::binary) {
  if (!in_) throw std::runtime_error("cannot open event log");
}

bool EventLogReader::next(InputEvent& event) {
  LogHeader header{};
  if (!in_.read(reinterpret_cast<char*>(&header), sizeof(header))) return false;
  if (header.magic != 0x4D4D4C47 || header.version != 2 ||
      header.event_sequence != last_sequence_ + 1) {
    throw std::runtime_error("bad or non-contiguous log header");
  }
  last_sequence_ = header.event_sequence;

  auto read_checked = [&](auto& value) {
    in_.read(reinterpret_cast<char*>(&value), sizeof(value));
    if (!in_ || checksum(&value, sizeof(value)) != header.checksum) {
      throw std::runtime_error("log checksum mismatch");
    }
  };

  if (header.kind == EventKind::Submit && header.payload_size == sizeof(OrderRequest)) {
    OrderRequest value{};
    read_checked(value);
    event = value;
    return true;
  }
  if (header.kind == EventKind::Cancel && header.payload_size == sizeof(CancelRequest)) {
    CancelRequest value{};
    read_checked(value);
    event = value;
    return true;
  }
  if (header.kind == EventKind::Replace && header.payload_size == sizeof(ReplaceRequest)) {
    ReplaceRequest value{};
    read_checked(value);
    event = value;
    return true;
  }
  throw std::runtime_error("unknown log event");
}
}  // namespace minimatch
