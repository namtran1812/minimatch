#include "minimatch/fix_session.hpp"

#include <charconv>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace minimatch {

namespace {

constexpr std::uint64_t nanoseconds_per_second =
    1'000'000'000ULL;

std::optional<int> parse_positive_integer(
    const std::optional<std::string>& value
) {
  if (!value.has_value()) {
    return std::nullopt;
  }

  int parsed = 0;

  const char* begin = value->data();
  const char* end = value->data() + value->size();

  const auto result =
      std::from_chars(begin, end, parsed);

  if (result.ec != std::errc{} ||
      result.ptr != end ||
      parsed <= 0) {
    return std::nullopt;
  }

  return parsed;
}

}  // namespace

std::string to_string(
    FixSessionState state
) {
  switch (state) {
    case FixSessionState::Disconnected:
      return "DISCONNECTED";

    case FixSessionState::AwaitingLogon:
      return "AWAITING_LOGON";

    case FixSessionState::Active:
      return "ACTIVE";

    case FixSessionState::LogoutPending:
      return "LOGOUT_PENDING";

    case FixSessionState::Terminated:
      return "TERMINATED";
  }

  return "UNKNOWN";
}

std::string to_string(
    FixSessionRejectReason reason
) {
  switch (reason) {
    case FixSessionRejectReason::None:
      return "NONE";

    case FixSessionRejectReason::InvalidState:
      return "INVALID_STATE";

    case FixSessionRejectReason::InvalidSequenceNumber:
      return "INVALID_SEQUENCE_NUMBER";

    case FixSessionRejectReason::MissingRequiredField:
      return "MISSING_REQUIRED_FIELD";

    case FixSessionRejectReason::SenderMismatch:
      return "SENDER_MISMATCH";

    case FixSessionRejectReason::TargetMismatch:
      return "TARGET_MISMATCH";

    case FixSessionRejectReason::UnsupportedMessage:
      return "UNSUPPORTED_MESSAGE";

    case FixSessionRejectReason::InvalidHeartbeatInterval:
      return "INVALID_HEARTBEAT_INTERVAL";
  }

  return "UNKNOWN";
}

FixSession::FixSession(
    FixSessionConfig config
)
    : config_(std::move(config)) {
  if (config_.sender_comp_id.empty() ||
      config_.target_comp_id.empty()) {
    throw std::invalid_argument(
        "FIX session CompIDs cannot be empty"
    );
  }

  if (config_.heartbeat_interval_seconds <= 0) {
    throw std::invalid_argument(
        "FIX heartbeat interval must be positive"
    );
  }
}

FixSessionState FixSession::state() const {
  return state_;
}

int FixSession::expected_incoming_sequence() const {
  return expected_incoming_sequence_;
}

int FixSession::next_outgoing_sequence() const {
  return next_outgoing_sequence_;
}

std::uint64_t FixSession::last_received_time_ns() const {
  return last_received_time_ns_;
}

std::uint64_t FixSession::last_sent_time_ns() const {
  return last_sent_time_ns_;
}

FixSessionResult FixSession::receive(
    const FixMessage& message,
    std::uint64_t timestamp_ns
) {
  if (!validate_comp_ids(message)) {
    const auto sender = message.get(49);
    const auto target = message.get(56);

    if (!sender.has_value() ||
        *sender != config_.target_comp_id) {
      return reject(
          FixSessionRejectReason::SenderMismatch,
          "unexpected SenderCompID",
          timestamp_ns
      );
    }

    if (!target.has_value() ||
        *target != config_.sender_comp_id) {
      return reject(
          FixSessionRejectReason::TargetMismatch,
          "unexpected TargetCompID",
          timestamp_ns
      );
    }
  }

  const auto sequence =
      sequence_number(message);

  if (!sequence.has_value()) {
    return reject(
        FixSessionRejectReason::MissingRequiredField,
        "missing or invalid MsgSeqNum",
        timestamp_ns
    );
  }

  if (*sequence != expected_incoming_sequence_) {
    return reject(
        FixSessionRejectReason::InvalidSequenceNumber,
        "unexpected incoming sequence number",
        timestamp_ns
    );
  }

  ++expected_incoming_sequence_;
  last_received_time_ns_ = timestamp_ns;

  switch (message.message_type) {
    case FixMessageType::Logon:
      return process_logon(
          message,
          timestamp_ns
      );

    case FixMessageType::Logout:
      return process_logout(
          message,
          timestamp_ns
      );

    case FixMessageType::Heartbeat:
      return process_heartbeat(
          message,
          timestamp_ns
      );

    case FixMessageType::TestRequest:
      return process_test_request(
          message,
          timestamp_ns
      );

    case FixMessageType::NewOrderSingle:
    case FixMessageType::OrderCancelRequest:
      if (state_ != FixSessionState::Active) {
        return reject(
            FixSessionRejectReason::InvalidState,
            "application message received before logon",
            timestamp_ns
        );
      }

      return FixSessionResult{
          .accepted = true,
          .reject_reason =
              FixSessionRejectReason::None,
          .message = "accepted",
          .response = std::nullopt
      };

    default:
      return reject(
          FixSessionRejectReason::UnsupportedMessage,
          "unsupported FIX message type",
          timestamp_ns
      );
  }
}

FixMessage FixSession::create_logon(
    std::uint64_t timestamp_ns
) {
  if (state_ != FixSessionState::Disconnected &&
      state_ != FixSessionState::AwaitingLogon) {
    throw std::logic_error(
        "cannot send logon in current FIX state"
    );
  }

  if (config_.reset_sequence_numbers_on_logon) {
    expected_incoming_sequence_ = 1;
    next_outgoing_sequence_ = 1;
  }

  state_ = FixSessionState::AwaitingLogon;

  FixMessage message =
      create_admin_message(
          FixMessageType::Logon,
          timestamp_ns
      );

  message.set(98, "0");

  message.set(
      108,
      std::to_string(
          config_.heartbeat_interval_seconds
      )
  );

  if (config_.reset_sequence_numbers_on_logon) {
    message.set(141, "Y");
  }

  return message;
}

FixMessage FixSession::create_logout(
    std::uint64_t timestamp_ns,
    const std::string& text
) {
  if (state_ != FixSessionState::Active) {
    throw std::logic_error(
        "cannot send logout when session is not active"
    );
  }

  state_ = FixSessionState::LogoutPending;

  FixMessage message =
      create_admin_message(
          FixMessageType::Logout,
          timestamp_ns
      );

  if (!text.empty()) {
    message.set(58, text);
  }

  return message;
}

FixMessage FixSession::create_heartbeat(
    std::uint64_t timestamp_ns,
    const std::string& test_request_id
) {
  FixMessage message =
      create_admin_message(
          FixMessageType::Heartbeat,
          timestamp_ns
      );

  if (!test_request_id.empty()) {
    message.set(112, test_request_id);
  }

  return message;
}

FixMessage FixSession::create_test_request(
    std::uint64_t timestamp_ns,
    const std::string& test_request_id
) {
  if (test_request_id.empty()) {
    throw std::invalid_argument(
        "TestReqID cannot be empty"
    );
  }

  FixMessage message =
      create_admin_message(
          FixMessageType::TestRequest,
          timestamp_ns
      );

  message.set(112, test_request_id);

  return message;
}

bool FixSession::heartbeat_due(
    std::uint64_t timestamp_ns
) const {
  if (state_ != FixSessionState::Active ||
      last_sent_time_ns_ == 0) {
    return false;
  }

  const std::uint64_t interval =
      static_cast<std::uint64_t>(
          config_.heartbeat_interval_seconds
      ) *
      nanoseconds_per_second;

  return timestamp_ns >=
      last_sent_time_ns_ + interval;
}

bool FixSession::test_request_due(
    std::uint64_t timestamp_ns
) const {
  if (state_ != FixSessionState::Active ||
      last_received_time_ns_ == 0) {
    return false;
  }

  const std::uint64_t interval =
      static_cast<std::uint64_t>(
          config_.heartbeat_interval_seconds
      ) *
      nanoseconds_per_second *
      2ULL;

  return timestamp_ns >=
      last_received_time_ns_ + interval;
}

void FixSession::reset() {
  state_ = FixSessionState::Disconnected;
  expected_incoming_sequence_ = 1;
  next_outgoing_sequence_ = 1;
  last_received_time_ns_ = 0;
  last_sent_time_ns_ = 0;
}

FixSessionResult FixSession::reject(
    FixSessionRejectReason reason,
    std::string message,
    std::uint64_t timestamp_ns
) {
  FixMessage response =
      create_admin_message(
          FixMessageType::Reject,
          timestamp_ns
      );

  response.set(58, message);

  return FixSessionResult{
      .accepted = false,
      .reject_reason = reason,
      .message = std::move(message),
      .response = std::move(response)
  };
}

FixMessage FixSession::create_admin_message(
    FixMessageType type,
    std::uint64_t timestamp_ns
) {
  FixMessage message;
  message.message_type = type;

  message.set(
      34,
      std::to_string(
          next_outgoing_sequence_++
      )
  );

  message.set(
      49,
      config_.sender_comp_id
  );

  message.set(
      56,
      config_.target_comp_id
  );

  message.set(
      52,
      std::to_string(timestamp_ns)
  );

  last_sent_time_ns_ = timestamp_ns;

  return message;
}

FixSessionResult FixSession::process_logon(
    const FixMessage& message,
    std::uint64_t timestamp_ns
) {
  if (state_ != FixSessionState::Disconnected &&
      state_ != FixSessionState::AwaitingLogon) {
    return reject(
        FixSessionRejectReason::InvalidState,
        "duplicate or unexpected logon",
        timestamp_ns
    );
  }

  const auto heartbeat =
      parse_positive_integer(
          message.get(108)
      );

  if (!heartbeat.has_value()) {
    return reject(
        FixSessionRejectReason::InvalidHeartbeatInterval,
        "missing or invalid HeartBtInt",
        timestamp_ns
    );
  }

  config_.heartbeat_interval_seconds =
      *heartbeat;

  state_ = FixSessionState::Active;

  FixMessage response =
      create_admin_message(
          FixMessageType::Logon,
          timestamp_ns
      );

  response.set(98, "0");

  response.set(
      108,
      std::to_string(
          config_.heartbeat_interval_seconds
      )
  );

  return FixSessionResult{
      .accepted = true,
      .reject_reason =
          FixSessionRejectReason::None,
      .message = "logon accepted",
      .response = std::move(response)
  };
}

FixSessionResult FixSession::process_logout(
    const FixMessage&,
    std::uint64_t timestamp_ns
) {
  if (state_ == FixSessionState::Terminated) {
    return reject(
        FixSessionRejectReason::InvalidState,
        "session already terminated",
        timestamp_ns
    );
  }

  FixSessionResult result{
      .accepted = true,
      .reject_reason =
          FixSessionRejectReason::None,
      .message = "logout accepted",
      .response = std::nullopt
  };

  if (state_ != FixSessionState::LogoutPending) {
    result.response =
        create_admin_message(
            FixMessageType::Logout,
            timestamp_ns
        );
  }

  state_ = FixSessionState::Terminated;

  return result;
}

FixSessionResult FixSession::process_heartbeat(
    const FixMessage&,
    std::uint64_t
) {
  if (state_ != FixSessionState::Active) {
    return FixSessionResult{
        .accepted = false,
        .reject_reason =
            FixSessionRejectReason::InvalidState,
        .message =
            "heartbeat received while session inactive",
        .response = std::nullopt
    };
  }

  return FixSessionResult{
      .accepted = true,
      .reject_reason =
          FixSessionRejectReason::None,
      .message = "heartbeat accepted",
      .response = std::nullopt
  };
}

FixSessionResult FixSession::process_test_request(
    const FixMessage& message,
    std::uint64_t timestamp_ns
) {
  if (state_ != FixSessionState::Active) {
    return reject(
        FixSessionRejectReason::InvalidState,
        "test request received while session inactive",
        timestamp_ns
    );
  }

  const auto test_request_id =
      message.get(112);

  if (!test_request_id.has_value() ||
      test_request_id->empty()) {
    return reject(
        FixSessionRejectReason::MissingRequiredField,
        "missing TestReqID",
        timestamp_ns
    );
  }

  return FixSessionResult{
      .accepted = true,
      .reject_reason =
          FixSessionRejectReason::None,
      .message = "test request accepted",
      .response = create_heartbeat(
          timestamp_ns,
          *test_request_id
      )
  };
}

std::optional<int> FixSession::sequence_number(
    const FixMessage& message
) const {
  return parse_positive_integer(
      message.get(34)
  );
}

bool FixSession::validate_comp_ids(
    const FixMessage& message
) const {
  const auto sender = message.get(49);
  const auto target = message.get(56);

  return sender.has_value() &&
         target.has_value() &&
         *sender == config_.target_comp_id &&
         *target == config_.sender_comp_id;
}

}  // namespace minimatch
