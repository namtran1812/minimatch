#include "minimatch/protocol.hpp"
#include <cstring>

namespace minimatch {
template <class Payload>
auto encode_payload(MessageType type, const Payload& payload) {
  std::array<std::byte, sizeof(WireHeader) + sizeof(Payload)> bytes{};
  WireHeader header;
  header.type = type;
  header.length = sizeof(Payload);
  std::memcpy(bytes.data(), &header, sizeof(header));
  std::memcpy(bytes.data() + sizeof(header), &payload, sizeof(payload));
  return bytes;
}

std::array<std::byte, sizeof(WireHeader) + sizeof(WireOrder)> encode(const OrderRequest& r) {
  return encode_payload(MessageType::NewOrder,
                        WireOrder{r.order_id, r.client_id, r.symbol,
                                  static_cast<std::uint8_t>(r.side),
                                  static_cast<std::uint8_t>(r.type), r.price, r.quantity});
}
std::array<std::byte, sizeof(WireHeader) + sizeof(WireCancel)> encode(const CancelRequest& r) {
  return encode_payload(MessageType::Cancel, WireCancel{r.order_id, r.client_id, r.symbol});
}
std::array<std::byte, sizeof(WireHeader) + sizeof(WireReplace)> encode(const ReplaceRequest& r) {
  return encode_payload(MessageType::Replace,
                        WireReplace{r.order_id, r.client_id, r.symbol, r.new_price,
                                    r.new_quantity});
}
OrderRequest decode_order(const WireOrder& w) {
  return {w.order_id, w.client_id, static_cast<Side>(w.side), static_cast<OrderType>(w.type),
          w.price, w.quantity, w.symbol};
}
CancelRequest decode_cancel(const WireCancel& w) { return {w.order_id, w.client_id, w.symbol}; }
ReplaceRequest decode_replace(const WireReplace& w) {
  return {w.order_id, w.client_id, w.new_price, w.new_quantity, w.symbol};
}
}  // namespace minimatch
