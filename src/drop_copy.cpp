#include "minimatch/drop_copy.hpp"

#include <sqlite3.h>

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <utility>

namespace minimatch {

namespace {

void check_sqlite(
    int result,
    sqlite3* database,
    const char* operation
) {
  if (
      result == SQLITE_OK ||
      result == SQLITE_DONE ||
      result == SQLITE_ROW
  ) {
    return;
  }

  throw std::runtime_error(
      std::string(operation) +
      ": " +
      sqlite3_errmsg(database)
  );
}

}  // namespace

DropCopyStore::DropCopyStore(
    std::string database_path,
    std::size_t capacity
) : database_path_(
        std::move(database_path)
    ),
    capacity_(
        std::max<std::size_t>(
            1,
            capacity
        )
    ) {
  const std::filesystem::path path(
      database_path_
  );

  if (path.has_parent_path()) {
    std::filesystem::
        create_directories(
            path.parent_path()
        );
  }

  check_sqlite(
      sqlite3_open(
          database_path_.c_str(),
          &database_
      ),
      database_,
      "open drop-copy database"
  );

  sqlite3_busy_timeout(
      database_,
      30000
  );

  initialize_schema();
  load_recent();
}

DropCopyStore::~DropCopyStore() {
  if (database_ != nullptr) {
    sqlite3_close(
        database_
    );
  }
}

void DropCopyStore::initialize_schema() {
  const char* sql = R"SQL(
    CREATE TABLE IF NOT EXISTS
    drop_copy_events (
      id INTEGER PRIMARY KEY,
      timestamp_ns INTEGER NOT NULL,
      order_id INTEGER NOT NULL,
      symbol INTEGER NOT NULL,
      event_type TEXT NOT NULL,
      status TEXT NOT NULL,
      remaining INTEGER NOT NULL,
      reject_reason INTEGER NOT NULL
    );

    CREATE INDEX IF NOT EXISTS
    idx_drop_copy_events_order_id
    ON drop_copy_events(order_id);
  )SQL";

  char* error = nullptr;

  const int result =
      sqlite3_exec(
          database_,
          sql,
          nullptr,
          nullptr,
          &error
      );

  if (result != SQLITE_OK) {
    const std::string message =
        error != nullptr
            ? error
            : sqlite3_errmsg(
                  database_
              );

    sqlite3_free(error);

    throw std::runtime_error(
        message
    );
  }
}

void DropCopyStore::persist(
    const DropCopyEvent& event
) {
  const char* sql = R"SQL(
    INSERT INTO drop_copy_events (
      id,
      timestamp_ns,
      order_id,
      symbol,
      event_type,
      status,
      remaining,
      reject_reason
    )
    VALUES (?, ?, ?, ?, ?, ?, ?, ?);
  )SQL";

  sqlite3_stmt* stmt = nullptr;

  check_sqlite(
      sqlite3_prepare_v2(
          database_,
          sql,
          -1,
          &stmt,
          nullptr
      ),
      database_,
      "prepare drop-copy insert"
  );

  sqlite3_bind_int64(
      stmt,
      1,
      static_cast<sqlite3_int64>(
          event.id
      )
  );

  sqlite3_bind_int64(
      stmt,
      2,
      static_cast<sqlite3_int64>(
          event.timestamp_ns
      )
  );

  sqlite3_bind_int64(
      stmt,
      3,
      static_cast<sqlite3_int64>(
          event.order_id
      )
  );

  sqlite3_bind_int64(
      stmt,
      4,
      static_cast<sqlite3_int64>(
          event.symbol
      )
  );

  sqlite3_bind_text(
      stmt,
      5,
      event.event_type.c_str(),
      -1,
      SQLITE_TRANSIENT
  );

  sqlite3_bind_text(
      stmt,
      6,
      event.status.c_str(),
      -1,
      SQLITE_TRANSIENT
  );

  sqlite3_bind_int64(
      stmt,
      7,
      static_cast<sqlite3_int64>(
          event.remaining
      )
  );

  sqlite3_bind_int(
      stmt,
      8,
      static_cast<int>(
          event.reject_reason
      )
  );

  check_sqlite(
      sqlite3_step(stmt),
      database_,
      "insert drop-copy event"
  );

  sqlite3_finalize(stmt);
}

void DropCopyStore::load_recent() {
  const char* sql = R"SQL(
    SELECT
      id,
      timestamp_ns,
      order_id,
      symbol,
      event_type,
      status,
      remaining,
      reject_reason
    FROM drop_copy_events
    ORDER BY id DESC
    LIMIT ?;
  )SQL";

  sqlite3_stmt* stmt = nullptr;

  check_sqlite(
      sqlite3_prepare_v2(
          database_,
          sql,
          -1,
          &stmt,
          nullptr
      ),
      database_,
      "prepare drop-copy load"
  );

  sqlite3_bind_int64(
      stmt,
      1,
      static_cast<sqlite3_int64>(
          capacity_
      )
  );

  std::uint64_t max_id = 0;

  while (
      sqlite3_step(stmt) ==
      SQLITE_ROW
  ) {
    DropCopyEvent event;

    event.id =
        static_cast<std::uint64_t>(
            sqlite3_column_int64(
                stmt,
                0
            )
        );

    event.timestamp_ns =
        static_cast<std::uint64_t>(
            sqlite3_column_int64(
                stmt,
                1
            )
        );

    event.order_id =
        static_cast<OrderId>(
            sqlite3_column_int64(
                stmt,
                2
            )
        );

    event.symbol =
        static_cast<SymbolId>(
            sqlite3_column_int64(
                stmt,
                3
            )
        );

    event.event_type =
        reinterpret_cast<const char*>(
            sqlite3_column_text(
                stmt,
                4
            )
        );

    event.status =
        reinterpret_cast<const char*>(
            sqlite3_column_text(
                stmt,
                5
            )
        );

    event.remaining =
        static_cast<Quantity>(
            sqlite3_column_int64(
                stmt,
                6
            )
        );

    event.reject_reason =
        static_cast<RejectReason>(
            sqlite3_column_int(
                stmt,
                7
            )
        );

    max_id =
        std::max(
            max_id,
            event.id
        );

    events_.push_back(
        std::move(
            event
        )
    );
  }

  sqlite3_finalize(stmt);

  next_id_ =
      max_id + 1;
}

void DropCopyStore::publish(
    DropCopyEvent event
) {
  std::lock_guard lock(
      mutex_
  );

  event.id =
      next_id_++;

  persist(
      event
  );

  events_.push_front(
      std::move(
          event
      )
  );

  while (
      events_.size() >
      capacity_
  ) {
    events_.pop_back();
  }
}

std::vector<DropCopyEvent>
DropCopyStore::recent(
    std::size_t limit
) const {
  std::lock_guard lock(
      mutex_
  );

  const auto count =
      std::min(
          limit,
          events_.size()
      );

  std::vector<DropCopyEvent>
      result;

  result.reserve(
      count
  );

  for (
      std::size_t i = 0;
      i < count;
      ++i
  ) {
    result.push_back(
        events_[i]
    );
  }

  return result;
}


std::vector<DropCopyEvent>
DropCopyStore::events_for_order(
    OrderId order_id
) const {
  std::lock_guard lock(
      mutex_
  );

  const char* sql = R"SQL(
    SELECT
      id,
      timestamp_ns,
      order_id,
      symbol,
      event_type,
      status,
      remaining,
      reject_reason
    FROM drop_copy_events
    WHERE order_id = ?
    ORDER BY id ASC;
  )SQL";

  sqlite3_stmt* stmt = nullptr;

  check_sqlite(
      sqlite3_prepare_v2(
          database_,
          sql,
          -1,
          &stmt,
          nullptr
      ),
      database_,
      "prepare drop-copy order lookup"
  );

  sqlite3_bind_int64(
      stmt,
      1,
      static_cast<sqlite3_int64>(
          order_id
      )
  );

  std::vector<DropCopyEvent>
      result;

  while (
      sqlite3_step(stmt) ==
      SQLITE_ROW
  ) {
    DropCopyEvent event;

    event.id =
        static_cast<std::uint64_t>(
            sqlite3_column_int64(
                stmt,
                0
            )
        );

    event.timestamp_ns =
        static_cast<std::uint64_t>(
            sqlite3_column_int64(
                stmt,
                1
            )
        );

    event.order_id =
        static_cast<OrderId>(
            sqlite3_column_int64(
                stmt,
                2
            )
        );

    event.symbol =
        static_cast<SymbolId>(
            sqlite3_column_int64(
                stmt,
                3
            )
        );

    event.event_type =
        reinterpret_cast<const char*>(
            sqlite3_column_text(
                stmt,
                4
            )
        );

    event.status =
        reinterpret_cast<const char*>(
            sqlite3_column_text(
                stmt,
                5
            )
        );

    event.remaining =
        static_cast<Quantity>(
            sqlite3_column_int64(
                stmt,
                6
            )
        );

    event.reject_reason =
        static_cast<RejectReason>(
            sqlite3_column_int(
                stmt,
                7
            )
        );

    result.push_back(
        std::move(event)
    );
  }

  sqlite3_finalize(
      stmt
  );

  return result;
}

}  // namespace minimatch
