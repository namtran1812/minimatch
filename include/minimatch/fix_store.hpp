#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct sqlite3;

namespace minimatch {

struct FixSessionSnapshot {
  std::string session_id;

  int next_incoming_sequence{1};
  int next_outgoing_sequence{1};

  std::uint64_t last_received_time_ns{0};
  std::uint64_t last_sent_time_ns{0};
};

struct StoredFixMessage {
  std::int64_t id{0};

  std::string session_id;
  std::string direction;
  int sequence_number{0};
  std::string message_type;
  std::string wire_message;

  std::uint64_t timestamp_ns{0};
};

class FixStore {
 public:
  explicit FixStore(
      const std::string& database_path
  );

  ~FixStore();

  FixStore(const FixStore&) = delete;
  FixStore& operator=(const FixStore&) = delete;

  FixStore(FixStore&& other) noexcept;
  FixStore& operator=(FixStore&& other) noexcept;

  void save_session(
      const FixSessionSnapshot& snapshot
  );

  [[nodiscard]] FixSessionSnapshot load_session(
      const std::string& session_id
  ) const;

  void append_message(
      const StoredFixMessage& message
  );

  [[nodiscard]] std::vector<StoredFixMessage>
  messages_for_session(
      const std::string& session_id
  ) const;

  [[nodiscard]] std::vector<StoredFixMessage>
  outbound_messages_from_sequence(
      const std::string& session_id,
      int begin_sequence
  ) const;

 private:
  sqlite3* database_{nullptr};

  void initialize_schema();
};

}  // namespace minimatch
