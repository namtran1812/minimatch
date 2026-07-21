#pragma once

#include "minimatch/portfolio_risk.hpp"

#include <cstdint>
#include <string>

struct sqlite3;

namespace minimatch {

struct CircuitBreakerSettings {
  bool enabled{false};
  double threshold_percent{5.0};
  std::uint64_t window_ns{
      30'000'000'000ULL
  };
};

class SystemSettingsStore {
 public:
  explicit SystemSettingsStore(
      const std::string& database_path
  );

  ~SystemSettingsStore();

  SystemSettingsStore(
      const SystemSettingsStore&
  ) = delete;

  SystemSettingsStore& operator=(
      const SystemSettingsStore&
  ) = delete;

  void save_portfolio_limits(
      const PortfolioRiskLimits& limits
  );

  [[nodiscard]]
  PortfolioRiskLimits
  load_portfolio_limits() const;

  void save_circuit_breaker(
      const CircuitBreakerSettings& settings
  );

  [[nodiscard]]
  CircuitBreakerSettings
  load_circuit_breaker() const;

 private:
  void initialize_schema();

  std::string database_path_;
  sqlite3* database_{nullptr};
};

}  // namespace minimatch
