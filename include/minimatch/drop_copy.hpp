#pragma once

#include "minimatch/types.hpp"

#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

struct sqlite3;

namespace minimatch {

struct DropCopyEvent {
  std::uint64_t id{0};
  std::uint64_t timestamp_ns{0};

  OrderId order_id{0};
  SymbolId symbol{DEFAULT_SYMBOL};

  std::string event_type;
  std::string status;

  Quantity remaining{0};
  RejectReason reject_reason{
      RejectReason::None
  };
};

class DropCopyStore {
 public:
  explicit DropCopyStore(
      std::string database_path,
      std::size_t capacity = 10'000
  );

  ~DropCopyStore();

  DropCopyStore(
      const DropCopyStore&
  ) = delete;

  DropCopyStore& operator=(
      const DropCopyStore&
  ) = delete;

  void publish(
      DropCopyEvent event
  );

  [[nodiscard]]
  std::vector<DropCopyEvent>
  recent(
      std::size_t limit = 100
  ) const;

  [[nodiscard]]
  std::vector<DropCopyEvent>
  events_for_order(
      OrderId order_id
  ) const;

 private:
  void initialize_schema();

  void persist(
      const DropCopyEvent& event
  );

  void load_recent();

  std::string database_path_;
  std::size_t capacity_;

  mutable std::mutex mutex_;

  std::deque<
      DropCopyEvent
  > events_;

  std::uint64_t next_id_{1};

  sqlite3* database_{nullptr};
};

}  // namespace minimatch
