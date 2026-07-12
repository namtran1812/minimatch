#include "minimatch/pipeline.hpp"
#include <algorithm>
#include <mutex>

namespace minimatch {
MatchingPipeline::MatchingPipeline(std::size_t capacity) : exchange_(capacity) {
  exchange_.on_trade([this](const Trade&) { trades_.fetch_add(1, std::memory_order_relaxed); });
  exchange_.on_report([this](const ExecutionReport&) { reports_.fetch_add(1, std::memory_order_relaxed); });
}
MatchingPipeline::~MatchingPipeline() { stop(); }
std::uint64_t MatchingPipeline::now_ns() noexcept {
  return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count());
}
void MatchingPipeline::start() {
  bool expected = false;
  if (!running_.compare_exchange_strong(expected, true)) return;
  worker_ = std::thread([this] { run(); });
}
void MatchingPipeline::stop() {
  if (!running_.exchange(false)) return;
  PipelineCommand stop_cmd; stop_cmd.kind = PipelineCommand::Kind::Stop;
  while (!queue_.try_push(stop_cmd)) std::this_thread::yield();
  if (worker_.joinable()) worker_.join();
}
bool MatchingPipeline::submit(const PipelineCommand& input) {
  PipelineCommand c = input;
  if (c.ingress_ns == 0) c.ingress_ns = now_ns();
  submitted_.fetch_add(1, std::memory_order_relaxed);
  if (!queue_.try_push(c)) { dropped_.fetch_add(1, std::memory_order_relaxed); return false; }
  const auto depth = queue_.approximate_size();
  auto current = max_depth_.load(std::memory_order_relaxed);
  while (depth > current && !max_depth_.compare_exchange_weak(current, depth)) {}
  return true;
}
void MatchingPipeline::run() {
  PipelineCommand c;
  while (true) {
    if (!queue_.try_pop(c)) { std::this_thread::yield(); continue; }
    if (c.kind == PipelineCommand::Kind::Stop) break;
    switch (c.kind) {
      case PipelineCommand::Kind::New: exchange_.submit(c.order); break;
      case PipelineCommand::Kind::Cancel: exchange_.cancel(c.cancel); break;
      case PipelineCommand::Kind::Replace: exchange_.replace(c.replace); break;
      case PipelineCommand::Kind::Stop: break;
    }
    processed_.fetch_add(1, std::memory_order_relaxed);
    const auto latency = now_ns() - c.ingress_ns;
    std::lock_guard lock(latency_mutex_);
    latencies_.push_back(latency);
  }
}
PipelineMetrics MatchingPipeline::metrics() const {
  PipelineMetrics m{submitted_.load(), processed_.load(), dropped_.load(), trades_.load(), reports_.load(), max_depth_.load(), {}};
  std::lock_guard lock(latency_mutex_); m.latency_ns = latencies_; return m;
}
std::uint64_t MatchingPipeline::state_hash() const noexcept { return exchange_.state_hash(); }
std::size_t MatchingPipeline::active_orders() const noexcept { return exchange_.active_orders(); }
}  // namespace minimatch
