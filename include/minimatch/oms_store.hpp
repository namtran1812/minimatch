#pragma once

#include "minimatch/oms.hpp"

#include <cstdint>
#include <string>
#include <vector>

struct sqlite3;

namespace minimatch {

struct OmsRecoveryState {
  std::vector<ParentOrder> parents;
  std::vector<ChildOrder> children;
  std::vector<OmsExecutionReport> fills;

  std::uint64_t next_parent_id{1};
  std::uint64_t next_child_id{1};
  std::uint64_t next_execution_report_id{1};
};

class OmsStore {
 public:
  explicit OmsStore(
      const std::string& database_path
  );

  ~OmsStore();

  OmsStore(const OmsStore&) = delete;
  OmsStore& operator=(const OmsStore&) = delete;

  void save_parent(
      const ParentOrder& parent
  );

  void save_child(
      const ChildOrder& child
  );

  void save_fill(
      const OmsExecutionReport& fill
  );

  void save_parent_and_children(
      const ParentOrder& parent,
      const std::vector<ChildOrder>& children
  );

  void save_snapshot(
      const OrderManagementSystem& oms
  );

  [[nodiscard]]
  OmsRecoveryState load() const;

  [[nodiscard]]
  const std::string& database_path() const {
    return database_path_;
  }

 private:
  void initialize_schema();
  void execute(const char* sql) const;

  std::string database_path_;
  sqlite3* database_{nullptr};
};

}  // namespace minimatch
