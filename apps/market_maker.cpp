#include "minimatch/exchange.hpp"
#include "minimatch/risk.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <unordered_map>

using namespace minimatch;

int main(int argc, char** argv) {
  const std::size_t steps = argc > 1 ? std::stoull(argv[1]) : 100'000;
  constexpr ClientId maker_client = 7;
  constexpr SymbolId symbol = 1;
  Exchange exchange(400'000);
  RiskEngine risk(RiskLimits{2'000, 10'000, 250'000'000, 2'000'000});
  std::unordered_map<OrderId, OrderRequest> live;
  std::int64_t cash = 0;
  std::int64_t position = 0;
  std::uint64_t fills = 0;
  std::uint64_t rejected_by_risk = 0;
  double fundamental = 10'000.0;
  OrderId next_id = 1;

  exchange.on_trade([&](const Trade& t) {
    auto apply = [&](OrderId id, bool maker) {
      auto it = live.find(id);
      if (it == live.end()) return;
      const auto& o = it->second;
      const auto signed_qty = static_cast<std::int64_t>(t.quantity);
      if (o.client_id == maker_client) {
        if (o.side == Side::Buy) { position += signed_qty; cash -= t.price * signed_qty; }
        else { position -= signed_qty; cash += t.price * signed_qty; }
        risk.on_fill(o.client_id, o.symbol, o.side, t.price, t.quantity);
        ++fills;
      }
      (void)maker;
    };
    apply(t.maker_order_id, true);
    apply(t.taker_order_id, false);
  });

  std::mt19937_64 rng(20260712);
  std::normal_distribution<double> move(0.0, 1.6);
  std::uniform_real_distribution<double> uniform(0.0, 1.0);
  std::uniform_int_distribution<int> qty_dist(1, 40);

  auto submit = [&](OrderRequest o) {
    if (!risk.allow(o)) { ++rejected_by_risk; return false; }
    risk.on_order_accepted(o);
    live[o.order_id] = o;
    return exchange.submit(o);
  };

  for (std::size_t step = 0; step < steps; ++step) {
    fundamental = std::max(100.0, fundamental + move(rng));
    const Price mid = static_cast<Price>(std::llround(fundamental));

    if (step % 10 == 0) {
      const auto inventory_skew = static_cast<Price>(position / 300);
      const Price half_spread = 3 + static_cast<Price>(std::abs(position) / 2'000);
      const Price bid = mid - half_spread - inventory_skew;
      const Price ask = mid + half_spread - inventory_skew;
      submit({next_id++, maker_client, Side::Buy, OrderType::PostOnly, bid, 50, symbol});
      submit({next_id++, maker_client, Side::Sell, OrderType::PostOnly, ask, 50, symbol});
    }

    const bool buy = uniform(rng) < 0.5;
    const Quantity q = static_cast<Quantity>(qty_dist(rng));
    const ClientId client = static_cast<ClientId>(100 + step % 50);
    OrderRequest flow{next_id++, client, buy ? Side::Buy : Side::Sell,
                      OrderType::IOC, buy ? mid + 8 : mid - 8, q, symbol};
    live[flow.order_id] = flow;
    exchange.submit(flow);

    const std::int64_t mtm = cash + position * mid;
    risk.set_realized_pnl(maker_client, symbol, mtm);
    if (risk.state(maker_client, symbol).killed) break;
  }

  const Price final_mid = static_cast<Price>(std::llround(fundamental));
  const std::int64_t pnl = cash + position * final_mid;
  const auto* book = exchange.find_book(symbol);
  std::cout << "steps=" << steps << '\n'
            << "fills=" << fills << '\n'
            << "position=" << position << '\n'
            << "cash_ticks=" << cash << '\n'
            << "mark_to_market_pnl_ticks=" << pnl << '\n'
            << "risk_rejections=" << rejected_by_risk << '\n'
            << "killed=" << risk.state(maker_client, symbol).killed << '\n'
            << "best_bid=" << (book ? book->best_bid() : 0) << '\n'
            << "best_ask=" << (book ? book->best_ask() : 0) << '\n'
            << "active_orders=" << exchange.active_orders() << '\n'
            << "state_hash=" << exchange.state_hash() << '\n';
}
