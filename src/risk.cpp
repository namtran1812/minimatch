#include "minimatch/risk.hpp"
#include <algorithm>
#include <cstdlib>
#include <limits>

namespace minimatch {
RiskEngine::RiskEngine(RiskLimits limits) : limits_(limits) {}

std::uint64_t RiskEngine::key(ClientId client, SymbolId symbol) noexcept {
  return (static_cast<std::uint64_t>(client) << 32U) | symbol;
}

bool RiskEngine::allow(const OrderRequest& r) const noexcept {
  if (r.quantity == 0 || r.quantity > limits_.max_order_quantity) return false;
  const auto it = states_.find(key(r.client_id, r.symbol));
  const RiskState empty{};
  const auto& s = it == states_.end() ? empty : it->second;
  if (s.killed || s.realized_pnl <= -limits_.max_daily_loss) return false;
  const std::int64_t signed_qty = r.side == Side::Buy ? r.quantity : -static_cast<std::int64_t>(r.quantity);
  if (std::llabs(s.position + signed_qty) > limits_.max_abs_position) return false;
  if (r.type != OrderType::Market) {
    const auto add = static_cast<std::int64_t>(r.quantity) * r.price;
    if (add < 0 || s.open_notional > limits_.max_open_notional - add) return false;
  }
  return true;
}

void RiskEngine::on_order_accepted(const OrderRequest& r) noexcept {
  auto& s = states_[key(r.client_id, r.symbol)];
  if (r.type != OrderType::Market) {
    s.open_notional += static_cast<std::int64_t>(r.quantity) * r.price;
  }
}

void RiskEngine::on_order_closed(ClientId client, SymbolId symbol, Side, Price price,
                                 Quantity quantity) noexcept {
  auto& s = states_[key(client, symbol)];
  const auto reduce = static_cast<std::int64_t>(quantity) * price;
  s.open_notional = std::max<std::int64_t>(0, s.open_notional - reduce);
}

void RiskEngine::on_fill(ClientId client, SymbolId symbol, Side side, Price price,
                         Quantity quantity) noexcept {
  auto& s = states_[key(client, symbol)];
  const auto q = static_cast<std::int64_t>(quantity);
  s.position += side == Side::Buy ? q : -q;
  const auto reduce = q * price;
  s.open_notional = std::max<std::int64_t>(0, s.open_notional - reduce);
}

void RiskEngine::set_realized_pnl(ClientId client, SymbolId symbol, std::int64_t pnl) noexcept {
  auto& s = states_[key(client, symbol)];
  s.realized_pnl = pnl;
  if (pnl <= -limits_.max_daily_loss) s.killed = true;
}

void RiskEngine::kill(ClientId client, SymbolId symbol) noexcept {
  states_[key(client, symbol)].killed = true;
}
void RiskEngine::reset(ClientId client, SymbolId symbol) noexcept {
  states_[key(client, symbol)] = RiskState{};
}
RiskState RiskEngine::state(ClientId client, SymbolId symbol) const noexcept {
  const auto it = states_.find(key(client, symbol));
  return it == states_.end() ? RiskState{} : it->second;
}
}  // namespace minimatch
