#pragma once
#include "minimatch/types.hpp"
#include <cstdint>

namespace minimatch {
struct QueueState {
  Quantity ahead{};
  Quantity own_remaining{};
  Quantity filled{};
  std::uint64_t events{};
};

class QueuePositionModel {
 public:
  QueuePositionModel(Quantity ahead, Quantity own_quantity);
  void on_trade(Quantity traded_at_level) noexcept;
  void on_cancel_ahead(Quantity cancelled_ahead) noexcept;
  [[nodiscard]] const QueueState& state() const noexcept { return state_; }
  [[nodiscard]] bool complete() const noexcept { return state_.own_remaining == 0; }
 private:
  QueueState state_;
};
}  // namespace minimatch
