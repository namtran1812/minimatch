#include "minimatch/system_settings_store.hpp"

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

SystemSettingsStore::SystemSettingsStore(
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
      "open system settings database"
  );

  sqlite3_busy_timeout(
      database_,
      30000
  );

  initialize_schema();
}

SystemSettingsStore::~SystemSettingsStore() {
  if (database_ != nullptr) {
    sqlite3_close(
        database_
    );
  }
}

void SystemSettingsStore::initialize_schema() {
  const char* sql = R"SQL(
    CREATE TABLE IF NOT EXISTS
    system_settings (
      key TEXT PRIMARY KEY,
      value TEXT NOT NULL
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
      "create system settings schema"
  );
}

void SystemSettingsStore::save_portfolio_limits(
    const PortfolioRiskLimits& limits
) {
  const char* sql = R"SQL(
    INSERT INTO system_settings (
      key,
      value
    )
    VALUES (?, ?)
    ON CONFLICT(key)
    DO UPDATE SET
      value = excluded.value;
  )SQL";

  const auto save =
      [&](const char* key,
          double value) {
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
            "prepare portfolio setting"
        );

        sqlite3_bind_text(
            stmt,
            1,
            key,
            -1,
            SQLITE_TRANSIENT
        );

        const auto text =
            std::to_string(
                value
            );

        sqlite3_bind_text(
            stmt,
            2,
            text.c_str(),
            -1,
            SQLITE_TRANSIENT
        );

        check_sqlite(
            sqlite3_step(
                stmt
            ),
            database_,
            "save portfolio setting"
        );

        sqlite3_finalize(
            stmt
        );
      };

  save(
      "portfolio.max_gross_exposure",
      limits.max_gross_exposure
  );

  save(
      "portfolio.max_net_exposure",
      limits.max_net_exposure
  );

  save(
      "portfolio.max_concentration_percent",
      limits.max_concentration_percent
  );

  save(
      "portfolio.max_daily_loss",
      limits.max_daily_loss
  );
}

PortfolioRiskLimits
SystemSettingsStore::load_portfolio_limits()
    const {
  PortfolioRiskLimits result;

  const char* sql = R"SQL(
    SELECT value
    FROM system_settings
    WHERE key = ?;
  )SQL";

  const auto load =
      [&](const char* key,
          double fallback) {
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
            "prepare portfolio load"
        );

        sqlite3_bind_text(
            stmt,
            1,
            key,
            -1,
            SQLITE_TRANSIENT
        );

        double value =
            fallback;

        if (
            sqlite3_step(
                stmt
            ) ==
            SQLITE_ROW
        ) {
          const auto* text =
              reinterpret_cast<
                  const char*
              >(
                  sqlite3_column_text(
                      stmt,
                      0
                  )
              );

          if (text != nullptr) {
            value =
                std::stod(
                    text
                );
          }
        }

        sqlite3_finalize(
            stmt
        );

        return value;
      };

  result.max_gross_exposure =
      load(
          "portfolio.max_gross_exposure",
          result.max_gross_exposure
      );

  result.max_net_exposure =
      load(
          "portfolio.max_net_exposure",
          result.max_net_exposure
      );

  result.max_concentration_percent =
      load(
          "portfolio.max_concentration_percent",
          result.max_concentration_percent
      );

  result.max_daily_loss =
      load(
          "portfolio.max_daily_loss",
          result.max_daily_loss
      );

  return result;
}

void SystemSettingsStore::save_circuit_breaker(
    const CircuitBreakerSettings& settings
) {
  const char* sql = R"SQL(
    INSERT INTO system_settings (
      key,
      value
    )
    VALUES (?, ?)
    ON CONFLICT(key)
    DO UPDATE SET
      value = excluded.value;
  )SQL";

  const auto save =
      [&](const char* key,
          const std::string& value) {
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
            "prepare circuit setting"
        );

        sqlite3_bind_text(
            stmt,
            1,
            key,
            -1,
            SQLITE_TRANSIENT
        );

        sqlite3_bind_text(
            stmt,
            2,
            value.c_str(),
            -1,
            SQLITE_TRANSIENT
        );

        check_sqlite(
            sqlite3_step(
                stmt
            ),
            database_,
            "save circuit setting"
        );

        sqlite3_finalize(
            stmt
        );
      };

  save(
      "circuit.enabled",
      settings.enabled
          ? "1"
          : "0"
  );

  save(
      "circuit.threshold_percent",
      std::to_string(
          settings.threshold_percent
      )
  );

  save(
      "circuit.window_ns",
      std::to_string(
          settings.window_ns
      )
  );
}

CircuitBreakerSettings
SystemSettingsStore::load_circuit_breaker()
    const {
  CircuitBreakerSettings result;

  const char* sql = R"SQL(
    SELECT value
    FROM system_settings
    WHERE key = ?;
  )SQL";

  const auto load =
      [&](const char* key)
          -> std::string {
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
            "prepare circuit load"
        );

        sqlite3_bind_text(
            stmt,
            1,
            key,
            -1,
            SQLITE_TRANSIENT
        );

        std::string value;

        if (
            sqlite3_step(
                stmt
            ) ==
            SQLITE_ROW
        ) {
          const auto* text =
              reinterpret_cast<
                  const char*
              >(
                  sqlite3_column_text(
                      stmt,
                      0
                  )
              );

          if (text != nullptr) {
            value =
                text;
          }
        }

        sqlite3_finalize(
            stmt
        );

        return value;
      };

  const auto enabled =
      load(
          "circuit.enabled"
      );

  if (!enabled.empty()) {
    result.enabled =
        enabled == "1";
  }

  const auto threshold =
      load(
          "circuit.threshold_percent"
      );

  if (!threshold.empty()) {
    result.threshold_percent =
        std::stod(
            threshold
        );
  }

  const auto window =
      load(
          "circuit.window_ns"
      );

  if (!window.empty()) {
    result.window_ns =
        std::stoull(
            window
        );
  }

  return result;
}

}  // namespace minimatch
