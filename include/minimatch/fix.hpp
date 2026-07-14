#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace minimatch {

constexpr char FIX_SOH = '\x01';

enum class FixMessageType {
  Logon,
  Logout,
  Heartbeat,
  TestRequest,
  NewOrderSingle,
  OrderCancelRequest,
  ExecutionReport,
  Reject,
  Unknown
};

struct FixField {
  int tag{0};
  std::string value;
};

struct FixMessage {
  std::string begin_string{"FIX.4.4"};
  int body_length{0};
  FixMessageType message_type{
      FixMessageType::Unknown
  };

  std::vector<FixField> fields;
  int checksum{0};

  [[nodiscard]] std::optional<std::string>
  get(int tag) const;

  void set(int tag, std::string value);
};

struct FixParseResult {
  bool valid{false};
  FixMessage message;
  std::string error;
};

[[nodiscard]] std::string to_string(
    FixMessageType type
);

[[nodiscard]] FixMessageType
fix_message_type_from_code(
    std::string_view code
);

[[nodiscard]] std::string fix_message_code(
    FixMessageType type
);

[[nodiscard]] int calculate_fix_checksum(
    std::string_view message_without_checksum
);

[[nodiscard]] std::string encode_fix_message(
    const FixMessage& message
);

[[nodiscard]] FixParseResult parse_fix_message(
    std::string_view wire_message
);

}  // namespace minimatch
