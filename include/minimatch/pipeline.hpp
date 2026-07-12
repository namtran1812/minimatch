#pragma once
#include "minimatch/exchange.hpp"
#include "minimatch/spsc_ring.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

namespace minimatch {

struct PipelineCommand {
  enum class Kind : std::uint8_t { New, Cancel, Replace, Stop };
  Kind kind{Kind::New};
  OrderRequest order{};
  CancelRequest cancel{};
  ReplaceRequest replace{};
  std::uint64_t ingress_ns{};
};

struct PipelineMetrics {
  std::uint64_t submitted{};
  std::uint64_t processed{};
  std::uint64_t dropped{};
  std::uint64_t trades{};
  std::uint64_t reports{};
  std::uint64_t max_queue_depth{};
  std::vector<std::uint64_t> latency_ns;
};

class MatchingPipeline {
 public:
  explicit MatchingPipeline(std::size_t capacity = 300'000);
  ~MatchingPipeline();
  void start();
  void stop();
  bool submit(const PipelineCommand& command);
  [[nodiscard]] PipelineMetrics metrics() const;
  [[nodiscard]] std::uint64_t state_hash() const noexcept;
  [[nodiscard]] std::size_t active_orders() const noexcept;
 private:
  static std::uint64_t now_ns() noexcept;
  void run();
  Exchange exchange_;
  SpscRing<PipelineCommand, 1u << 16> queue_;
  std::atomic<bool> running_{false};
  std::thread worker_;
  std::atomic<std::uint64_t> submitted_{0}, processed_{0}, dropped_{0}, trades_{0}, reports_{0}, max_depth_{0};
  mutable std::mutex latency_mutex_;
  std::vector<std::uint64_t> latencies_;
};

}  // namespace minimatch
