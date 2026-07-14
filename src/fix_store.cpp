#include "minimatch/fix_store.hpp"

#include <sqlite3.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace minimatch {

namespace {

void check_sqlite(
    int result,
    sqlite3* database,
    const char* operation
) {
  if (result == SQLITE_OK ||
      result == SQLITE_DONE ||
      result == SQLITE_ROW) {
    return;
  }

  throw std::runtime_error(
      std::string(operation) +
      ": " +
      sqlite3_errmsg(database)
  );
}

class Statement {
 public:
  Statement(
      sqlite3* database,
      const char* sql
  )
      : database_(database) {
    check_sqlite(
        sqlite3_prepare_v2(
            database_,
            sql,
            -1,
            &statement_,
            nullptr
        ),
        database_,
        "prepare SQLite statement"
    );
  }

  ~Statement() {
    if (statement_ != nullptr) {
      sqlite3_finalize(statement_);
    }
  }

  Statement(const Statement&) = delete;
  Statement& operator=(const Statement&) = delete;

  sqlite3_stmt* get() const {
    return statement_;
  }

 private:
  sqlite3* database_{nullptr};
  sqlite3_stmt* statement_{nullptr};
};

void bind_text(
    sqlite3* database,
    sqlite3_stmt* statement,
    int index,
    const std::string& value
) {
  check_sqlite(
      sqlite3_bind_text(
          statement,
          index,
          value.c_str(),
          -1,
          SQLITE_TRANSIENT
      ),
      database,
      "bind SQLite text"
  );
}

void bind_integer(
    sqlite3* database,
    sqlite3_stmt* statement,
    int index,
    std::int64_t value
) {
  check_sqlite(
      sqlite3_bind_int64(
          statement,
          index,
          value
      ),
      database,
      "bind SQLite integer"
  );
}

std::string column_text(
    sqlite3_stmt* statement,
    int column
) {
  const auto* value =
      sqlite3_column_text(
          statement,
          column
      );

  return value == nullptr
      ? std::string{}
      : reinterpret_cast<
            const char*
        >(value);
}

}  // namespace

FixStore::FixStore(
    const std::string& database_path
) {
  if (database_path.empty()) {
    throw std::invalid_argument(
        "FIX database path cannot be empty"
    );
  }

  const int result =
      sqlite3_open(
          database_path.c_str(),
          &database_
      );

  if (result != SQLITE_OK) {
    const std::string message =
        database_ == nullptr
            ? "could not open FIX database"
            : sqlite3_errmsg(database_);

    if (database_ != nullptr) {
      sqlite3_close(database_);
      database_ = nullptr;
    }

    throw std::runtime_error(message);
  }

  initialize_schema();
}

FixStore::~FixStore() {
  if (database_ != nullptr) {
    sqlite3_close(database_);
  }
}

FixStore::FixStore(
    FixStore&& other
) noexcept
    : database_(
          std::exchange(
              other.database_,
              nullptr
          )
      ) {}

FixStore& FixStore::operator=(
    FixStore&& other
) noexcept {
  if (this == &other) {
    return *this;
  }

  if (database_ != nullptr) {
    sqlite3_close(database_);
  }

  database_ = std::exchange(
      other.database_,
      nullptr
  );

  return *this;
}

void FixStore::initialize_schema() {
  const char* schema = R"SQL(
CREATE TABLE IF NOT EXISTS fix_sessions (
  session_id TEXT PRIMARY KEY,
  next_incoming_sequence INTEGER NOT NULL,
  next_outgoing_sequence INTEGER NOT NULL,
  last_received_time_ns INTEGER NOT NULL,
  last_sent_time_ns INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS fix_messages (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  session_id TEXT NOT NULL,
  direction TEXT NOT NULL,
  sequence_number INTEGER NOT NULL,
  message_type TEXT NOT NULL,
  wire_message TEXT NOT NULL,
  timestamp_ns INTEGER NOT NULL
);

CREATE INDEX IF NOT EXISTS
idx_fix_messages_session_sequence
ON fix_messages(
  session_id,
  direction,
  sequence_number
);
)SQL";

  char* error = nullptr;

  const int result =
      sqlite3_exec(
          database_,
          schema,
          nullptr,
          nullptr,
          &error
      );

  if (result != SQLITE_OK) {
    const std::string message =
        error == nullptr
            ? "initialize FIX schema"
            : error;

    sqlite3_free(error);

    throw std::runtime_error(message);
  }
}

void FixStore::save_session(
    const FixSessionSnapshot& snapshot
) {
  if (snapshot.session_id.empty()) {
    throw std::invalid_argument(
        "FIX session id cannot be empty"
    );
  }

  if (snapshot.next_incoming_sequence <= 0 ||
      snapshot.next_outgoing_sequence <= 0) {
    throw std::invalid_argument(
        "FIX sequence numbers must be positive"
    );
  }

  Statement statement(
      database_,
      R"SQL(
INSERT INTO fix_sessions (
  session_id,
  next_incoming_sequence,
  next_outgoing_sequence,
  last_received_time_ns,
  last_sent_time_ns
)
VALUES (?, ?, ?, ?, ?)
ON CONFLICT(session_id)
DO UPDATE SET
  next_incoming_sequence =
      excluded.next_incoming_sequence,
  next_outgoing_sequence =
      excluded.next_outgoing_sequence,
  last_received_time_ns =
      excluded.last_received_time_ns,
  last_sent_time_ns =
      excluded.last_sent_time_ns;
)SQL"
  );

  bind_text(
      database_,
      statement.get(),
      1,
      snapshot.session_id
  );

  bind_integer(
      database_,
      statement.get(),
      2,
      snapshot.next_incoming_sequence
  );

  bind_integer(
      database_,
      statement.get(),
      3,
      snapshot.next_outgoing_sequence
  );

  bind_integer(
      database_,
      statement.get(),
      4,
      static_cast<std::int64_t>(
          snapshot.last_received_time_ns
      )
  );

  bind_integer(
      database_,
      statement.get(),
      5,
      static_cast<std::int64_t>(
          snapshot.last_sent_time_ns
      )
  );

  check_sqlite(
      sqlite3_step(statement.get()),
      database_,
      "save FIX session"
  );
}

FixSessionSnapshot FixStore::load_session(
    const std::string& session_id
) const {
  if (session_id.empty()) {
    throw std::invalid_argument(
        "FIX session id cannot be empty"
    );
  }

  Statement statement(
      database_,
      R"SQL(
SELECT
  next_incoming_sequence,
  next_outgoing_sequence,
  last_received_time_ns,
  last_sent_time_ns
FROM fix_sessions
WHERE session_id = ?;
)SQL"
  );

  bind_text(
      database_,
      statement.get(),
      1,
      session_id
  );

  const int result =
      sqlite3_step(statement.get());

  if (result == SQLITE_DONE) {
    return FixSessionSnapshot{
        .session_id = session_id
    };
  }

  check_sqlite(
      result,
      database_,
      "load FIX session"
  );

  return FixSessionSnapshot{
      .session_id = session_id,
      .next_incoming_sequence =
          sqlite3_column_int(
              statement.get(),
              0
          ),
      .next_outgoing_sequence =
          sqlite3_column_int(
              statement.get(),
              1
          ),
      .last_received_time_ns =
          static_cast<std::uint64_t>(
              sqlite3_column_int64(
                  statement.get(),
                  2
              )
          ),
      .last_sent_time_ns =
          static_cast<std::uint64_t>(
              sqlite3_column_int64(
                  statement.get(),
                  3
              )
          )
  };
}

void FixStore::append_message(
    const StoredFixMessage& message
) {
  if (message.session_id.empty()) {
    throw std::invalid_argument(
        "FIX session id cannot be empty"
    );
  }

  if (message.direction != "IN" &&
      message.direction != "OUT") {
    throw std::invalid_argument(
        "FIX direction must be IN or OUT"
    );
  }

  if (message.sequence_number <= 0) {
    throw std::invalid_argument(
        "FIX message sequence must be positive"
    );
  }

  Statement statement(
      database_,
      R"SQL(
INSERT INTO fix_messages (
  session_id,
  direction,
  sequence_number,
  message_type,
  wire_message,
  timestamp_ns
)
VALUES (?, ?, ?, ?, ?, ?);
)SQL"
  );

  bind_text(
      database_,
      statement.get(),
      1,
      message.session_id
  );

  bind_text(
      database_,
      statement.get(),
      2,
      message.direction
  );

  bind_integer(
      database_,
      statement.get(),
      3,
      message.sequence_number
  );

  bind_text(
      database_,
      statement.get(),
      4,
      message.message_type
  );

  bind_text(
      database_,
      statement.get(),
      5,
      message.wire_message
  );

  bind_integer(
      database_,
      statement.get(),
      6,
      static_cast<std::int64_t>(
          message.timestamp_ns
      )
  );

  check_sqlite(
      sqlite3_step(statement.get()),
      database_,
      "append FIX message"
  );
}

std::vector<StoredFixMessage>
FixStore::messages_for_session(
    const std::string& session_id
) const {
  Statement statement(
      database_,
      R"SQL(
SELECT
  id,
  session_id,
  direction,
  sequence_number,
  message_type,
  wire_message,
  timestamp_ns
FROM fix_messages
WHERE session_id = ?
ORDER BY id ASC;
)SQL"
  );

  bind_text(
      database_,
      statement.get(),
      1,
      session_id
  );

  std::vector<StoredFixMessage> messages;

  for (;;) {
    const int result =
        sqlite3_step(statement.get());

    if (result == SQLITE_DONE) {
      break;
    }

    check_sqlite(
        result,
        database_,
        "read FIX messages"
    );

    messages.push_back(
        StoredFixMessage{
            .id =
                sqlite3_column_int64(
                    statement.get(),
                    0
                ),
            .session_id =
                column_text(
                    statement.get(),
                    1
                ),
            .direction =
                column_text(
                    statement.get(),
                    2
                ),
            .sequence_number =
                sqlite3_column_int(
                    statement.get(),
                    3
                ),
            .message_type =
                column_text(
                    statement.get(),
                    4
                ),
            .wire_message =
                column_text(
                    statement.get(),
                    5
                ),
            .timestamp_ns =
                static_cast<std::uint64_t>(
                    sqlite3_column_int64(
                        statement.get(),
                        6
                    )
                )
        }
    );
  }

  return messages;
}

std::vector<StoredFixMessage>
FixStore::outbound_messages_from_sequence(
    const std::string& session_id,
    int begin_sequence
) const {
  if (begin_sequence <= 0) {
    throw std::invalid_argument(
        "begin sequence must be positive"
    );
  }

  Statement statement(
      database_,
      R"SQL(
SELECT
  id,
  session_id,
  direction,
  sequence_number,
  message_type,
  wire_message,
  timestamp_ns
FROM fix_messages
WHERE session_id = ?
  AND direction = 'OUT'
  AND sequence_number >= ?
ORDER BY sequence_number ASC;
)SQL"
  );

  bind_text(
      database_,
      statement.get(),
      1,
      session_id
  );

  bind_integer(
      database_,
      statement.get(),
      2,
      begin_sequence
  );

  std::vector<StoredFixMessage> messages;

  for (;;) {
    const int result =
        sqlite3_step(statement.get());

    if (result == SQLITE_DONE) {
      break;
    }

    check_sqlite(
        result,
        database_,
        "read outbound FIX messages"
    );

    messages.push_back(
        StoredFixMessage{
            .id =
                sqlite3_column_int64(
                    statement.get(),
                    0
                ),
            .session_id =
                column_text(
                    statement.get(),
                    1
                ),
            .direction =
                column_text(
                    statement.get(),
                    2
                ),
            .sequence_number =
                sqlite3_column_int(
                    statement.get(),
                    3
                ),
            .message_type =
                column_text(
                    statement.get(),
                    4
                ),
            .wire_message =
                column_text(
                    statement.get(),
                    5
                ),
            .timestamp_ns =
                static_cast<std::uint64_t>(
                    sqlite3_column_int64(
                        statement.get(),
                        6
                    )
                )
        }
    );
  }

  return messages;
}

}  // namespace minimatch
