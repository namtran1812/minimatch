#include "minimatch/fix_session.hpp"

#include <gtest/gtest.h>

namespace {

using minimatch::FixMessage;
using minimatch::FixMessageType;
using minimatch::FixSession;
using minimatch::FixSessionConfig;
using minimatch::FixSessionRejectReason;
using minimatch::FixSessionState;

FixMessage inbound_message(
    FixMessageType type,
    int sequence
) {
  FixMessage message;
  message.message_type = type;

  message.set(34, std::to_string(sequence));
  message.set(49, "CLIENT");
  message.set(56, "MINIMATCH");
  message.set(52, "100");

  return message;
}

TEST(FixSession, AcceptsLogonAndBecomesActive) {
  FixSession session;

  FixMessage logon =
      inbound_message(
          FixMessageType::Logon,
          1
      );

  logon.set(98, "0");
  logon.set(108, "30");

  const auto result =
      session.receive(logon, 100);

  EXPECT_TRUE(result.accepted);
  EXPECT_EQ(
      session.state(),
      FixSessionState::Active
  );

  EXPECT_EQ(
      session.expected_incoming_sequence(),
      2
  );

  ASSERT_TRUE(result.response.has_value());

  EXPECT_EQ(
      result.response->message_type,
      FixMessageType::Logon
  );
}

TEST(FixSession, RejectsIncorrectSequenceNumber) {
  FixSession session;

  FixMessage logon =
      inbound_message(
          FixMessageType::Logon,
          2
      );

  logon.set(98, "0");
  logon.set(108, "30");

  const auto result =
      session.receive(logon, 100);

  EXPECT_FALSE(result.accepted);

  EXPECT_EQ(
      result.reject_reason,
      FixSessionRejectReason::InvalidSequenceNumber
  );
}

TEST(FixSession, RejectsApplicationMessageBeforeLogon) {
  FixSession session;

  FixMessage order =
      inbound_message(
          FixMessageType::NewOrderSingle,
          1
      );

  order.set(11, "ORDER-1");

  const auto result =
      session.receive(order, 100);

  EXPECT_FALSE(result.accepted);

  EXPECT_EQ(
      result.reject_reason,
      FixSessionRejectReason::InvalidState
  );
}

TEST(FixSession, AcceptsApplicationMessageAfterLogon) {
  FixSession session;

  FixMessage logon =
      inbound_message(
          FixMessageType::Logon,
          1
      );

  logon.set(98, "0");
  logon.set(108, "30");

  ASSERT_TRUE(
      session.receive(logon, 100).accepted
  );

  FixMessage order =
      inbound_message(
          FixMessageType::NewOrderSingle,
          2
      );

  order.set(11, "ORDER-1");

  const auto result =
      session.receive(order, 200);

  EXPECT_TRUE(result.accepted);
}

TEST(FixSession, RespondsToTestRequestWithHeartbeat) {
  FixSession session;

  FixMessage logon =
      inbound_message(
          FixMessageType::Logon,
          1
      );

  logon.set(98, "0");
  logon.set(108, "30");

  ASSERT_TRUE(
      session.receive(logon, 100).accepted
  );

  FixMessage test_request =
      inbound_message(
          FixMessageType::TestRequest,
          2
      );

  test_request.set(112, "TEST-1");

  const auto result =
      session.receive(
          test_request,
          200
      );

  ASSERT_TRUE(result.accepted);
  ASSERT_TRUE(result.response.has_value());

  EXPECT_EQ(
      result.response->message_type,
      FixMessageType::Heartbeat
  );

  EXPECT_EQ(
      result.response->get(112),
      std::optional<std::string>("TEST-1")
  );
}

TEST(FixSession, HandlesLogout) {
  FixSession session;

  FixMessage logon =
      inbound_message(
          FixMessageType::Logon,
          1
      );

  logon.set(98, "0");
  logon.set(108, "30");

  ASSERT_TRUE(
      session.receive(logon, 100).accepted
  );

  FixMessage logout =
      inbound_message(
          FixMessageType::Logout,
          2
      );

  const auto result =
      session.receive(logout, 200);

  EXPECT_TRUE(result.accepted);

  EXPECT_EQ(
      session.state(),
      FixSessionState::Terminated
  );

  EXPECT_TRUE(result.response.has_value());
}

TEST(FixSession, DetectsHeartbeatAndTestRequestDeadlines) {
  FixSession session(
      FixSessionConfig{
          .sender_comp_id = "MINIMATCH",
          .target_comp_id = "CLIENT",
          .heartbeat_interval_seconds = 30
      }
  );

  FixMessage logon =
      inbound_message(
          FixMessageType::Logon,
          1
      );

  logon.set(98, "0");
  logon.set(108, "30");

  ASSERT_TRUE(
      session.receive(
          logon,
          1'000'000'000ULL
      ).accepted
  );

  session.create_heartbeat(
      2'000'000'000ULL
  );

  EXPECT_FALSE(
      session.heartbeat_due(
          31'000'000'000ULL
      )
  );

  EXPECT_TRUE(
      session.heartbeat_due(
          32'000'000'000ULL
      )
  );

  EXPECT_FALSE(
      session.test_request_due(
          60'000'000'000ULL
      )
  );

  EXPECT_TRUE(
      session.test_request_due(
          61'000'000'000ULL
      )
  );
}

TEST(FixSession, RejectsCompIdMismatch) {
  FixSession session;

  FixMessage logon =
      inbound_message(
          FixMessageType::Logon,
          1
      );

  logon.set(49, "WRONG");
  logon.set(98, "0");
  logon.set(108, "30");

  const auto result =
      session.receive(logon, 100);

  EXPECT_FALSE(result.accepted);

  EXPECT_EQ(
      result.reject_reason,
      FixSessionRejectReason::SenderMismatch
  );
}

TEST(FixSession, OutgoingMessagesIncrementSequence) {
  FixSession session;

  const auto logon =
      session.create_logon(100);

  EXPECT_EQ(
      logon.get(34),
      std::optional<std::string>("1")
  );

  EXPECT_EQ(
      session.next_outgoing_sequence(),
      2
  );
}

TEST(FixSession, ResetRestoresInitialState) {
  FixSession session;

  session.create_logon(100);
  session.reset();

  EXPECT_EQ(
      session.state(),
      FixSessionState::Disconnected
  );

  EXPECT_EQ(
      session.expected_incoming_sequence(),
      1
  );

  EXPECT_EQ(
      session.next_outgoing_sequence(),
      1
  );
}


TEST(FixSession, SequencesOutboundExecutionReports) {
  FixSession session;

  FixMessage logon =
      inbound_message(
          FixMessageType::Logon,
          1
      );

  logon.set(98, "0");
  logon.set(108, "30");

  ASSERT_TRUE(
      session.receive(logon, 100).accepted
  );

  FixMessage report;
  report.message_type =
      FixMessageType::ExecutionReport;

  report.set(11, "ORDER-1");
  report.set(17, "EXEC-1");
  report.set(150, "0");
  report.set(39, "0");

  const auto outbound =
      session.prepare_outbound(
          report,
          200
      );

  EXPECT_EQ(
      outbound.get(34),
      std::optional<std::string>("2")
  );

  EXPECT_EQ(
      outbound.get(49),
      std::optional<std::string>("MINIMATCH")
  );

  EXPECT_EQ(
      outbound.get(56),
      std::optional<std::string>("CLIENT")
  );

  EXPECT_EQ(
      outbound.get(52),
      std::optional<std::string>("200")
  );
}


}  // namespace
