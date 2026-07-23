#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

namespace minimatch {

enum class ItchMessageType : char {
  AddOrder = 'A'
};


constexpr bool is_valid_itch_price(double price) noexcept {
  return price >= 0.0;
}

constexpr std::uint32_t encode_itch_price(double price) noexcept {
  return static_cast<std::uint32_t>(price * 10000.0 + 0.5);
}

constexpr double decode_itch_price(std::uint32_t price) noexcept {
  return static_cast<double>(price) / 10000.0;
}

constexpr bool is_valid_itch_side(char side) noexcept {
  return side == 'B' || side == 'S';
}

inline void write_itch_stock(char (&stock)[8], std::string_view symbol) {
  std::memset(stock, ' ', sizeof(stock));
  const std::size_t n = std::min(symbol.size(), sizeof(stock));
  std::memcpy(stock, symbol.data(), n);
}

inline std::string read_itch_stock(const char (&stock)[8]) {
  std::string s(stock, stock + 8);
  while (!s.empty() && s.back() == ' ')
    s.pop_back();
  return s;
}

#pragma pack(push, 1)

struct ItchMessageHeader {
  std::uint16_t stock_locate{};
  std::uint16_t tracking_number{};
  std::uint64_t timestamp_ns{};
};

struct ItchAddOrder {
  ItchMessageType message_type{ItchMessageType::AddOrder};
  ItchMessageHeader header{};
  std::uint64_t order_reference_number{};
  char side{'B'};
  std::uint32_t shares{};
  char stock[8]{};
  std::uint32_t price{};
};

#pragma pack(pop)

}  // namespace minimatch
