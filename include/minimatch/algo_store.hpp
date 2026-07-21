#pragma once

#include "minimatch/algo_scheduler.hpp"

#include <string>
#include <vector>

struct sqlite3;

namespace minimatch {

struct PersistedAlgoOrder {
  AlgoOrderState state;
  LiveAlgoRequest request;
  bool has_request{false};
};

class AlgoStore {
 public:
  explicit AlgoStore(
      const std::string& database_path
  );

  ~AlgoStore();

  AlgoStore(
      const AlgoStore&
  ) = delete;

  AlgoStore& operator=(
      const AlgoStore&
  ) = delete;

  void save(
      const AlgoOrderState& state
  );

  void save_request(
      ParentOrderId parent_order_id,
      const LiveAlgoRequest& request
  );

  [[nodiscard]]
  std::vector<AlgoOrderState>
  load() const;

  [[nodiscard]]
  std::vector<PersistedAlgoOrder>
  load_recoverable() const;

 private:
  void initialize_schema();
  void execute(
      const char* sql
  ) const;

  std::string database_path_;
  sqlite3* database_{nullptr};
};

}  // namespace minimatch
