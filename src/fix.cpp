#include "minimatch/fix.hpp"

#include <algorithm>
#include <charconv>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace minimatch {

namespace {

std::vector<std::string_view> split_fields(
    std::string_view input
) {
  std::vector<std::string_view> fields;

  std::size_t start = 0;

  while (start < input.size()) {
    const std::size_t end =
        input.find(FIX_SOH, start);

    if (end == std::string_view::npos) {
      if (start < input.size()) {
        fields.push_back(input.substr(start));
      }

      break;
    }

    if (end > start) {
      fields.push_back(
          input.substr(start, end - start)
      );
    }

    start = end + 1;
  }

  return fields;
}

bool parse_integer(
    std::string_view value,
    int& output
) {
  const char* begin = value.data();
  const char* end = value.data() + value.size();

  const auto result =
      std::from_chars(begin, end, output);

  return result.ec == std::errc{} &&
         result.ptr == end;
}

std::optional<FixField> parse_field(
    std::string_view field
) {
  const std::size_t separator =
      field.find('=');

  if (separator == std::string_view::npos ||
      separator == 0) {
    return std::nullopt;
  }

  int tag = 0;

  if (!parse_integer(
          field.substr(0, separator),
          tag
      )) {
    return std::nullopt;
  }

  return FixField{
      .tag = tag,
      .value = std::string(
          field.substr(separator + 1)
      )
  };
}

std::string format_checksum(int checksum) {
  std::ostringstream output;

  output << std::setw(3)
         << std::setfill('0')
         << checksum;

  return output.str();
}

}  // namespace

std::optional<std::string>
FixMessage::get(int tag) const {
  const auto iterator = std::find_if(
      fields.begin(),
      fields.end(),
      [tag](const FixField& field) {
        return field.tag == tag;
      }
  );

  if (iterator == fields.end()) {
    return std::nullopt;
  }

  return iterator->value;
}

void FixMessage::set(
    int tag,
    std::string value
) {
  const auto iterator = std::find_if(
      fields.begin(),
      fields.end(),
      [tag](const FixField& field) {
        return field.tag == tag;
      }
  );

  if (iterator != fields.end()) {
    iterator->value = std::move(value);
    return;
  }

  fields.push_back(
      FixField{
          .tag = tag,
          .value = std::move(value)
      }
  );
}

std::string to_string(
    FixMessageType type
) {
  switch (type) {
    case FixMessageType::Logon:
      return "LOGON";

    case FixMessageType::Logout:
      return "LOGOUT";

    case FixMessageType::Heartbeat:
      return "HEARTBEAT";

    case FixMessageType::TestRequest:
      return "TEST_REQUEST";

    case FixMessageType::NewOrderSingle:
      return "NEW_ORDER_SINGLE";

    case FixMessageType::OrderCancelRequest:
      return "ORDER_CANCEL_REQUEST";

    case FixMessageType::ExecutionReport:
      return "EXECUTION_REPORT";

    case FixMessageType::Reject:
      return "REJECT";

    case FixMessageType::Unknown:
      return "UNKNOWN";
  }

  return "UNKNOWN";
}

FixMessageType fix_message_type_from_code(
    std::string_view code
) {
  if (code == "A") {
    return FixMessageType::Logon;
  }

  if (code == "5") {
    return FixMessageType::Logout;
  }

  if (code == "0") {
    return FixMessageType::Heartbeat;
  }

  if (code == "1") {
    return FixMessageType::TestRequest;
  }

  if (code == "D") {
    return FixMessageType::NewOrderSingle;
  }

  if (code == "F") {
    return FixMessageType::OrderCancelRequest;
  }

  if (code == "8") {
    return FixMessageType::ExecutionReport;
  }

  if (code == "3") {
    return FixMessageType::Reject;
  }

  return FixMessageType::Unknown;
}

std::string fix_message_code(
    FixMessageType type
) {
  switch (type) {
    case FixMessageType::Logon:
      return "A";

    case FixMessageType::Logout:
      return "5";

    case FixMessageType::Heartbeat:
      return "0";

    case FixMessageType::TestRequest:
      return "1";

    case FixMessageType::NewOrderSingle:
      return "D";

    case FixMessageType::OrderCancelRequest:
      return "F";

    case FixMessageType::ExecutionReport:
      return "8";

    case FixMessageType::Reject:
      return "3";

    case FixMessageType::Unknown:
      return "";
  }

  return "";
}

int calculate_fix_checksum(
    std::string_view message_without_checksum
) {
  unsigned int sum = 0;

  for (char character :
       message_without_checksum) {
    sum += static_cast<unsigned char>(
        character
    );
  }

  return static_cast<int>(sum % 256U);
}

std::string encode_fix_message(
    const FixMessage& message
) {
  const std::string message_code =
      fix_message_code(message.message_type);

  if (message_code.empty()) {
    throw std::invalid_argument(
        "cannot encode unknown FIX message type"
    );
  }

  std::ostringstream body;

  body << "35=" << message_code << FIX_SOH;

  for (const auto& field : message.fields) {
    if (field.tag == 8 ||
        field.tag == 9 ||
        field.tag == 10 ||
        field.tag == 35) {
      continue;
    }

    body << field.tag
         << "="
         << field.value
         << FIX_SOH;
  }

  const std::string body_string = body.str();

  std::ostringstream message_without_checksum;

  message_without_checksum
      << "8="
      << message.begin_string
      << FIX_SOH
      << "9="
      << body_string.size()
      << FIX_SOH
      << body_string;

  const std::string prefix =
      message_without_checksum.str();

  const int checksum =
      calculate_fix_checksum(prefix);

  std::ostringstream wire;

  wire << prefix
       << "10="
       << format_checksum(checksum)
       << FIX_SOH;

  return wire.str();
}

FixParseResult parse_fix_message(
    std::string_view wire_message
) {
  FixParseResult result;

  const auto fields =
      split_fields(wire_message);

  if (fields.size() < 4) {
    result.error =
        "FIX message has too few fields";

    return result;
  }

  const auto begin_field =
      parse_field(fields.front());

  if (!begin_field.has_value() ||
      begin_field->tag != 8) {
    result.error =
        "FIX message must begin with tag 8";

    return result;
  }

  const auto body_length_field =
      parse_field(fields[1]);

  if (!body_length_field.has_value() ||
      body_length_field->tag != 9) {
    result.error =
        "FIX message must contain tag 9 second";

    return result;
  }

  int declared_body_length = 0;

  if (!parse_integer(
          body_length_field->value,
          declared_body_length
      ) ||
      declared_body_length < 0) {
    result.error =
        "invalid FIX body length";

    return result;
  }

  const auto checksum_field =
      parse_field(fields.back());

  if (!checksum_field.has_value() ||
      checksum_field->tag != 10) {
    result.error =
        "FIX message must end with tag 10";

    return result;
  }

  int declared_checksum = 0;

  if (!parse_integer(
          checksum_field->value,
          declared_checksum
      ) ||
      declared_checksum < 0 ||
      declared_checksum > 255) {
    result.error =
        "invalid FIX checksum";

    return result;
  }

  const std::size_t checksum_position =
      wire_message.rfind("10=");

  if (checksum_position ==
      std::string_view::npos) {
    result.error =
        "FIX checksum field not found";

    return result;
  }

  const std::string_view prefix =
      wire_message.substr(
          0,
          checksum_position
      );

  const int calculated_checksum =
      calculate_fix_checksum(prefix);

  if (calculated_checksum !=
      declared_checksum) {
    result.error =
        "FIX checksum mismatch";

    return result;
  }

  const std::size_t body_start =
      wire_message.find(
          FIX_SOH,
          wire_message.find(FIX_SOH) + 1
      );

  if (body_start ==
      std::string_view::npos) {
    result.error =
        "FIX body start not found";

    return result;
  }

  const std::size_t actual_body_start =
      body_start + 1;

  if (checksum_position <
      actual_body_start) {
    result.error =
        "invalid FIX body boundaries";

    return result;
  }

  const std::size_t actual_body_length =
      checksum_position -
      actual_body_start;

  if (actual_body_length !=
      static_cast<std::size_t>(
          declared_body_length
      )) {
    result.error =
        "FIX body length mismatch";

    return result;
  }

  FixMessage message;

  message.begin_string =
      begin_field->value;

  message.body_length =
      declared_body_length;

  message.checksum =
      declared_checksum;

  for (std::size_t index = 2;
       index + 1 < fields.size();
       ++index) {
    const auto parsed =
        parse_field(fields[index]);

    if (!parsed.has_value()) {
      result.error =
          "invalid FIX field";

      return result;
    }

    if (parsed->tag == 35) {
      message.message_type =
          fix_message_type_from_code(
              parsed->value
          );
    }

    message.fields.push_back(*parsed);
  }

  if (message.message_type ==
      FixMessageType::Unknown) {
    result.error =
        "unsupported or missing FIX message type";

    return result;
  }

  result.valid = true;
  result.message = std::move(message);

  return result;
}

}  // namespace minimatch
