#pragma once
#include "minimatch/types.hpp"
#include <cstddef>
#include <functional>
#include <limits>
#include <map>
#include <unordered_map>
#include <vector>

namespace minimatch {

class OrderBook {
 public:
  using TradeHandler = std::function<void(const Trade&)>;
  using ReportHandler = std::function<void(const ExecutionReport&)>;

  explicit OrderBook(std::size_t capacity = 1'100'000, SymbolId symbol = DEFAULT_SYMBOL);
  bool submit(const OrderRequest& request);
  bool cancel(const CancelRequest& request);
  bool replace(const ReplaceRequest& request);
  void reset();

  [[nodiscard]] SymbolId symbol() const noexcept { return symbol_; }
  [[nodiscard]] std::size_t active_orders() const noexcept { return active_count_; }
  [[nodiscard]] Sequence sequence() const noexcept { return sequence_; }
  [[nodiscard]] Price best_bid() const noexcept;
  [[nodiscard]] Price best_ask() const noexcept;
  [[nodiscard]] Quantity quantity_at(Side side, Price price) const;
  [[nodiscard]] std::uint64_t state_hash() const noexcept;
  [[nodiscard]] std::vector<DepthLevel> depth(Side side, std::size_t levels) const;
  [[nodiscard]] std::vector<OrderRequest> resting_orders() const;
  void restore_resting_orders(const std::vector<OrderRequest>& orders, Sequence sequence);

  void on_trade(TradeHandler h) { trade_handler_ = std::move(h); }
  void on_report(ReportHandler h) { report_handler_ = std::move(h); }

  void set_stp_policy(
      SelfTradePreventionPolicy policy
  ) noexcept {
    stp_policy_ = policy;
  }

  [[nodiscard]]
  SelfTradePreventionPolicy stp_policy() const noexcept {
    return stp_policy_;
  }

 private:
  static constexpr std::uint32_t npos = std::numeric_limits<std::uint32_t>::max();
  struct Node {
    OrderId id{};
    ClientId client_id{};
    Price price{};
    Quantity remaining{};
    std::uint32_t prev{npos};
    std::uint32_t next{npos};
    Side side{};
    bool active{false};
  };
  struct Level {
    std::uint32_t head{npos};
    std::uint32_t tail{npos};
    Quantity total{};
    std::uint32_t order_count{};
  };

  using Bids = std::map<Price, Level, std::greater<Price>>;
  using Asks = std::map<Price, Level, std::less<Price>>;

  bool validate(const OrderRequest& request);
  std::uint32_t allocate_node();
  void release_node(std::uint32_t idx);
  void rest(const OrderRequest& request, Quantity remaining);
  void unlink(std::uint32_t idx);
  void remove_without_report(std::uint32_t idx);
  void emit_report(OrderId id, ExecutionReport::Status status, Quantity remaining,
                   RejectReason reason = RejectReason::None);
  void emit_trade(const Node& maker, const OrderRequest& taker, Price px, Quantity qty);
  bool crosses(const OrderRequest& taker, Price maker_price) const noexcept;
  [[nodiscard]] Quantity available_to_match(const OrderRequest& request) const noexcept;

  enum class MatchOutcome {
    Completed,
    TakerCancelled
  };

  template <typename Levels>
  MatchOutcome match_against(
      Levels& levels,
      const OrderRequest& taker,
      Quantity& remaining
  );

  SymbolId symbol_;
  std::vector<Node> nodes_;
  std::vector<std::uint32_t> free_list_;
  std::unordered_map<OrderId, std::uint32_t> by_id_;
  Bids bids_;
  Asks asks_;
  std::size_t active_count_{0};
  Sequence sequence_{0};
  TradeHandler trade_handler_;
  ReportHandler report_handler_;
  SelfTradePreventionPolicy stp_policy_{
      SelfTradePreventionPolicy::None
  };
};

}  // namespace minimatch
