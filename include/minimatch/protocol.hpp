#pragma once
#include "minimatch/types.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace minimatch {
enum class MessageType : std::uint8_t { NewOrder = 1, Cancel = 2, Replace = 3 };
#pragma pack(push, 1)
struct WireHeader {
  std::uint32_t magic{0x4D4D4154};
  std::uint16_t version{2};
  MessageType type{};
  std::uint8_t reserved{};
  std::uint32_t length{};
};
struct WireOrder {
  std::uint64_t order_id;
  std::uint32_t client_id;
  std::uint32_t symbol;
  std::uint8_t side;
  std::uint8_t type;
  std::int64_t price;
  std::uint32_t quantity;
};
struct WireCancel {
  std::uint64_t order_id;
  std::uint32_t client_id;
  std::uint32_t symbol;
};
struct WireReplace {
  std::uint64_t order_id;
  std::uint32_t client_id;
  std::uint32_t symbol;
  std::int64_t new_price;
  std::uint32_t new_quantity;
};
#pragma pack(pop)

std::array<std::byte, sizeof(WireHeader) + sizeof(WireOrder)> encode(const OrderRequest&);
std::array<std::byte, sizeof(WireHeader) + sizeof(WireCancel)> encode(const CancelRequest&);
std::array<std::byte, sizeof(WireHeader) + sizeof(WireReplace)> encode(const ReplaceRequest&);
OrderRequest decode_order(const WireOrder&);
CancelRequest decode_cancel(const WireCancel&);
ReplaceRequest decode_replace(const WireReplace&);
}  // namespace minimatch
