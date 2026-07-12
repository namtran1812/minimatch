#include "minimatch/pipeline.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <thread>

using namespace minimatch;
static std::uint64_t percentile(std::vector<std::uint64_t> v, double p) {
  if (v.empty()) return 0; std::sort(v.begin(), v.end());
  return v[static_cast<std::size_t>(p * static_cast<double>(v.size() - 1))];
}
int main(int argc, char** argv) {
  const std::size_t steps = argc > 1 ? std::stoull(argv[1]) : 200000;
  MatchingPipeline pipe; pipe.start();
  std::mt19937_64 rng(42); std::uniform_int_distribution<int> jitter(-5, 5), qty(1, 50), agent(0, 4);
  Price mid = 10000; OrderId id = 1;
  for (std::size_t i = 0; i < steps; ++i) {
    if (i % 50 == 0) mid += jitter(rng) > 1 ? 1 : (jitter(rng) < -1 ? -1 : 0);
    const int a = agent(rng); Side side = (rng() & 1) ? Side::Buy : Side::Sell;
    Price px = mid + (side == Side::Buy ? -1 : 1);
    OrderType type = OrderType::Limit;
    if (a == 0) { px = mid + (side == Side::Buy ? -2 : 2); }             // maker
    else if (a == 1) { type = OrderType::IOC; px = mid + (side == Side::Buy ? 2 : -2); } // taker
    else if (a == 2 && i % 20 == 0) { side = mid % 2 ? Side::Buy : Side::Sell; type = OrderType::Market; px = 0; } // momentum
    else if (a == 3) { side = (mid > 10000) ? Side::Sell : Side::Buy; px = mid; } // mean reversion
    PipelineCommand c; c.kind = PipelineCommand::Kind::New;
    c.order = {id++, static_cast<ClientId>(100 + a), side, type, px, static_cast<Quantity>(qty(rng)), 1};
    while (!pipe.submit(c)) std::this_thread::yield();
  }
  while (pipe.metrics().processed < steps) std::this_thread::yield();
  pipe.stop(); const auto m = pipe.metrics();
  std::cout << "steps=" << steps << " processed=" << m.processed << " dropped=" << m.dropped
            << " trades=" << m.trades << " reports=" << m.reports
            << " active=" << pipe.active_orders() << " max_queue_depth=" << m.max_queue_depth
            << " p50_ns=" << percentile(m.latency_ns, .50) << " p99_ns=" << percentile(m.latency_ns, .99)
            << " p999_ns=" << percentile(m.latency_ns, .999) << " state_hash=" << pipe.state_hash() << '\n';
}
