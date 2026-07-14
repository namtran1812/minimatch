#pragma once

#include "minimatch/execution_engine.hpp"
#include "minimatch/router.hpp"

#include <cstdint>
#include <string>

struct sqlite3;

namespace minimatch {

class ExecutionStore {
 public:
  explicit ExecutionStore(const std::string& database_path);
  ~ExecutionStore();

  ExecutionStore(const ExecutionStore&) = delete;
  ExecutionStore& operator=(const ExecutionStore&) = delete;

  ExecutionStore(ExecutionStore&&) = delete;
  ExecutionStore& operator=(ExecutionStore&&) = delete;

  std::int64_t save(
      const std::string& symbol,
      RouteSide side,
      const ExecutionSimulationConfig& config,
      const RoutedExecutionSummary& summary
  );

  [[nodiscard]] std::int64_t route_count() const;
  [[nodiscard]] std::int64_t child_count() const;

 private:
  sqlite3* database_{nullptr};

  void execute(const char* sql) const;
  void initialize_schema();
};

}  // namespace minimatch
