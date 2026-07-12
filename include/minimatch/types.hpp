#pragma once
#include <cstdint>
#include <string_view>

namespace minimatch {
using OrderId = std::uint64_t;
using Sequence = std::uint64_t;
using Price = std::int64_t;
using Quantity = std::uint32_t;
using ClientId = std::uint32_t;
using SymbolId = std::uint32_t;

constexpr Price MARKET_PRICE = 0;
constexpr SymbolId DEFAULT_SYMBOL = 1;

enum class Side : std::uint8_t { Buy = 0, Sell = 1 };
enum class OrderType : std::uint8_t {
  Limit = 0,
  Market = 1,
  IOC = 2,
  FOK = 3,
  PostOnly = 4
};

enum class RejectReason : std::uint8_t {
  None = 0,
  DuplicateOrderId,
  UnknownOrder,
  InvalidQuantity,
  InvalidPrice,
  CapacityExceeded,
  WouldTrade,
  InsufficientLiquidity,
  ClientMismatch,
  InvalidReplacement
};

struct OrderRequest {
  OrderId order_id{};
  ClientId client_id{};
  Side side{};
  OrderType type{};
  Price price{};
  Quantity quantity{};
  SymbolId symbol{DEFAULT_SYMBOL};
};

struct CancelRequest {
  OrderId order_id{};
  ClientId client_id{};
  SymbolId symbol{DEFAULT_SYMBOL};
};

struct ReplaceRequest {
  OrderId order_id{};
  ClientId client_id{};
  Price new_price{};
  Quantity new_quantity{};
  SymbolId symbol{DEFAULT_SYMBOL};
};

struct Trade {
  Sequence sequence{};
  OrderId maker_order_id{};
  OrderId taker_order_id{};
  Price price{};
  Quantity quantity{};
  Side taker_side{};
  SymbolId symbol{DEFAULT_SYMBOL};
};

struct ExecutionReport {
  enum class Status : std::uint8_t {
    Accepted,
    PartiallyFilled,
    Filled,
    Cancelled,
    Replaced,
    Rejected,
    Expired
  };
  Sequence sequence{};
  OrderId order_id{};
  Status status{};
  Quantity remaining{};
  RejectReason reject_reason{};
  SymbolId symbol{DEFAULT_SYMBOL};
};

struct DepthLevel {
  Price price{};
  Quantity quantity{};
  std::uint32_t order_count{};
};

constexpr std::string_view to_string(Side x) { return x == Side::Buy ? "BUY" : "SELL"; }
}  // namespace minimatch
