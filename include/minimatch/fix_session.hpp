#pragma once

#include "minimatch/fix.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace minimatch {

enum class FixSessionState {
  Disconnected,
  AwaitingLogon,
  Active,
  LogoutPending,
  Terminated
};

enum class FixSessionRejectReason {
  None,
  InvalidState,
  InvalidSequenceNumber,
  MissingRequiredField,
  SenderMismatch,
  TargetMismatch,
  UnsupportedMessage,
  InvalidHeartbeatInterval
};

struct FixSessionConfig {
  std::string sender_comp_id{"MINIMATCH"};
  std::string target_comp_id{"CLIENT"};

  int heartbeat_interval_seconds{30};

  bool reset_sequence_numbers_on_logon{false};
};

struct FixSessionResult {
  bool accepted{false};

  FixSessionRejectReason reject_reason{
      FixSessionRejectReason::None
  };

  std::string message;

  std::optional<FixMessage> response;
};

class FixSession {
 public:
  explicit FixSession(
      FixSessionConfig config = {}
  );

  [[nodiscard]] FixSessionState state() const;

  [[nodiscard]] int expected_incoming_sequence() const;
  [[nodiscard]] int next_outgoing_sequence() const;

  [[nodiscard]] std::uint64_t last_received_time_ns() const;
  [[nodiscard]] std::uint64_t last_sent_time_ns() const;

  FixSessionResult receive(
      const FixMessage& message,
      std::uint64_t timestamp_ns
  );

  FixMessage create_logon(
      std::uint64_t timestamp_ns
  );

  FixMessage create_logout(
      std::uint64_t timestamp_ns,
      const std::string& text = ""
  );

  FixMessage create_heartbeat(
      std::uint64_t timestamp_ns,
      const std::string& test_request_id = ""
  );

  FixMessage create_test_request(
      std::uint64_t timestamp_ns,
      const std::string& test_request_id
  );

  [[nodiscard]] bool heartbeat_due(
      std::uint64_t timestamp_ns
  ) const;

  [[nodiscard]] bool test_request_due(
      std::uint64_t timestamp_ns
  ) const;

  void reset();

 private:
  FixSessionConfig config_;

  FixSessionState state_{
      FixSessionState::Disconnected
  };

  int expected_incoming_sequence_{1};
  int next_outgoing_sequence_{1};

  std::uint64_t last_received_time_ns_{0};
  std::uint64_t last_sent_time_ns_{0};

  FixSessionResult reject(
      FixSessionRejectReason reason,
      std::string message,
      std::uint64_t timestamp_ns
  );

  FixMessage create_admin_message(
      FixMessageType type,
      std::uint64_t timestamp_ns
  );

  FixSessionResult process_logon(
      const FixMessage& message,
      std::uint64_t timestamp_ns
  );

  FixSessionResult process_logout(
      const FixMessage& message,
      std::uint64_t timestamp_ns
  );

  FixSessionResult process_heartbeat(
      const FixMessage& message,
      std::uint64_t timestamp_ns
  );

  FixSessionResult process_test_request(
      const FixMessage& message,
      std::uint64_t timestamp_ns
  );

  [[nodiscard]] std::optional<int>
  sequence_number(
      const FixMessage& message
  ) const;

  [[nodiscard]] bool validate_comp_ids(
      const FixMessage& message
  ) const;
};

std::string to_string(
    FixSessionState state
);

std::string to_string(
    FixSessionRejectReason reason
);

}  // namespace minimatch
