#include "minimatch/queue_model.hpp"
#include <algorithm>
namespace minimatch {
QueuePositionModel::QueuePositionModel(Quantity ahead, Quantity own_quantity)
    : state_{ahead, own_quantity, 0, 0} {}
void QueuePositionModel::on_trade(Quantity traded_at_level) noexcept {
  ++state_.events;
  const Quantity consume_ahead = std::min(state_.ahead, traded_at_level);
  state_.ahead -= consume_ahead;
  const Quantity residual = traded_at_level - consume_ahead;
  const Quantity own_fill = std::min(state_.own_remaining, residual);
  state_.own_remaining -= own_fill;
  state_.filled += own_fill;
}
void QueuePositionModel::on_cancel_ahead(Quantity cancelled_ahead) noexcept {
  ++state_.events;
  state_.ahead -= std::min(state_.ahead, cancelled_ahead);
}
}  // namespace minimatch
