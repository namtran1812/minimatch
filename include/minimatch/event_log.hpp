#pragma once
#include "minimatch/types.hpp"
#include <cstdint>
#include <fstream>
#include <string>
#include <variant>

namespace minimatch {
enum class EventKind : std::uint8_t { Submit = 1, Cancel = 2, Replace = 3 };
using InputEvent = std::variant<OrderRequest, CancelRequest, ReplaceRequest>;

#pragma pack(push, 1)
struct LogHeader {
  std::uint32_t magic{0x4D4D4C47};
  std::uint16_t version{2};
  EventKind kind{};
  std::uint8_t reserved{};
  std::uint32_t payload_size{};
  std::uint64_t event_sequence{};
  std::uint64_t checksum{};
};
#pragma pack(pop)

class EventLogWriter {
 public:
  explicit EventLogWriter(const std::string& path);
  void append(const OrderRequest& request);
  void append(const CancelRequest& request);
  void append(const ReplaceRequest& request);
  void flush();
  [[nodiscard]] std::uint64_t event_sequence() const noexcept { return event_sequence_; }
 private:
  template <class T>
  void write(EventKind kind, const T& value);
  std::ofstream out_;
  std::uint64_t event_sequence_{0};
};

class EventLogReader {
 public:
  explicit EventLogReader(const std::string& path);
  bool next(InputEvent& event);
  [[nodiscard]] std::uint64_t last_sequence() const noexcept { return last_sequence_; }
 private:
  std::ifstream in_;
  std::uint64_t last_sequence_{0};
};
}  // namespace minimatch
