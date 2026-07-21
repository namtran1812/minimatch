#pragma once

#include "minimatch/midpoint_history.hpp"

#include <cstdint>
#include <string>
#include <vector>

struct sqlite3;

namespace minimatch {

class MidpointStore {
 public:
  explicit MidpointStore(
      const std::string& database_path
  );

  ~MidpointStore();

  MidpointStore(
      const MidpointStore&
  ) = delete;

  MidpointStore& operator=(
      const MidpointStore&
  ) = delete;

  void save(
      const std::string& symbol,
      std::uint64_t timestamp_ns,
      double midpoint
  );

  [[nodiscard]]
  std::vector<MidpointObservation>
  load_since(
      const std::string& symbol,
      std::uint64_t timestamp_ns
  ) const;

  void prune_before(
      std::uint64_t timestamp_ns
  );

 private:
  void initialize_schema();

  std::string database_path_;
  sqlite3* database_{nullptr};
};

}  // namespace minimatch
