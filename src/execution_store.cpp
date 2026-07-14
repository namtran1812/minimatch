#include "minimatch/execution_store.hpp"

#include <sqlite3.h>

#include <filesystem>
#include <stdexcept>
#include <string>

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

  const std::string message =
      database != nullptr
          ? sqlite3_errmsg(database)
          : "unknown SQLite error";

  throw std::runtime_error(
      std::string(operation) + ": " + message
  );
}

class Statement {
 public:
  Statement(sqlite3* database, const char* sql)
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
        "prepare statement"
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

}  // namespace

ExecutionStore::ExecutionStore(
    const std::string& database_path
) {
  const std::filesystem::path path(database_path);

  if (path.has_parent_path()) {
    std::filesystem::create_directories(
        path.parent_path()
    );
  }

  check_sqlite(
      sqlite3_open(database_path.c_str(), &database_),
      database_,
      "open execution database"
  );

  sqlite3_busy_timeout(database_, 30000);

  execute("PRAGMA journal_mode=WAL;");
  execute("PRAGMA foreign_keys=ON;");
  execute("PRAGMA synchronous=NORMAL;");

  initialize_schema();
}

ExecutionStore::~ExecutionStore() {
  if (database_ != nullptr) {
    sqlite3_close(database_);
  }
}

void ExecutionStore::execute(const char* sql) const {
  char* error = nullptr;

  const int result = sqlite3_exec(
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
            : sqlite3_errmsg(database_);

    sqlite3_free(error);

    throw std::runtime_error(
        "execute SQLite statement: " + message
    );
  }
}

void ExecutionStore::initialize_schema() {
  execute(R"SQL(
    CREATE TABLE IF NOT EXISTS route_executions (
      execution_id INTEGER PRIMARY KEY AUTOINCREMENT,
      created_at TEXT NOT NULL
        DEFAULT CURRENT_TIMESTAMP,

      symbol TEXT NOT NULL,
      side TEXT NOT NULL,

      requested_quantity REAL NOT NULL,
      filled_quantity REAL NOT NULL,
      remaining_quantity REAL NOT NULL,

      average_fill_price REAL NOT NULL,
      total_notional REAL NOT NULL,
      total_fees REAL NOT NULL,
      total_latency_ms REAL NOT NULL,

      complete INTEGER NOT NULL,

      fill_ratio REAL NOT NULL,
      rejection_probability REAL NOT NULL,
      base_latency_ms REAL NOT NULL,
      latency_jitter_ms REAL NOT NULL,
      simulation_seed INTEGER NOT NULL
    );

    CREATE TABLE IF NOT EXISTS child_executions (
      child_id INTEGER PRIMARY KEY AUTOINCREMENT,
      execution_id INTEGER NOT NULL,

      venue TEXT NOT NULL,
      level_index INTEGER NOT NULL,

      requested_quantity REAL NOT NULL,
      filled_quantity REAL NOT NULL,
      remaining_quantity REAL NOT NULL,

      price REAL NOT NULL,
      notional REAL NOT NULL,
      fee REAL NOT NULL,
      latency_ms REAL NOT NULL,
      status TEXT NOT NULL,

      FOREIGN KEY(execution_id)
        REFERENCES route_executions(execution_id)
        ON DELETE CASCADE
    );

    CREATE INDEX IF NOT EXISTS
      idx_route_executions_created_at
    ON route_executions(created_at);

    CREATE INDEX IF NOT EXISTS
      idx_child_executions_execution_id
    ON child_executions(execution_id);
  )SQL");
}

std::int64_t ExecutionStore::save(
    const std::string& symbol,
    RouteSide side,
    const ExecutionSimulationConfig& config,
    const RoutedExecutionSummary& summary
) {
  execute("BEGIN IMMEDIATE TRANSACTION;");

  try {
    Statement route_statement(
        database_,
        R"SQL(
          INSERT INTO route_executions (
            symbol,
            side,
            requested_quantity,
            filled_quantity,
            remaining_quantity,
            average_fill_price,
            total_notional,
            total_fees,
            total_latency_ms,
            complete,
            fill_ratio,
            rejection_probability,
            base_latency_ms,
            latency_jitter_ms,
            simulation_seed
          )
          VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
        )SQL"
    );

    sqlite3_stmt* route = route_statement.get();

    sqlite3_bind_text(
        route,
        1,
        symbol.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    const char* side_text =
        side == RouteSide::Buy ? "BUY" : "SELL";

    sqlite3_bind_text(
        route,
        2,
        side_text,
        -1,
        SQLITE_STATIC
    );

    sqlite3_bind_double(
        route,
        3,
        summary.requested_quantity
    );
    sqlite3_bind_double(
        route,
        4,
        summary.filled_quantity
    );
    sqlite3_bind_double(
        route,
        5,
        summary.remaining_quantity
    );
    sqlite3_bind_double(
        route,
        6,
        summary.average_fill_price
    );
    sqlite3_bind_double(
        route,
        7,
        summary.total_notional
    );
    sqlite3_bind_double(
        route,
        8,
        summary.total_fees
    );
    sqlite3_bind_double(
        route,
        9,
        summary.total_latency_ms
    );
    sqlite3_bind_int(
        route,
        10,
        summary.complete ? 1 : 0
    );
    sqlite3_bind_double(
        route,
        11,
        config.fill_ratio
    );
    sqlite3_bind_double(
        route,
        12,
        config.rejection_probability
    );
    sqlite3_bind_double(
        route,
        13,
        config.base_latency_ms
    );
    sqlite3_bind_double(
        route,
        14,
        config.latency_jitter_ms
    );
    sqlite3_bind_int64(
        route,
        15,
        static_cast<sqlite3_int64>(config.seed)
    );

    check_sqlite(
        sqlite3_step(route),
        database_,
        "insert route execution"
    );

    const std::int64_t execution_id =
        sqlite3_last_insert_rowid(database_);

    Statement child_statement(
        database_,
        R"SQL(
          INSERT INTO child_executions (
            execution_id,
            venue,
            level_index,
            requested_quantity,
            filled_quantity,
            remaining_quantity,
            price,
            notional,
            fee,
            latency_ms,
            status
          )
          VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
        )SQL"
    );

    sqlite3_stmt* child = child_statement.get();

    for (const auto& execution : summary.children) {
      sqlite3_reset(child);
      sqlite3_clear_bindings(child);

      sqlite3_bind_int64(
          child,
          1,
          execution_id
      );
      sqlite3_bind_text(
          child,
          2,
          execution.venue.c_str(),
          -1,
          SQLITE_TRANSIENT
      );
      sqlite3_bind_int64(
          child,
          3,
          static_cast<sqlite3_int64>(
              execution.level_index
          )
      );
      sqlite3_bind_double(
          child,
          4,
          execution.requested_quantity
      );
      sqlite3_bind_double(
          child,
          5,
          execution.filled_quantity
      );
      sqlite3_bind_double(
          child,
          6,
          execution.remaining_quantity
      );
      sqlite3_bind_double(
          child,
          7,
          execution.price
      );
      sqlite3_bind_double(
          child,
          8,
          execution.notional
      );
      sqlite3_bind_double(
          child,
          9,
          execution.fee
      );
      sqlite3_bind_double(
          child,
          10,
          execution.latency_ms
      );

      const std::string status =
          to_string(execution.status);

      sqlite3_bind_text(
          child,
          11,
          status.c_str(),
          -1,
          SQLITE_TRANSIENT
      );

      check_sqlite(
          sqlite3_step(child),
          database_,
          "insert child execution"
      );
    }

    execute("COMMIT;");
    return execution_id;
  } catch (...) {
    try {
      execute("ROLLBACK;");
    } catch (...) {
    }

    throw;
  }
}

std::int64_t ExecutionStore::route_count() const {
  Statement statement(
      database_,
      "SELECT COUNT(*) FROM route_executions;"
  );

  check_sqlite(
      sqlite3_step(statement.get()),
      database_,
      "count route executions"
  );

  return sqlite3_column_int64(statement.get(), 0);
}

std::int64_t ExecutionStore::child_count() const {
  Statement statement(
      database_,
      "SELECT COUNT(*) FROM child_executions;"
  );

  check_sqlite(
      sqlite3_step(statement.get()),
      database_,
      "count child executions"
  );

  return sqlite3_column_int64(statement.get(), 0);
}

}  // namespace minimatch
