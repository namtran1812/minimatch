#include "minimatch/latency.hpp"
#include "minimatch/order_book.hpp"
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

using namespace minimatch;
int main(int argc, char** argv) {
  const std::size_t count = argc > 1 ? static_cast<std::size_t>(std::stoull(argv[1])) : 1'000'000;
  std::vector<OrderRequest> orders;
  orders.reserve(count);
  std::mt19937_64 random(42);
  std::uniform_int_distribution<int> side(0, 1), offset(-50, 50), qty(1, 100);
  for (std::size_t i = 0; i < count; ++i) {
    orders.push_back({i + 1, 1, side(random) ? Side::Buy : Side::Sell, OrderType::Limit,
                      10'000 + offset(random), static_cast<Quantity>(qty(random)), 1});
  }

  OrderBook warmup(100'000, 1);
  for (std::size_t i = 0; i < std::min<std::size_t>(50'000, orders.size()); ++i) warmup.submit(orders[i]);

  OrderBook book(count + 1024, 1);
  LatencyRecorder latency(count);
  const auto wall_start = std::chrono::steady_clock::now();
  for (const auto& order : orders) {
    const auto start = std::chrono::steady_clock::now();
    book.submit(order);
    const auto end = std::chrono::steady_clock::now();
    latency.record(static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()));
  }
  const auto wall_end = std::chrono::steady_clock::now();
  latency.sort();
  const double seconds = std::chrono::duration<double>(wall_end - wall_start).count();
  std::cout << "orders=" << count << " throughput=" << static_cast<double>(count) / seconds
            << " p50_ns=" << latency.percentile(0.50)
            << " p95_ns=" << latency.percentile(0.95)
            << " p99_ns=" << latency.percentile(0.99)
            << " p999_ns=" << latency.percentile(0.999)
            << " max_ns=" << latency.max()
            << " state_hash=" << book.state_hash() << '\n';
}
