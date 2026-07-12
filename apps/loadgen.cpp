#include "minimatch/exchange.hpp"
#include <chrono>
#include <iostream>
#include <random>

using namespace minimatch;
int main(int argc, char** argv) {
  const std::uint64_t count = argc > 1 ? std::stoull(argv[1]) : 1'000'000;
  const std::uint32_t symbol_count = argc > 2 ? static_cast<std::uint32_t>(std::stoul(argv[2])) : 8;
  Exchange exchange(static_cast<std::size_t>(count / symbol_count + 100'000));
  std::mt19937_64 random(42);
  std::uniform_int_distribution<int> side_dist(0, 1), offset(-50, 50), qty(1, 100), action(0, 99);
  const auto start = std::chrono::steady_clock::now();
  for (std::uint64_t id = 1; id <= count; ++id) {
    const SymbolId symbol = static_cast<SymbolId>(1 + (id % symbol_count));
    const Side side = side_dist(random) ? Side::Buy : Side::Sell;
    const Price price = 10'000 + offset(random) + static_cast<Price>(symbol) * 100;
    const int roll = action(random);
    OrderType type = OrderType::Limit;
    if (roll < 4) type = OrderType::IOC;
    else if (roll < 6) type = OrderType::FOK;
    else if (roll < 10) type = OrderType::PostOnly;
    exchange.submit({id, static_cast<ClientId>(1 + id % 64), side, type, price,
                     static_cast<Quantity>(qty(random)), symbol});
  }
  const auto end = std::chrono::steady_clock::now();
  const double seconds = std::chrono::duration<double>(end - start).count();
  std::cout << "orders=" << count << " symbols=" << symbol_count << " seconds=" << seconds
            << " orders_per_sec=" << static_cast<double>(count) / seconds
            << " active=" << exchange.active_orders() << " state_hash=" << exchange.state_hash()
            << '\n';
}
