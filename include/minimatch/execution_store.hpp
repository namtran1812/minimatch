#pragma once

#include "minimatch/execution_engine.hpp"
#include "minimatch/router.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct sqlite3;


namespace minimatch {

struct StoredRouteExecution {
  std::int64_t execution_id{0};
  std::string created_at;
  std::string symbol;
  std::string side;

  double requested_quantity{0.0};
  double filled_quantity{0.0};
  double remaining_quantity{0.0};
  double average_fill_price{0.0};
  double total_notional{0.0};
  double total_fees{0.0};
  double total_latency_ms{0.0};

  bool complete{false};

  double fill_ratio{1.0};
  double rejection_probability{0.0};
  double base_latency_ms{0.0};
  double latency_jitter_ms{0.0};
  std::uint64_t simulation_seed{0};

  std::vector<RoutedChildExecution> children;
};


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

  [[nodiscard]] std::vector<StoredRouteExecution> recent(
      std::size_t limit
  ) const;

  [[nodiscard]] std::optional<StoredRouteExecution> find(
      std::int64_t execution_id
  ) const;

 private:
  sqlite3* database_{nullptr};

  void execute(const char* sql) const;
  void initialize_schema();
};

}  // namespace minimatch
