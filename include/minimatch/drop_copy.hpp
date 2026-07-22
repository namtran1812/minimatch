#pragma once

#include "minimatch/types.hpp"

#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
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
  RejectReason reject_reason{RejectReason::None};
};

class DropCopyStore {
public:
  explicit DropCopyStore(std::string database_path,
                         std::size_t capacity = 10'000);

  ~DropCopyStore();

  DropCopyStore(const DropCopyStore &) = delete;

  DropCopyStore &operator=(const DropCopyStore &) = delete;

  void publish(DropCopyEvent event);

  [[nodiscard]]
  std::vector<DropCopyEvent> recent(std::size_t limit = 100) const;

  [[nodiscard]]
  std::vector<DropCopyEvent> events_for_order(OrderId order_id) const;

private:
  void initialize_schema();

  void persist(const DropCopyEvent &event);

  void load_recent();

  std::string database_path_;
  std::size_t capacity_;

  mutable std::mutex mutex_;

  std::deque<DropCopyEvent> events_;

  // Chronological event history for orders whose full history is
  // still represented by the bounded in-memory cache.
  std::unordered_map<OrderId, std::vector<DropCopyEvent>> events_by_order_;

  // Once an event is evicted for an order, its in-memory history is
  // incomplete and lookups must fall back to SQLite.
  std::unordered_set<OrderId> incomplete_order_history_;

  std::uint64_t next_id_{1};

  sqlite3 *database_{nullptr};
};

} // namespace minimatch
