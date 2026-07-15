#include "minimatch/fix.hpp"

#include <gtest/gtest.h>

#include <string>

namespace {

using minimatch::FixMessage;
using minimatch::FixMessageType;
using minimatch::calculate_fix_checksum;
using minimatch::encode_fix_message;
using minimatch::parse_fix_message;

TEST(FixCodec, EncodesAndParsesLogon) {
  FixMessage message;

  message.message_type =
      FixMessageType::Logon;

  message.set(34, "1");
  message.set(49, "CLIENT");
  message.set(56, "MINIMATCH");
  message.set(98, "0");
  message.set(108, "30");

  const std::string wire =
      encode_fix_message(message);

  const auto parsed =
      parse_fix_message(wire);

  ASSERT_TRUE(parsed.valid)
      << parsed.error;

  EXPECT_EQ(
      parsed.message.message_type,
      FixMessageType::Logon
  );

  EXPECT_EQ(
      parsed.message.get(49),
      std::optional<std::string>("CLIENT")
  );

  EXPECT_EQ(
      parsed.message.get(56),
      std::optional<std::string>("MINIMATCH")
  );

  EXPECT_EQ(
      parsed.message.get(108),
      std::optional<std::string>("30")
  );
}

TEST(FixCodec, EncodesNewOrderSingle) {
  FixMessage message;

  message.message_type =
      FixMessageType::NewOrderSingle;

  message.set(11, "ORDER-1");
  message.set(55, "BTCUSD");
  message.set(54, "1");
  message.set(38, "0.50");
  message.set(40, "2");
  message.set(44, "62640.10");
  message.set(59, "0");

  const std::string wire =
      encode_fix_message(message);

  const auto parsed =
      parse_fix_message(wire);

  ASSERT_TRUE(parsed.valid)
      << parsed.error;

  EXPECT_EQ(
      parsed.message.message_type,
      FixMessageType::NewOrderSingle
  );

  EXPECT_EQ(
      parsed.message.get(11),
      std::optional<std::string>(
          "ORDER-1"
      )
  );

  EXPECT_EQ(
      parsed.message.get(55),
      std::optional<std::string>(
          "BTCUSD"
      )
  );

  EXPECT_EQ(
      parsed.message.get(44),
      std::optional<std::string>(
          "62640.10"
      )
  );
}

TEST(FixCodec, RejectsInvalidChecksum) {
  FixMessage message;

  message.message_type =
      FixMessageType::Heartbeat;

  message.set(34, "2");
  message.set(49, "CLIENT");
  message.set(56, "MINIMATCH");

  std::string wire =
      encode_fix_message(message);

  const std::size_t checksum =
      wire.rfind("10=");

  ASSERT_NE(
      checksum,
      std::string::npos
  );

  wire[checksum + 3] =
      wire[checksum + 3] == '0'
          ? '1'
          : '0';

  const auto parsed =
      parse_fix_message(wire);

  EXPECT_FALSE(parsed.valid);
  EXPECT_EQ(
      parsed.error,
      "FIX checksum mismatch"
  );
}

TEST(FixCodec, RejectsInvalidBodyLength) {
  FixMessage message;

  message.message_type =
      FixMessageType::Logout;

  message.set(34, "3");

  std::string wire =
      encode_fix_message(message);

  const std::size_t body_length =
      wire.find("9=");

  ASSERT_NE(
      body_length,
      std::string::npos
  );

  const std::size_t value_start =
      body_length + 2;

  wire[value_start] =
      wire[value_start] == '9'
          ? '8'
          : '9';

  const std::size_t checksum_position =
      wire.rfind("10=");

  const std::string prefix =
      wire.substr(
          0,
          checksum_position
      );

  const int checksum =
      calculate_fix_checksum(prefix);

  char checksum_buffer[4];

  std::snprintf(
      checksum_buffer,
      sizeof(checksum_buffer),
      "%03d",
      checksum
  );

  wire.replace(
      checksum_position + 3,
      3,
      checksum_buffer
  );

  const auto parsed =
      parse_fix_message(wire);

  EXPECT_FALSE(parsed.valid);
  EXPECT_EQ(
      parsed.error,
      "FIX body length mismatch"
  );
}

TEST(FixCodec, SupportsCancelAndExecutionReport) {
  FixMessage cancel;

  cancel.message_type =
      FixMessageType::OrderCancelRequest;

  cancel.set(11, "CANCEL-1");
  cancel.set(41, "ORDER-1");
  cancel.set(55, "BTCUSD");
  cancel.set(54, "1");

  EXPECT_TRUE(
      parse_fix_message(
          encode_fix_message(cancel)
      ).valid
  );

  FixMessage report;

  report.message_type =
      FixMessageType::ExecutionReport;

  report.set(11, "ORDER-1");
  report.set(17, "EXEC-1");
  report.set(39, "2");
  report.set(150, "F");
  report.set(14, "0.50");
  report.set(151, "0");
  report.set(31, "62640.10");
  report.set(32, "0.50");

  const auto parsed_report =
      parse_fix_message(
          encode_fix_message(report)
      );

  ASSERT_TRUE(parsed_report.valid)
      << parsed_report.error;

  EXPECT_EQ(
      parsed_report.message.message_type,
      FixMessageType::ExecutionReport
  );
}


TEST(FixCodec, PreparesPossDupResend) {
  FixMessage original;

  original.message_type =
      FixMessageType::ExecutionReport;

  original.set(34, "7");
  original.set(49, "MINIMATCH");
  original.set(56, "CLIENT");
  original.set(52, "1000");
  original.set(11, "ORDER-1");
  original.set(17, "EXEC-1");
  original.set(150, "0");
  original.set(39, "0");

  const auto resent =
      minimatch::prepare_fix_resend(
          original,
          2000
      );

  EXPECT_EQ(
      resent.get(34),
      std::optional<std::string>("7")
  );

  EXPECT_EQ(
      resent.get(43),
      std::optional<std::string>("Y")
  );

  EXPECT_EQ(
      resent.get(122),
      std::optional<std::string>("1000")
  );

  EXPECT_EQ(
      resent.get(52),
      std::optional<std::string>("2000")
  );

  const auto parsed =
      parse_fix_message(
          encode_fix_message(resent)
      );

  EXPECT_TRUE(parsed.valid)
      << parsed.error;
}

TEST(FixCodec, CreatesSequenceResetGapFill) {
  const auto gap_fill =
      minimatch::create_fix_gap_fill(
          2,
          5,
          "MINIMATCH",
          "CLIENT",
          3000
      );

  EXPECT_EQ(
      gap_fill.message_type,
      FixMessageType::SequenceReset
  );

  EXPECT_EQ(
      gap_fill.get(34),
      std::optional<std::string>("2")
  );

  EXPECT_EQ(
      gap_fill.get(123),
      std::optional<std::string>("Y")
  );

  EXPECT_EQ(
      gap_fill.get(36),
      std::optional<std::string>("5")
  );

  EXPECT_EQ(
      gap_fill.get(43),
      std::optional<std::string>("Y")
  );

  const auto parsed =
      parse_fix_message(
          encode_fix_message(gap_fill)
      );

  EXPECT_TRUE(parsed.valid)
      << parsed.error;
}


}  // namespace
