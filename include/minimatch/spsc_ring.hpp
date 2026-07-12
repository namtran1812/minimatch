#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <optional>
#include <type_traits>

namespace minimatch {

template <typename T, std::size_t Capacity>
class SpscRing {
  static_assert(Capacity >= 2);
 public:
  bool try_push(const T& value) noexcept(std::is_nothrow_copy_assignable_v<T>) {
    const auto head = head_.load(std::memory_order_relaxed);
    const auto next = increment(head);
    if (next == tail_.load(std::memory_order_acquire)) return false;
    data_[head] = value;
    head_.store(next, std::memory_order_release);
    return true;
  }
  bool try_pop(T& value) noexcept(std::is_nothrow_copy_assignable_v<T>) {
    const auto tail = tail_.load(std::memory_order_relaxed);
    if (tail == head_.load(std::memory_order_acquire)) return false;
    value = data_[tail];
    tail_.store(increment(tail), std::memory_order_release);
    return true;
  }
  [[nodiscard]] bool empty() const noexcept {
    return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
  }
  [[nodiscard]] std::size_t approximate_size() const noexcept {
    const auto h = head_.load(std::memory_order_acquire);
    const auto t = tail_.load(std::memory_order_acquire);
    return h >= t ? h - t : Capacity - (t - h);
  }
 private:
  static constexpr std::size_t increment(std::size_t i) noexcept { return (i + 1) % Capacity; }
  alignas(64) std::atomic<std::size_t> head_{0};
  alignas(64) std::atomic<std::size_t> tail_{0};
  std::array<T, Capacity> data_{};
};

}  // namespace minimatch
