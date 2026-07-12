#include "minimatch/exchange.hpp"
#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace minimatch {
namespace {
#pragma pack(push, 1)
struct SnapshotHeader {
  std::uint32_t magic{0x4D4D5350};
  std::uint16_t version{1};
  std::uint16_t reserved{};
  std::uint32_t symbol_count{};
};
struct SnapshotBookHeader {
  SymbolId symbol{};
  Sequence sequence{};
  std::uint64_t order_count{};
  std::uint64_t state_hash{};
};
#pragma pack(pop)
}  // namespace

Exchange::Exchange(std::size_t per_symbol_capacity) : per_symbol_capacity_(per_symbol_capacity) {}

OrderBook& Exchange::book_for(SymbolId symbol) {
  auto it = books_.find(symbol);
  if (it != books_.end()) return *it->second;
  auto book = std::make_unique<OrderBook>(per_symbol_capacity_, symbol);
  attach_handlers(*book);
  auto [inserted, ok] = books_.emplace(symbol, std::move(book));
  (void)ok;
  return *inserted->second;
}

void Exchange::attach_handlers(OrderBook& book) {
  book.on_trade([this](const Trade& trade) {
    if (trade_handler_) trade_handler_(trade);
  });
  book.on_report([this](const ExecutionReport& report) {
    if (report_handler_) report_handler_(report);
  });
}

bool Exchange::submit(const OrderRequest& request) { return book_for(request.symbol).submit(request); }
bool Exchange::cancel(const CancelRequest& request) { return book_for(request.symbol).cancel(request); }
bool Exchange::replace(const ReplaceRequest& request) { return book_for(request.symbol).replace(request); }

const OrderBook* Exchange::find_book(SymbolId symbol) const noexcept {
  const auto it = books_.find(symbol);
  return it == books_.end() ? nullptr : it->second.get();
}
OrderBook* Exchange::find_book(SymbolId symbol) noexcept {
  const auto it = books_.find(symbol);
  return it == books_.end() ? nullptr : it->second.get();
}

std::vector<SymbolId> Exchange::symbols() const {
  std::vector<SymbolId> result;
  result.reserve(books_.size());
  for (const auto& [symbol, _] : books_) result.push_back(symbol);
  std::sort(result.begin(), result.end());
  return result;
}

std::size_t Exchange::active_orders() const noexcept {
  std::size_t total = 0;
  for (const auto& [_, book] : books_) total += book->active_orders();
  return total;
}

std::uint64_t Exchange::state_hash() const noexcept {
  std::uint64_t hash = 1469598103934665603ULL;
  auto mix = [&](std::uint64_t value) {
    hash ^= value;
    hash *= 1099511628211ULL;
  };
  for (const auto symbol : symbols()) {
    mix(symbol);
    mix(books_.at(symbol)->state_hash());
  }
  return hash;
}

void Exchange::on_trade(OrderBook::TradeHandler handler) {
  trade_handler_ = std::move(handler);
  for (auto& [_, book] : books_) attach_handlers(*book);
}
void Exchange::on_report(OrderBook::ReportHandler handler) {
  report_handler_ = std::move(handler);
  for (auto& [_, book] : books_) attach_handlers(*book);
}

void Exchange::write_snapshot(const std::string& path) const {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) throw std::runtime_error("cannot open snapshot for writing");
  const auto ordered_symbols = symbols();
  SnapshotHeader header;
  header.symbol_count = static_cast<std::uint32_t>(ordered_symbols.size());
  out.write(reinterpret_cast<const char*>(&header), sizeof(header));
  for (const auto symbol : ordered_symbols) {
    const auto& book = *books_.at(symbol);
    const auto orders = book.resting_orders();
    SnapshotBookHeader book_header{symbol, book.sequence(), orders.size(), book.state_hash()};
    out.write(reinterpret_cast<const char*>(&book_header), sizeof(book_header));
    out.write(reinterpret_cast<const char*>(orders.data()),
              static_cast<std::streamsize>(orders.size() * sizeof(OrderRequest)));
  }
  if (!out) throw std::runtime_error("snapshot write failed");
}

void Exchange::load_snapshot(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("cannot open snapshot for reading");
  SnapshotHeader header{};
  in.read(reinterpret_cast<char*>(&header), sizeof(header));
  if (!in || header.magic != 0x4D4D5350 || header.version != 1) {
    throw std::runtime_error("bad snapshot header");
  }
  books_.clear();
  for (std::uint32_t i = 0; i < header.symbol_count; ++i) {
    SnapshotBookHeader book_header{};
    in.read(reinterpret_cast<char*>(&book_header), sizeof(book_header));
    if (!in || book_header.order_count > per_symbol_capacity_) {
      throw std::runtime_error("invalid snapshot book");
    }
    std::vector<OrderRequest> orders(static_cast<std::size_t>(book_header.order_count));
    in.read(reinterpret_cast<char*>(orders.data()),
            static_cast<std::streamsize>(orders.size() * sizeof(OrderRequest)));
    if (!in) throw std::runtime_error("truncated snapshot");
    auto book = std::make_unique<OrderBook>(per_symbol_capacity_, book_header.symbol);
    attach_handlers(*book);
    book->restore_resting_orders(orders, book_header.sequence);
    if (book->state_hash() != book_header.state_hash) {
      throw std::runtime_error("snapshot state hash mismatch");
    }
    books_.emplace(book_header.symbol, std::move(book));
  }
}

}  // namespace minimatch
