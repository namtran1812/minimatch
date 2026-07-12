#include "minimatch/order_book.hpp"
#include <algorithm>
#include <stdexcept>

namespace minimatch {
OrderBook::OrderBook(std::size_t capacity, SymbolId symbol) : symbol_(symbol), nodes_(capacity) {
  free_list_.reserve(capacity);
  by_id_.reserve(capacity);
  for (std::size_t i = capacity; i-- > 0;) free_list_.push_back(static_cast<std::uint32_t>(i));
}

bool OrderBook::validate(const OrderRequest& r) {
  RejectReason reason = RejectReason::None;
  if (r.symbol != symbol_) reason = RejectReason::InvalidPrice;
  else if (r.quantity == 0) reason = RejectReason::InvalidQuantity;
  else if (r.type != OrderType::Market && r.price <= 0) reason = RejectReason::InvalidPrice;
  else if (r.type == OrderType::Market && r.price != MARKET_PRICE) reason = RejectReason::InvalidPrice;
  else if (by_id_.contains(r.order_id)) reason = RejectReason::DuplicateOrderId;
  else if (free_list_.empty()) reason = RejectReason::CapacityExceeded;
  if (reason != RejectReason::None) {
    emit_report(r.order_id, ExecutionReport::Status::Rejected, r.quantity, reason);
    return false;
  }
  return true;
}

std::uint32_t OrderBook::allocate_node() {
  if (free_list_.empty()) throw std::runtime_error("order capacity exceeded");
  const auto idx = free_list_.back();
  free_list_.pop_back();
  return idx;
}

void OrderBook::release_node(std::uint32_t idx) {
  nodes_[idx] = Node{};
  free_list_.push_back(idx);
}

bool OrderBook::crosses(const OrderRequest& t, Price p) const noexcept {
  if (t.type == OrderType::Market) return true;
  return t.side == Side::Buy ? t.price >= p : t.price <= p;
}

Quantity OrderBook::available_to_match(const OrderRequest& request) const noexcept {
  std::uint64_t available = 0;
  if (request.side == Side::Buy) {
    for (const auto& [price, level] : asks_) {
      if (!crosses(request, price)) break;
      available += level.total;
      if (available >= request.quantity) return request.quantity;
    }
  } else {
    for (const auto& [price, level] : bids_) {
      if (!crosses(request, price)) break;
      available += level.total;
      if (available >= request.quantity) return request.quantity;
    }
  }
  return static_cast<Quantity>(available);
}

void OrderBook::emit_report(OrderId id, ExecutionReport::Status status, Quantity remaining,
                            RejectReason reason) {
  const auto seq = ++sequence_;
  if (report_handler_) report_handler_(ExecutionReport{seq, id, status, remaining, reason, symbol_});
}

void OrderBook::emit_trade(const Node& maker, const OrderRequest& taker, Price px, Quantity qty) {
  const auto seq = ++sequence_;
  if (trade_handler_) {
    trade_handler_(Trade{seq, maker.id, taker.order_id, px, qty, taker.side, symbol_});
  }
}

template <typename Levels>
void OrderBook::match_against(Levels& levels, const OrderRequest& taker, Quantity& remaining) {
  while (remaining > 0 && !levels.empty()) {
    auto level_it = levels.begin();
    if (!crosses(taker, level_it->first)) break;
    auto& level = level_it->second;
    while (remaining > 0 && level.head != npos) {
      const auto maker_idx = level.head;
      auto& maker = nodes_[maker_idx];
      const Quantity fill = std::min(remaining, maker.remaining);
      remaining -= fill;
      maker.remaining -= fill;
      level.total -= fill;
      emit_trade(maker, taker, maker.price, fill);
      emit_report(maker.id,
                  maker.remaining == 0 ? ExecutionReport::Status::Filled
                                       : ExecutionReport::Status::PartiallyFilled,
                  maker.remaining);
      if (maker.remaining == 0) {
        level.head = maker.next;
        if (level.head != npos) nodes_[level.head].prev = npos;
        else level.tail = npos;
        --level.order_count;
        by_id_.erase(maker.id);
        release_node(maker_idx);
        --active_count_;
      }
    }
    if (level.head == npos) levels.erase(level_it);
  }
}

bool OrderBook::submit(const OrderRequest& r) {
  if (!validate(r)) return false;

  if (r.type == OrderType::PostOnly) {
    const bool would_trade = r.side == Side::Buy ? (!asks_.empty() && r.price >= best_ask())
                                                 : (!bids_.empty() && r.price <= best_bid());
    if (would_trade) {
      emit_report(r.order_id, ExecutionReport::Status::Rejected, r.quantity,
                  RejectReason::WouldTrade);
      return false;
    }
  }

  if (r.type == OrderType::FOK && available_to_match(r) < r.quantity) {
    emit_report(r.order_id, ExecutionReport::Status::Rejected, r.quantity,
                RejectReason::InsufficientLiquidity);
    return false;
  }

  emit_report(r.order_id, ExecutionReport::Status::Accepted, r.quantity);
  Quantity remaining = r.quantity;
  if (r.side == Side::Buy) match_against(asks_, r, remaining);
  else match_against(bids_, r, remaining);

  if (remaining == 0) {
    emit_report(r.order_id, ExecutionReport::Status::Filled, 0);
  } else if (remaining < r.quantity) {
    emit_report(r.order_id, ExecutionReport::Status::PartiallyFilled, remaining);
  }

  if (remaining > 0) {
    if (r.type == OrderType::Limit || r.type == OrderType::PostOnly) rest(r, remaining);
    else emit_report(r.order_id, ExecutionReport::Status::Expired, remaining);
  }
  return true;
}

void OrderBook::rest(const OrderRequest& r, Quantity remaining) {
  const auto idx = allocate_node();
  nodes_[idx] = Node{r.order_id, r.client_id, r.price, remaining, npos, npos, r.side, true};
  auto append = [&](auto& levels) {
    auto [it, inserted] = levels.try_emplace(r.price, Level{});
    (void)inserted;
    auto& level = it->second;
    nodes_[idx].prev = level.tail;
    if (level.tail != npos) nodes_[level.tail].next = idx;
    else level.head = idx;
    level.tail = idx;
    level.total += remaining;
    ++level.order_count;
  };
  if (r.side == Side::Buy) append(bids_);
  else append(asks_);
  by_id_[r.order_id] = idx;
  ++active_count_;
}

void OrderBook::unlink(std::uint32_t idx) {
  auto& node = nodes_[idx];
  auto remove = [&](auto& levels) {
    auto it = levels.find(node.price);
    if (it == levels.end()) return;
    auto& level = it->second;
    if (node.prev != npos) nodes_[node.prev].next = node.next;
    else level.head = node.next;
    if (node.next != npos) nodes_[node.next].prev = node.prev;
    else level.tail = node.prev;
    level.total -= node.remaining;
    --level.order_count;
    if (level.head == npos) levels.erase(it);
  };
  if (node.side == Side::Buy) remove(bids_);
  else remove(asks_);
}

void OrderBook::remove_without_report(std::uint32_t idx) {
  const auto id = nodes_[idx].id;
  unlink(idx);
  by_id_.erase(id);
  release_node(idx);
  --active_count_;
}

bool OrderBook::cancel(const CancelRequest& r) {
  auto it = by_id_.find(r.order_id);
  if (it == by_id_.end()) {
    emit_report(r.order_id, ExecutionReport::Status::Rejected, 0, RejectReason::UnknownOrder);
    return false;
  }
  const auto idx = it->second;
  if (nodes_[idx].client_id != r.client_id) {
    emit_report(r.order_id, ExecutionReport::Status::Rejected, nodes_[idx].remaining,
                RejectReason::ClientMismatch);
    return false;
  }
  const auto remaining = nodes_[idx].remaining;
  remove_without_report(idx);
  emit_report(r.order_id, ExecutionReport::Status::Cancelled, remaining);
  return true;
}

bool OrderBook::replace(const ReplaceRequest& r) {
  auto it = by_id_.find(r.order_id);
  if (it == by_id_.end()) {
    emit_report(r.order_id, ExecutionReport::Status::Rejected, 0, RejectReason::UnknownOrder);
    return false;
  }
  const auto idx = it->second;
  auto& node = nodes_[idx];
  if (node.client_id != r.client_id) {
    emit_report(r.order_id, ExecutionReport::Status::Rejected, node.remaining,
                RejectReason::ClientMismatch);
    return false;
  }
  if (r.new_price <= 0 || r.new_quantity == 0) {
    emit_report(r.order_id, ExecutionReport::Status::Rejected, node.remaining,
                RejectReason::InvalidReplacement);
    return false;
  }

  if (r.new_price == node.price && r.new_quantity <= node.remaining) {
    const auto delta = node.remaining - r.new_quantity;
    node.remaining = r.new_quantity;
    auto update = [&](auto& levels) {
      auto level = levels.find(node.price);
      if (level != levels.end()) level->second.total -= delta;
    };
    if (node.side == Side::Buy) update(bids_);
    else update(asks_);
    emit_report(r.order_id, ExecutionReport::Status::Replaced, node.remaining);
    return true;
  }

  const Side side = node.side;
  remove_without_report(idx);
  emit_report(r.order_id, ExecutionReport::Status::Replaced, r.new_quantity);
  return submit(OrderRequest{r.order_id, r.client_id, side, OrderType::Limit, r.new_price,
                             r.new_quantity, symbol_});
}

Price OrderBook::best_bid() const noexcept { return bids_.empty() ? 0 : bids_.begin()->first; }
Price OrderBook::best_ask() const noexcept { return asks_.empty() ? 0 : asks_.begin()->first; }

Quantity OrderBook::quantity_at(Side side, Price price) const {
  if (side == Side::Buy) {
    const auto it = bids_.find(price);
    return it == bids_.end() ? 0 : it->second.total;
  }
  const auto it = asks_.find(price);
  return it == asks_.end() ? 0 : it->second.total;
}

std::vector<DepthLevel> OrderBook::depth(Side side, std::size_t levels) const {
  std::vector<DepthLevel> result;
  result.reserve(levels);
  auto collect = [&](const auto& source) {
    for (const auto& [price, level] : source) {
      if (result.size() == levels) break;
      result.push_back({price, level.total, level.order_count});
    }
  };
  if (side == Side::Buy) collect(bids_);
  else collect(asks_);
  return result;
}

std::vector<OrderRequest> OrderBook::resting_orders() const {
  std::vector<OrderRequest> result;
  result.reserve(active_count_);
  auto collect = [&](const auto& levels, Side side) {
    for (const auto& [price, level] : levels) {
      auto idx = level.head;
      while (idx != npos) {
        const auto& node = nodes_[idx];
        result.push_back({node.id, node.client_id, side, OrderType::Limit, price,
                          node.remaining, symbol_});
        idx = node.next;
      }
    }
  };
  collect(bids_, Side::Buy);
  collect(asks_, Side::Sell);
  return result;
}

void OrderBook::restore_resting_orders(const std::vector<OrderRequest>& orders, Sequence sequence) {
  reset();
  for (const auto& order : orders) {
    if (order.symbol != symbol_ || order.quantity == 0 || order.price <= 0 ||
        by_id_.contains(order.order_id)) {
      throw std::runtime_error("invalid snapshot order");
    }
    rest(order, order.quantity);
  }
  sequence_ = sequence;
}

void OrderBook::reset() {
  bids_.clear();
  asks_.clear();
  by_id_.clear();
  active_count_ = 0;
  sequence_ = 0;
  free_list_.clear();
  for (std::size_t i = nodes_.size(); i-- > 0;) {
    nodes_[i] = Node{};
    free_list_.push_back(static_cast<std::uint32_t>(i));
  }
}

std::uint64_t OrderBook::state_hash() const noexcept {
  std::uint64_t hash = 1469598103934665603ULL;
  auto mix = [&](std::uint64_t value) {
    hash ^= value;
    hash *= 1099511628211ULL;
  };
  mix(symbol_);
  auto collect = [&](const auto& levels, Side side) {
    mix(static_cast<std::uint64_t>(side));
    for (const auto& [price, level] : levels) {
      mix(static_cast<std::uint64_t>(price));
      auto idx = level.head;
      while (idx != npos) {
        const auto& node = nodes_[idx];
        mix(node.id);
        mix(node.client_id);
        mix(node.remaining);
        idx = node.next;
      }
    }
  };
  collect(bids_, Side::Buy);
  collect(asks_, Side::Sell);
  mix(active_count_);
  return hash;
}

template void OrderBook::match_against<OrderBook::Bids>(OrderBook::Bids&, const OrderRequest&,
                                                         Quantity&);
template void OrderBook::match_against<OrderBook::Asks>(OrderBook::Asks&, const OrderRequest&,
                                                         Quantity&);
}  // namespace minimatch
