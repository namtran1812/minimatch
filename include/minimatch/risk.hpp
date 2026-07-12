#pragma once
#include "minimatch/types.hpp"
#include <cstdint>
#include <unordered_map>

namespace minimatch {

struct RiskLimits {
  Quantity max_order_quantity{5'000};
  std::int64_t max_abs_position{25'000};
  std::int64_t max_open_notional{500'000'000};
  std::int64_t max_daily_loss{5'000'000};
};

struct RiskState {
  std::int64_t position{};
  std::int64_t open_notional{};
  std::int64_t realized_pnl{};
  bool killed{};
};

class RiskEngine {
 public:
  explicit RiskEngine(RiskLimits limits = {});
  [[nodiscard]] bool allow(const OrderRequest& request) const noexcept;
  void on_order_accepted(const OrderRequest& request) noexcept;
  void on_order_closed(ClientId client, SymbolId symbol, Side side, Price price,
                       Quantity quantity) noexcept;
  void on_fill(ClientId client, SymbolId symbol, Side side, Price price,
               Quantity quantity) noexcept;
  void set_realized_pnl(ClientId client, SymbolId symbol, std::int64_t pnl) noexcept;
  void kill(ClientId client, SymbolId symbol) noexcept;
  void reset(ClientId client, SymbolId symbol) noexcept;
  [[nodiscard]] RiskState state(ClientId client, SymbolId symbol) const noexcept;

 private:
  static std::uint64_t key(ClientId client, SymbolId symbol) noexcept;
  RiskLimits limits_;
  std::unordered_map<std::uint64_t, RiskState> states_;
};

}  // namespace minimatch
