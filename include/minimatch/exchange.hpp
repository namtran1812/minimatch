#pragma once
#include "minimatch/order_book.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace minimatch {

class Exchange {
 public:
  explicit Exchange(std::size_t per_symbol_capacity = 300'000);

  bool submit(const OrderRequest& request);
  bool cancel(const CancelRequest& request);
  bool replace(const ReplaceRequest& request);

  [[nodiscard]] const OrderBook* find_book(SymbolId symbol) const noexcept;
  [[nodiscard]] OrderBook* find_book(SymbolId symbol) noexcept;
  [[nodiscard]] std::vector<SymbolId> symbols() const;
  [[nodiscard]] std::size_t active_orders() const noexcept;
  [[nodiscard]] std::uint64_t state_hash() const noexcept;

  void on_trade(OrderBook::TradeHandler handler);
  void on_report(OrderBook::ReportHandler handler);

  void write_snapshot(const std::string& path) const;
  void load_snapshot(const std::string& path);

 private:
  OrderBook& book_for(SymbolId symbol);
  void attach_handlers(OrderBook& book);

  std::size_t per_symbol_capacity_;
  std::unordered_map<SymbolId, std::unique_ptr<OrderBook>> books_;
  OrderBook::TradeHandler trade_handler_;
  OrderBook::ReportHandler report_handler_;
};

}  // namespace minimatch
