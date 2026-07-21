#include "minimatch/algo_store.hpp"

#include <sqlite3.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <sstream>

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

std::vector<double>
parse_volume_profile(
    const std::string& value
) {
  std::vector<double> result;

  if (value.empty()) {
    return result;
  }

  std::stringstream input(
      value
  );

  std::string token;

  while (
      std::getline(
          input,
          token,
          ','
      )
  ) {
    if (!token.empty()) {
      result.push_back(
          std::stod(
              token
          )
      );
    }
  }

  return result;
}

std::string serialize_volume_profile(
    const std::vector<double>& values
) {
  std::ostringstream out;

  for (
      std::size_t i = 0;
      i < values.size();
      ++i
  ) {
    if (i != 0) {
      out << ",";
    }

    out << values[i];
  }

  return out.str();
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

AlgoOrderStatus parse_status(
    const std::string& value
) {
  if (value == "COMPLETED") {
    return AlgoOrderStatus::Completed;
  }

  if (value == "CANCELLED") {
    return AlgoOrderStatus::Cancelled;
  }

  if (value == "FAILED") {
    return AlgoOrderStatus::Failed;
  }

  return AlgoOrderStatus::Running;
}

}  // namespace

AlgoStore::AlgoStore(
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
      "open algo database"
  );

  sqlite3_busy_timeout(
      database_,
      30000
  );

  execute(
      "PRAGMA journal_mode=WAL;"
  );

  execute(
      "PRAGMA synchronous=NORMAL;"
  );

  initialize_schema();
}

AlgoStore::~AlgoStore() {
  if (database_ != nullptr) {
    sqlite3_close(
        database_
    );
  }
}

void AlgoStore::execute(
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
        "execute algo SQLite statement: " +
        message
    );
  }
}

void AlgoStore::initialize_schema() {
  execute(R"SQL(
    CREATE TABLE IF NOT EXISTS
    algo_orders (
      parent_id INTEGER PRIMARY KEY,

      symbol TEXT NOT NULL,
      algorithm TEXT NOT NULL,
      status TEXT NOT NULL,

      current_slice INTEGER NOT NULL,
      total_slices INTEGER NOT NULL,

      started_at_ns INTEGER NOT NULL,
      next_slice_at_ns INTEGER NOT NULL,
      completed_at_ns INTEGER NOT NULL,

      requested_quantity REAL NOT NULL,
      filled_quantity REAL NOT NULL,
      remaining_quantity REAL NOT NULL,

      arrival_price REAL NOT NULL,

      error TEXT NOT NULL
    );
  )SQL");

  execute(R"SQL(
    CREATE TABLE IF NOT EXISTS
    algo_order_requests (
      parent_id INTEGER PRIMARY KEY,

      symbol TEXT NOT NULL,
      side INTEGER NOT NULL,
      quantity REAL NOT NULL,

      algorithm TEXT NOT NULL,
      slices INTEGER NOT NULL,
      duration_seconds REAL NOT NULL,

      volume_profile TEXT NOT NULL,
      participation_rate REAL NOT NULL,
      displayed_quantity REAL NOT NULL,

      limit_price REAL NOT NULL,
      max_slippage_bps REAL NOT NULL,
      max_venue_count INTEGER NOT NULL,
      all_or_none INTEGER NOT NULL,

      simulation_fill_ratio REAL NOT NULL DEFAULT 1.0,
      simulation_rejection_probability REAL NOT NULL DEFAULT 0.0,
      simulation_base_latency_ms REAL NOT NULL DEFAULT 0.0,
      simulation_latency_jitter_ms REAL NOT NULL DEFAULT 0.0,
      simulation_seed INTEGER NOT NULL DEFAULT 1
    );
  )SQL");
}

void AlgoStore::save(
    const AlgoOrderState& state
) {
  const char* sql = R"SQL(
    INSERT INTO algo_orders (
      parent_id,
      symbol,
      algorithm,
      status,
      current_slice,
      total_slices,
      started_at_ns,
      next_slice_at_ns,
      completed_at_ns,
      requested_quantity,
      filled_quantity,
      remaining_quantity,
      arrival_price,
      error,
      recovery_count,
      last_recovered_at_ns
    )
    VALUES (
      ?, ?, ?, ?, ?, ?, ?,
      ?, ?, ?, ?, ?, ?, ?, ?, ?
    )
    ON CONFLICT(parent_id)
    DO UPDATE SET
      symbol = excluded.symbol,
      algorithm = excluded.algorithm,
      status = excluded.status,
      current_slice =
          excluded.current_slice,
      total_slices =
          excluded.total_slices,
      started_at_ns =
          excluded.started_at_ns,
      next_slice_at_ns =
          excluded.next_slice_at_ns,
      completed_at_ns =
          excluded.completed_at_ns,
      requested_quantity =
          excluded.requested_quantity,
      filled_quantity =
          excluded.filled_quantity,
      remaining_quantity =
          excluded.remaining_quantity,
      arrival_price =
          excluded.arrival_price,
      error = excluded.error,
      recovery_count =
          excluded.recovery_count,
      last_recovered_at_ns =
          excluded.last_recovered_at_ns;
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
      "prepare algo save"
  );

  sqlite3_bind_int64(
      stmt,
      1,
      static_cast<sqlite3_int64>(
          state.parent_order_id
      )
  );

  sqlite3_bind_text(
      stmt,
      2,
      state.symbol.c_str(),
      -1,
      SQLITE_TRANSIENT
  );

  const auto algorithm =
      algorithm_string(
          state.algorithm
      );

  sqlite3_bind_text(
      stmt,
      3,
      algorithm.c_str(),
      -1,
      SQLITE_TRANSIENT
  );

  const auto status =
      to_string(
          state.status
      );

  sqlite3_bind_text(
      stmt,
      4,
      status.c_str(),
      -1,
      SQLITE_TRANSIENT
  );

  sqlite3_bind_int64(
      stmt,
      5,
      static_cast<sqlite3_int64>(
          state.current_slice
      )
  );

  sqlite3_bind_int64(
      stmt,
      6,
      static_cast<sqlite3_int64>(
          state.total_slices
      )
  );

  sqlite3_bind_int64(
      stmt,
      7,
      static_cast<sqlite3_int64>(
          state.started_at_ns
      )
  );

  sqlite3_bind_int64(
      stmt,
      8,
      static_cast<sqlite3_int64>(
          state.next_slice_at_ns
      )
  );

  sqlite3_bind_int64(
      stmt,
      9,
      static_cast<sqlite3_int64>(
          state.completed_at_ns
      )
  );

  sqlite3_bind_double(
      stmt,
      10,
      state.requested_quantity
  );

  sqlite3_bind_double(
      stmt,
      11,
      state.filled_quantity
  );

  sqlite3_bind_double(
      stmt,
      12,
      state.remaining_quantity
  );

  sqlite3_bind_double(
      stmt,
      13,
      state.arrival_price
  );

  sqlite3_bind_text(
      stmt,
      14,
      state.error.c_str(),
      -1,
      SQLITE_TRANSIENT
  );

  sqlite3_bind_int64(
      stmt,
      15,
      static_cast<sqlite3_int64>(
          state.recovery_count
      )
  );

  sqlite3_bind_int64(
      stmt,
      16,
      static_cast<sqlite3_int64>(
          state.last_recovered_at_ns
      )
  );

  check_sqlite(
      sqlite3_step(stmt),
      database_,
      "save algo order"
  );

  sqlite3_finalize(stmt);
}

void AlgoStore::save_request(
    ParentOrderId parent_order_id,
    const LiveAlgoRequest& request
) {
  const char* sql = R"SQL(
    INSERT INTO algo_order_requests (
      parent_id,
      symbol,
      side,
      quantity,
      algorithm,
      slices,
      duration_seconds,
      volume_profile,
      participation_rate,
      displayed_quantity,
      limit_price,
      max_slippage_bps,
      max_venue_count,
      all_or_none,
      simulation_fill_ratio,
      simulation_rejection_probability,
      simulation_base_latency_ms,
      simulation_latency_jitter_ms,
      simulation_seed
    )
    VALUES (
      ?, ?, ?, ?, ?, ?, ?,
      ?, ?, ?, ?, ?, ?, ?,
      ?, ?, ?, ?, ?
    )
    ON CONFLICT(parent_id)
    DO UPDATE SET
      symbol = excluded.symbol,
      side = excluded.side,
      quantity = excluded.quantity,
      algorithm = excluded.algorithm,
      slices = excluded.slices,
      duration_seconds =
          excluded.duration_seconds,
      volume_profile =
          excluded.volume_profile,
      participation_rate =
          excluded.participation_rate,
      displayed_quantity =
          excluded.displayed_quantity,
      limit_price =
          excluded.limit_price,
      max_slippage_bps =
          excluded.max_slippage_bps,
      max_venue_count =
          excluded.max_venue_count,
      all_or_none =
          excluded.all_or_none,
      simulation_fill_ratio =
          excluded.simulation_fill_ratio,
      simulation_rejection_probability =
          excluded.simulation_rejection_probability,
      simulation_base_latency_ms =
          excluded.simulation_base_latency_ms,
      simulation_latency_jitter_ms =
          excluded.simulation_latency_jitter_ms,
      simulation_seed =
          excluded.simulation_seed;
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
      "prepare algo request save"
  );

  sqlite3_bind_int64(
      stmt,
      1,
      static_cast<sqlite3_int64>(
          parent_order_id
      )
  );

  sqlite3_bind_text(
      stmt,
      2,
      request.symbol.c_str(),
      -1,
      SQLITE_TRANSIENT
  );

  sqlite3_bind_int(
      stmt,
      3,
      static_cast<int>(
          request.side
      )
  );

  sqlite3_bind_double(
      stmt,
      4,
      request.quantity
  );

  const auto algorithm =
      algorithm_string(
          request.schedule.algorithm
      );

  sqlite3_bind_text(
      stmt,
      5,
      algorithm.c_str(),
      -1,
      SQLITE_TRANSIENT
  );

  sqlite3_bind_int(
      stmt,
      6,
      request.schedule.slices
  );

  sqlite3_bind_double(
      stmt,
      7,
      request.schedule.duration_seconds
  );

  const auto volume_profile =
      serialize_volume_profile(
          request.schedule.volume_profile
      );

  sqlite3_bind_text(
      stmt,
      8,
      volume_profile.c_str(),
      -1,
      SQLITE_TRANSIENT
  );

  sqlite3_bind_double(
      stmt,
      9,
      request.schedule.participation_rate
  );

  sqlite3_bind_double(
      stmt,
      10,
      request.schedule.displayed_quantity
  );

  sqlite3_bind_double(
      stmt,
      11,
      request.limit_price
  );

  sqlite3_bind_double(
      stmt,
      12,
      request.max_slippage_bps
  );

  sqlite3_bind_int64(
      stmt,
      13,
      static_cast<sqlite3_int64>(
          request.max_venue_count
      )
  );

  sqlite3_bind_int(
      stmt,
      14,
      request.all_or_none
          ? 1
          : 0
  );

  sqlite3_bind_double(
      stmt,
      15,
      request.simulation.fill_ratio
  );

  sqlite3_bind_double(
      stmt,
      16,
      request.simulation
          .rejection_probability
  );

  sqlite3_bind_double(
      stmt,
      17,
      request.simulation
          .base_latency_ms
  );

  sqlite3_bind_double(
      stmt,
      18,
      request.simulation
          .latency_jitter_ms
  );

  sqlite3_bind_int64(
      stmt,
      19,
      static_cast<sqlite3_int64>(
          request.simulation.seed
      )
  );

  check_sqlite(
      sqlite3_step(
          stmt
      ),
      database_,
      "save algo request"
  );

  sqlite3_finalize(
      stmt
  );
}


std::vector<AlgoOrderState>
AlgoStore::load() const {
  std::vector<AlgoOrderState>
      result;

  const char* sql = R"SQL(
    SELECT
      parent_id,
      symbol,
      algorithm,
      status,
      current_slice,
      total_slices,
      started_at_ns,
      next_slice_at_ns,
      completed_at_ns,
      requested_quantity,
      filled_quantity,
      remaining_quantity,
      arrival_price,
      error,
      recovery_count,
      last_recovered_at_ns
    FROM algo_orders
    ORDER BY parent_id;
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
      "prepare algo load"
  );

  while (
      sqlite3_step(stmt) ==
      SQLITE_ROW
  ) {
    AlgoOrderState state;

    state.parent_order_id =
        static_cast<ParentOrderId>(
            sqlite3_column_int64(
                stmt,
                0
            )
        );

    const auto text =
        [stmt](int index) {
          const auto* value =
              sqlite3_column_text(
                  stmt,
                  index
              );

          return value == nullptr
              ? std::string{}
              : std::string(
                    reinterpret_cast<
                        const char*
                    >(value)
                );
        };

    state.symbol =
        text(1);

    state.algorithm =
        parse_algorithm(
            text(2)
        );

    state.status =
        parse_status(
            text(3)
        );

    state.current_slice =
        static_cast<std::size_t>(
            sqlite3_column_int64(
                stmt,
                4
            )
        );

    state.total_slices =
        static_cast<std::size_t>(
            sqlite3_column_int64(
                stmt,
                5
            )
        );

    state.started_at_ns =
        static_cast<std::uint64_t>(
            sqlite3_column_int64(
                stmt,
                6
            )
        );

    state.next_slice_at_ns =
        static_cast<std::uint64_t>(
            sqlite3_column_int64(
                stmt,
                7
            )
        );

    state.completed_at_ns =
        static_cast<std::uint64_t>(
            sqlite3_column_int64(
                stmt,
                8
            )
        );

    state.requested_quantity =
        sqlite3_column_double(
            stmt,
            9
        );

    state.filled_quantity =
        sqlite3_column_double(
            stmt,
            10
        );

    state.remaining_quantity =
        sqlite3_column_double(
            stmt,
            11
        );

    state.arrival_price =
        sqlite3_column_double(
            stmt,
            12
        );

    state.error =
        text(13);

    state.recovery_count =
        static_cast<std::size_t>(
            sqlite3_column_int64(
                stmt,
                14
            )
        );

    state.last_recovered_at_ns =
        static_cast<std::uint64_t>(
            sqlite3_column_int64(
                stmt,
                15
            )
        );

    result.push_back(
        std::move(state)
    );
  }

  sqlite3_finalize(stmt);

  return result;
}

std::vector<PersistedAlgoOrder>
AlgoStore::load_recoverable() const {
  std::vector<PersistedAlgoOrder>
      result;

  const auto states =
      load();

  const char* sql = R"SQL(
    SELECT
      symbol,
      side,
      quantity,
      algorithm,
      slices,
      duration_seconds,
      volume_profile,
      participation_rate,
      displayed_quantity,
      limit_price,
      max_slippage_bps,
      max_venue_count,
      all_or_none,
      simulation_fill_ratio,
      simulation_rejection_probability,
      simulation_base_latency_ms,
      simulation_latency_jitter_ms,
      simulation_seed
    FROM algo_order_requests
    WHERE parent_id = ?;
  )SQL";

  for (const auto& state : states) {
    PersistedAlgoOrder recovered;

    recovered.state =
        state;

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
        "prepare recoverable algo load"
    );

    sqlite3_bind_int64(
        stmt,
        1,
        static_cast<sqlite3_int64>(
            state.parent_order_id
        )
    );

    if (
        sqlite3_step(stmt) ==
        SQLITE_ROW
    ) {
      recovered.has_request =
          true;

      const auto text =
          [stmt](int index) {
            const auto* value =
                sqlite3_column_text(
                    stmt,
                    index
                );

            return value == nullptr
                ? std::string{}
                : std::string(
                      reinterpret_cast<
                          const char*
                      >(value)
                  );
          };

      recovered.request.symbol =
          text(0);

      recovered.request.side =
          static_cast<RouteSide>(
              sqlite3_column_int(
                  stmt,
                  1
              )
          );

      recovered.request.quantity =
          sqlite3_column_double(
              stmt,
              2
          );

      recovered.request
          .schedule.algorithm =
          parse_algorithm(
              text(3)
          );

      recovered.request
          .schedule.quantity =
          recovered.request.quantity;

      recovered.request
          .schedule.slices =
          sqlite3_column_int(
              stmt,
              4
          );

      recovered.request
          .schedule.duration_seconds =
          sqlite3_column_double(
              stmt,
              5
          );

      const auto volume_profile =
          text(6);

      if (!volume_profile.empty()) {
        std::stringstream stream(
            volume_profile
        );

        std::string token;

        while (
            std::getline(
                stream,
                token,
                ','
            )
        ) {
          if (!token.empty()) {
            recovered.request
                .schedule
                .volume_profile
                .push_back(
                    std::stod(
                        token
                    )
                );
          }
        }
      }

      recovered.request
          .schedule.participation_rate =
          sqlite3_column_double(
              stmt,
              7
          );

      recovered.request
          .schedule.displayed_quantity =
          sqlite3_column_double(
              stmt,
              8
          );

      recovered.request.limit_price =
          sqlite3_column_double(
              stmt,
              9
          );

      recovered.request.max_slippage_bps =
          sqlite3_column_double(
              stmt,
              10
          );

      recovered.request.max_venue_count =
          static_cast<std::size_t>(
              sqlite3_column_int64(
                  stmt,
                  11
              )
          );

      recovered.request.all_or_none =
          sqlite3_column_int(
              stmt,
              12
          ) != 0;

      recovered.request
          .simulation.fill_ratio =
          sqlite3_column_double(
              stmt,
              13
          );

      recovered.request
          .simulation.rejection_probability =
          sqlite3_column_double(
              stmt,
              14
          );

      recovered.request
          .simulation.base_latency_ms =
          sqlite3_column_double(
              stmt,
              15
          );

      recovered.request
          .simulation.latency_jitter_ms =
          sqlite3_column_double(
              stmt,
              16
          );

      recovered.request
          .simulation.seed =
          static_cast<std::uint64_t>(
              sqlite3_column_int64(
                  stmt,
                  17
              )
          );
    }

    sqlite3_finalize(
        stmt
    );

    result.push_back(
        std::move(
            recovered
        )
    );
  }

  return result;
}


}  // namespace minimatch
