#include "minimatch/midpoint_store.hpp"

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

MidpointStore::MidpointStore(
    const std::string& database_path
) : database_path_(
        database_path
    ) {
  const std::filesystem::path path(
      database_path
  );

  if (path.has_parent_path()) {
    std::filesystem::
        create_directories(
            path.parent_path()
        );
  }

  check_sqlite(
      sqlite3_open(
          database_path.c_str(),
          &database_
      ),
      database_,
      "open midpoint database"
  );

  sqlite3_busy_timeout(
      database_,
      30000
  );

  initialize_schema();
}

MidpointStore::~MidpointStore() {
  if (database_ != nullptr) {
    sqlite3_close(
        database_
    );
  }
}

void MidpointStore::initialize_schema() {
  const char* sql = R"SQL(
    CREATE TABLE IF NOT EXISTS
    midpoint_observations (
      symbol TEXT NOT NULL,
      timestamp_ns INTEGER NOT NULL,
      midpoint REAL NOT NULL,
      PRIMARY KEY (
        symbol,
        timestamp_ns
      )
    );

    CREATE INDEX IF NOT EXISTS
    idx_midpoint_symbol_timestamp
    ON midpoint_observations (
      symbol,
      timestamp_ns
    );
  )SQL";

  check_sqlite(
      sqlite3_exec(
          database_,
          sql,
          nullptr,
          nullptr,
          nullptr
      ),
      database_,
      "create midpoint schema"
  );
}

void MidpointStore::save(
    const std::string& symbol,
    std::uint64_t timestamp_ns,
    double midpoint
) {
  const char* sql = R"SQL(
    INSERT OR REPLACE INTO
    midpoint_observations (
      symbol,
      timestamp_ns,
      midpoint
    )
    VALUES (?, ?, ?);
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
      "prepare midpoint save"
  );

  sqlite3_bind_text(
      stmt,
      1,
      symbol.c_str(),
      -1,
      SQLITE_TRANSIENT
  );

  sqlite3_bind_int64(
      stmt,
      2,
      static_cast<sqlite3_int64>(
          timestamp_ns
      )
  );

  sqlite3_bind_double(
      stmt,
      3,
      midpoint
  );

  check_sqlite(
      sqlite3_step(
          stmt
      ),
      database_,
      "save midpoint"
  );

  sqlite3_finalize(
      stmt
  );
}

std::vector<MidpointObservation>
MidpointStore::load_since(
    const std::string& symbol,
    std::uint64_t timestamp_ns
) const {
  const char* sql = R"SQL(
    SELECT
      timestamp_ns,
      midpoint
    FROM midpoint_observations
    WHERE symbol = ?
      AND timestamp_ns >= ?
    ORDER BY timestamp_ns ASC;
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
      "prepare midpoint load"
  );

  sqlite3_bind_text(
      stmt,
      1,
      symbol.c_str(),
      -1,
      SQLITE_TRANSIENT
  );

  sqlite3_bind_int64(
      stmt,
      2,
      static_cast<sqlite3_int64>(
          timestamp_ns
      )
  );

  std::vector<MidpointObservation>
      observations;

  while (
      sqlite3_step(
          stmt
      ) == SQLITE_ROW
  ) {
    observations.push_back(
        {
            .timestamp_ns =
                static_cast<std::uint64_t>(
                    sqlite3_column_int64(
                        stmt,
                        0
                    )
                ),
            .midpoint =
                sqlite3_column_double(
                    stmt,
                    1
                )
        }
    );
  }

  sqlite3_finalize(
      stmt
  );

  return observations;
}

void MidpointStore::prune_before(
    std::uint64_t timestamp_ns
) {
  const char* sql = R"SQL(
    DELETE FROM midpoint_observations
    WHERE timestamp_ns < ?;
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
      "prepare midpoint prune"
  );

  sqlite3_bind_int64(
      stmt,
      1,
      static_cast<sqlite3_int64>(
          timestamp_ns
      )
  );

  check_sqlite(
      sqlite3_step(
          stmt
      ),
      database_,
      "prune midpoints"
  );

  sqlite3_finalize(
      stmt
  );
}

}  // namespace minimatch
