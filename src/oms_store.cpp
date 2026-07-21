#include "minimatch/oms_store.hpp"

#include <sqlite3.h>

#include <algorithm>
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
  Statement(
      sqlite3* database,
      const char* sql
  ) : database_(database) {
    check_sqlite(
        sqlite3_prepare_v2(
            database_,
            sql,
            -1,
            &statement_,
            nullptr
        ),
        database_,
        "prepare OMS statement"
    );
  }

  ~Statement() {
    if (statement_ != nullptr) {
      sqlite3_finalize(statement_);
    }
  }

  Statement(const Statement&) = delete;
  Statement& operator=(
      const Statement&
  ) = delete;

  sqlite3_stmt* get() const {
    return statement_;
  }

 private:
  sqlite3* database_{nullptr};
  sqlite3_stmt* statement_{nullptr};
};

std::string text_column(
    sqlite3_stmt* statement,
    int index
) {
  const unsigned char* value =
      sqlite3_column_text(
          statement,
          index
      );

  return value == nullptr
      ? std::string{}
      : std::string(
            reinterpret_cast<
                const char*
            >(value)
        );
}

RouteSide parse_side(
    const std::string& value
) {
  return value == "SELL"
      ? RouteSide::Sell
      : RouteSide::Buy;
}

ExecutionAlgorithm parse_algorithm(
    const std::string& value
) {
  if (value == "TWAP") {
    return ExecutionAlgorithm::TWAP;
  }

  if (value == "VWAP") {
    return ExecutionAlgorithm::VWAP;
  }

  if (value == "POV") {
    return ExecutionAlgorithm::POV;
  }

  if (value == "ICEBERG") {
    return ExecutionAlgorithm::Iceberg;
  }

  return ExecutionAlgorithm::Market;
}

OrderStatus parse_status(
    const std::string& value
) {
  if (value == "WORKING") {
    return OrderStatus::Working;
  }

  if (value == "PARTIALLY_FILLED") {
    return OrderStatus::PartiallyFilled;
  }

  if (value == "FILLED") {
    return OrderStatus::Filled;
  }

  if (value == "CANCELLED") {
    return OrderStatus::Cancelled;
  }

  if (value == "REJECTED") {
    return OrderStatus::Rejected;
  }

  return OrderStatus::New;
}

std::string algorithm_string(
    ExecutionAlgorithm algorithm
) {
  switch (algorithm) {
    case ExecutionAlgorithm::TWAP:
      return "TWAP";

    case ExecutionAlgorithm::VWAP:
      return "VWAP";

    case ExecutionAlgorithm::POV:
      return "POV";

    case ExecutionAlgorithm::Iceberg:
      return "ICEBERG";

    case ExecutionAlgorithm::Market:
      return "MARKET";
  }

  return "MARKET";
}

}  // namespace

OmsStore::OmsStore(
    const std::string& database_path
) : database_path_(database_path) {
  const std::filesystem::path path(
      database_path
  );

  if (path.has_parent_path()) {
    std::filesystem::create_directories(
        path.parent_path()
    );
  }

  check_sqlite(
      sqlite3_open(
          database_path.c_str(),
          &database_
      ),
      database_,
      "open OMS database"
  );

  sqlite3_busy_timeout(
      database_,
      30000
  );

  execute(
      "PRAGMA journal_mode=WAL;"
  );

  execute(
      "PRAGMA foreign_keys=ON;"
  );

  execute(
      "PRAGMA synchronous=NORMAL;"
  );

  initialize_schema();
}

OmsStore::~OmsStore() {
  if (database_ != nullptr) {
    sqlite3_close(database_);
  }
}

void OmsStore::execute(
    const char* sql
) const {
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
        "execute OMS SQLite statement: " +
        message
    );
  }
}

void OmsStore::initialize_schema() {
  execute(R"SQL(
    CREATE TABLE IF NOT EXISTS oms_parents (
      parent_id INTEGER PRIMARY KEY,
      symbol TEXT NOT NULL,
      side TEXT NOT NULL,
      algorithm TEXT NOT NULL,

      quantity REAL NOT NULL,
      filled_quantity REAL NOT NULL,
      remaining_quantity REAL NOT NULL,

      status TEXT NOT NULL,

      created_timestamp_ns INTEGER NOT NULL,
      updated_timestamp_ns INTEGER NOT NULL
    );

    CREATE TABLE IF NOT EXISTS oms_children (
      child_id INTEGER PRIMARY KEY,
      parent_id INTEGER NOT NULL,

      venue TEXT NOT NULL,

      price REAL NOT NULL,
      quantity REAL NOT NULL,
      filled_quantity REAL NOT NULL,
      remaining_quantity REAL NOT NULL,

      status TEXT NOT NULL,

      created_timestamp_ns INTEGER NOT NULL,
      updated_timestamp_ns INTEGER NOT NULL,

      FOREIGN KEY(parent_id)
        REFERENCES oms_parents(parent_id)
        ON DELETE CASCADE
    );

    CREATE TABLE IF NOT EXISTS oms_fills (
      execution_report_id INTEGER PRIMARY KEY,

      parent_id INTEGER NOT NULL,
      child_id INTEGER NOT NULL,

      venue TEXT NOT NULL,

      price REAL NOT NULL,
      quantity REAL NOT NULL,
      notional REAL NOT NULL,
      fee REAL NOT NULL,

      timestamp_ns INTEGER NOT NULL,

      FOREIGN KEY(parent_id)
        REFERENCES oms_parents(parent_id)
        ON DELETE CASCADE,

      FOREIGN KEY(child_id)
        REFERENCES oms_children(child_id)
        ON DELETE CASCADE
    );

    CREATE INDEX IF NOT EXISTS
      idx_oms_children_parent
    ON oms_children(parent_id);

    CREATE INDEX IF NOT EXISTS
      idx_oms_fills_parent
    ON oms_fills(parent_id);

    CREATE INDEX IF NOT EXISTS
      idx_oms_fills_child
    ON oms_fills(child_id);
  )SQL");
}

void OmsStore::save_parent(
    const ParentOrder& parent
) {
  Statement statement(
      database_,
      R"SQL(
        INSERT INTO oms_parents (
          parent_id,
          symbol,
          side,
          algorithm,
          quantity,
          filled_quantity,
          remaining_quantity,
          status,
          created_timestamp_ns,
          updated_timestamp_ns
        )
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(parent_id)
        DO UPDATE SET
          symbol = excluded.symbol,
          side = excluded.side,
          algorithm = excluded.algorithm,
          quantity = excluded.quantity,
          filled_quantity =
              excluded.filled_quantity,
          remaining_quantity =
              excluded.remaining_quantity,
          status = excluded.status,
          updated_timestamp_ns =
              excluded.updated_timestamp_ns;
      )SQL"
  );

  sqlite3_stmt* stmt =
      statement.get();

  const std::string side =
      parent.side == RouteSide::Buy
          ? "BUY"
          : "SELL";

  const std::string algorithm =
      algorithm_string(
          parent.algorithm
      );

  const std::string status =
      to_string(parent.status);

  sqlite3_bind_int64(
      stmt,
      1,
      static_cast<sqlite3_int64>(
          parent.id
      )
  );

  sqlite3_bind_text(
      stmt,
      2,
      parent.symbol.c_str(),
      -1,
      SQLITE_TRANSIENT
  );

  sqlite3_bind_text(
      stmt,
      3,
      side.c_str(),
      -1,
      SQLITE_TRANSIENT
  );

  sqlite3_bind_text(
      stmt,
      4,
      algorithm.c_str(),
      -1,
      SQLITE_TRANSIENT
  );

  sqlite3_bind_double(
      stmt,
      5,
      parent.quantity
  );

  sqlite3_bind_double(
      stmt,
      6,
      parent.filled_quantity
  );

  sqlite3_bind_double(
      stmt,
      7,
      parent.remaining_quantity
  );

  sqlite3_bind_text(
      stmt,
      8,
      status.c_str(),
      -1,
      SQLITE_TRANSIENT
  );

  sqlite3_bind_int64(
      stmt,
      9,
      static_cast<sqlite3_int64>(
          parent.created_timestamp_ns
      )
  );

  sqlite3_bind_int64(
      stmt,
      10,
      static_cast<sqlite3_int64>(
          parent.updated_timestamp_ns
      )
  );

  check_sqlite(
      sqlite3_step(stmt),
      database_,
      "save OMS parent"
  );
}

void OmsStore::save_child(
    const ChildOrder& child
) {
  Statement statement(
      database_,
      R"SQL(
        INSERT INTO oms_children (
          child_id,
          parent_id,
          venue,
          price,
          quantity,
          filled_quantity,
          remaining_quantity,
          status,
          created_timestamp_ns,
          updated_timestamp_ns
        )
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(child_id)
        DO UPDATE SET
          parent_id = excluded.parent_id,
          venue = excluded.venue,
          price = excluded.price,
          quantity = excluded.quantity,
          filled_quantity =
              excluded.filled_quantity,
          remaining_quantity =
              excluded.remaining_quantity,
          status = excluded.status,
          updated_timestamp_ns =
              excluded.updated_timestamp_ns;
      )SQL"
  );

  sqlite3_stmt* stmt =
      statement.get();

  const std::string status =
      to_string(child.status);

  sqlite3_bind_int64(
      stmt,
      1,
      static_cast<sqlite3_int64>(
          child.id
      )
  );

  sqlite3_bind_int64(
      stmt,
      2,
      static_cast<sqlite3_int64>(
          child.parent_id
      )
  );

  sqlite3_bind_text(
      stmt,
      3,
      child.venue.c_str(),
      -1,
      SQLITE_TRANSIENT
  );

  sqlite3_bind_double(
      stmt,
      4,
      child.price
  );

  sqlite3_bind_double(
      stmt,
      5,
      child.quantity
  );

  sqlite3_bind_double(
      stmt,
      6,
      child.filled_quantity
  );

  sqlite3_bind_double(
      stmt,
      7,
      child.remaining_quantity
  );

  sqlite3_bind_text(
      stmt,
      8,
      status.c_str(),
      -1,
      SQLITE_TRANSIENT
  );

  sqlite3_bind_int64(
      stmt,
      9,
      static_cast<sqlite3_int64>(
          child.created_timestamp_ns
      )
  );

  sqlite3_bind_int64(
      stmt,
      10,
      static_cast<sqlite3_int64>(
          child.updated_timestamp_ns
      )
  );

  check_sqlite(
      sqlite3_step(stmt),
      database_,
      "save OMS child"
  );
}

void OmsStore::save_fill(
    const OmsExecutionReport& fill
) {
  Statement statement(
      database_,
      R"SQL(
        INSERT INTO oms_fills (
          execution_report_id,
          parent_id,
          child_id,
          venue,
          price,
          quantity,
          notional,
          fee,
          timestamp_ns
        )
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(execution_report_id)
        DO NOTHING;
      )SQL"
  );

  sqlite3_stmt* stmt =
      statement.get();

  sqlite3_bind_int64(
      stmt,
      1,
      static_cast<sqlite3_int64>(
          fill.execution_report_id
      )
  );

  sqlite3_bind_int64(
      stmt,
      2,
      static_cast<sqlite3_int64>(
          fill.parent_id
      )
  );

  sqlite3_bind_int64(
      stmt,
      3,
      static_cast<sqlite3_int64>(
          fill.child_id
      )
  );

  sqlite3_bind_text(
      stmt,
      4,
      fill.venue.c_str(),
      -1,
      SQLITE_TRANSIENT
  );

  sqlite3_bind_double(
      stmt,
      5,
      fill.price
  );

  sqlite3_bind_double(
      stmt,
      6,
      fill.quantity
  );

  sqlite3_bind_double(
      stmt,
      7,
      fill.notional
  );

  sqlite3_bind_double(
      stmt,
      8,
      fill.fee
  );

  sqlite3_bind_int64(
      stmt,
      9,
      static_cast<sqlite3_int64>(
          fill.timestamp_ns
      )
  );

  check_sqlite(
      sqlite3_step(stmt),
      database_,
      "save OMS fill"
  );
}

void OmsStore::save_parent_and_children(
    const ParentOrder& parent,
    const std::vector<ChildOrder>& children
) {
  execute(
      "BEGIN IMMEDIATE TRANSACTION;"
  );

  try {
    save_parent(parent);

    for (const auto& child : children) {
      save_child(child);
    }

    execute("COMMIT;");
  } catch (...) {
    try {
      execute("ROLLBACK;");
    } catch (...) {
    }

    throw;
  }
}

OmsRecoveryState OmsStore::load() const {
  OmsRecoveryState state;

  {
    Statement statement(
        database_,
        R"SQL(
          SELECT
            parent_id,
            symbol,
            side,
            algorithm,
            quantity,
            filled_quantity,
            remaining_quantity,
            status,
            created_timestamp_ns,
            updated_timestamp_ns
          FROM oms_parents
          ORDER BY parent_id;
        )SQL"
    );

    while (
        sqlite3_step(statement.get()) ==
        SQLITE_ROW
    ) {
      ParentOrder parent;

      parent.id =
          static_cast<ParentOrderId>(
              sqlite3_column_int64(
                  statement.get(),
                  0
              )
          );

      parent.symbol =
          text_column(
              statement.get(),
              1
          );

      parent.side =
          parse_side(
              text_column(
                  statement.get(),
                  2
              )
          );

      parent.algorithm =
          parse_algorithm(
              text_column(
                  statement.get(),
                  3
              )
          );

      parent.quantity =
          sqlite3_column_double(
              statement.get(),
              4
          );

      parent.filled_quantity =
          sqlite3_column_double(
              statement.get(),
              5
          );

      parent.remaining_quantity =
          sqlite3_column_double(
              statement.get(),
              6
          );

      parent.status =
          parse_status(
              text_column(
                  statement.get(),
                  7
              )
          );

      parent.created_timestamp_ns =
          static_cast<std::uint64_t>(
              sqlite3_column_int64(
                  statement.get(),
                  8
              )
          );

      parent.updated_timestamp_ns =
          static_cast<std::uint64_t>(
              sqlite3_column_int64(
                  statement.get(),
                  9
              )
          );

      state.next_parent_id =
          std::max(
              state.next_parent_id,
              parent.id + 1
          );

      state.parents.push_back(
          std::move(parent)
      );
    }
  }

  {
    Statement statement(
        database_,
        R"SQL(
          SELECT
            child_id,
            parent_id,
            venue,
            price,
            quantity,
            filled_quantity,
            remaining_quantity,
            status,
            created_timestamp_ns,
            updated_timestamp_ns
          FROM oms_children
          ORDER BY child_id;
        )SQL"
    );

    while (
        sqlite3_step(statement.get()) ==
        SQLITE_ROW
    ) {
      ChildOrder child;

      child.id =
          static_cast<ChildOrderId>(
              sqlite3_column_int64(
                  statement.get(),
                  0
              )
          );

      child.parent_id =
          static_cast<ParentOrderId>(
              sqlite3_column_int64(
                  statement.get(),
                  1
              )
          );

      child.venue =
          text_column(
              statement.get(),
              2
          );

      child.price =
          sqlite3_column_double(
              statement.get(),
              3
          );

      child.quantity =
          sqlite3_column_double(
              statement.get(),
              4
          );

      child.filled_quantity =
          sqlite3_column_double(
              statement.get(),
              5
          );

      child.remaining_quantity =
          sqlite3_column_double(
              statement.get(),
              6
          );

      child.status =
          parse_status(
              text_column(
                  statement.get(),
                  7
              )
          );

      child.created_timestamp_ns =
          static_cast<std::uint64_t>(
              sqlite3_column_int64(
                  statement.get(),
                  8
              )
          );

      child.updated_timestamp_ns =
          static_cast<std::uint64_t>(
              sqlite3_column_int64(
                  statement.get(),
                  9
              )
          );

      state.next_child_id =
          std::max(
              state.next_child_id,
              child.id + 1
          );

      state.children.push_back(
          std::move(child)
      );
    }
  }

  for (const auto& child :
       state.children) {
    for (auto& parent :
         state.parents) {
      if (parent.id ==
          child.parent_id) {
        parent.child_ids.push_back(
            child.id
        );

        break;
      }
    }
  }

  {
    Statement statement(
        database_,
        R"SQL(
          SELECT
            execution_report_id,
            parent_id,
            child_id,
            venue,
            price,
            quantity,
            notional,
            fee,
            timestamp_ns
          FROM oms_fills
          ORDER BY execution_report_id;
        )SQL"
    );

    while (
        sqlite3_step(statement.get()) ==
        SQLITE_ROW
    ) {
      OmsExecutionReport fill;

      fill.execution_report_id =
          static_cast<std::uint64_t>(
              sqlite3_column_int64(
                  statement.get(),
                  0
              )
          );

      fill.parent_id =
          static_cast<ParentOrderId>(
              sqlite3_column_int64(
                  statement.get(),
                  1
              )
          );

      fill.child_id =
          static_cast<ChildOrderId>(
              sqlite3_column_int64(
                  statement.get(),
                  2
              )
          );

      fill.venue =
          text_column(
              statement.get(),
              3
          );

      fill.price =
          sqlite3_column_double(
              statement.get(),
              4
          );

      fill.quantity =
          sqlite3_column_double(
              statement.get(),
              5
          );

      fill.notional =
          sqlite3_column_double(
              statement.get(),
              6
          );

      fill.fee =
          sqlite3_column_double(
              statement.get(),
              7
          );

      fill.timestamp_ns =
          static_cast<std::uint64_t>(
              sqlite3_column_int64(
                  statement.get(),
                  8
              )
          );

      state.next_execution_report_id =
          std::max(
              state.next_execution_report_id,
              fill.execution_report_id + 1
          );

      state.fills.push_back(
          std::move(fill)
      );
    }
  }

  return state;
}

}  // namespace minimatch

namespace minimatch {

void OmsStore::save_snapshot(
    const OrderManagementSystem& oms
) {
  execute(
      "BEGIN IMMEDIATE TRANSACTION;"
  );

  try {
    const auto parents =
        oms.parents();

    for (const auto& parent :
         parents) {
      save_parent(parent);

      const auto children =
          oms.children_for_parent(
              parent.id
          );

      for (const auto& child :
           children) {
        save_child(child);
      }

      const auto fills =
          oms.fills_for_parent(
              parent.id
          );

      for (const auto& fill :
           fills) {
        save_fill(fill);
      }
    }

    execute("COMMIT;");
  } catch (...) {
    try {
      execute("ROLLBACK;");
    } catch (...) {
    }

    throw;
  }
}

}  // namespace minimatch
