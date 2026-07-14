#include "minimatch/exchange.hpp"
#include <array>
#include <iomanip>
#include "minimatch/router.hpp"
#include "minimatch/execution_engine.hpp"
#include "minimatch/execution_store.hpp"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <random>
#include <vector>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <cstdlib>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = asio::ip::tcp;
using namespace minimatch;

namespace {

std::string json_escape(std::string_view value) {
  std::string out;
  out.reserve(value.size() + 8);
  for (const char ch : value) {
    switch (ch) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out.push_back(ch); break;
    }
  }
  return out;
}

std::string url_decode(std::string_view input) {
  std::string out;
  out.reserve(input.size());
  for (std::size_t i = 0; i < input.size(); ++i) {
    if (input[i] == '+' ) {
      out.push_back(' ');
    } else if (input[i] == '%' && i + 2 < input.size()) {
      const auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + c - 'a';
        if (c >= 'A' && c <= 'F') return 10 + c - 'A';
        return -1;
      };
      const int hi = hex(input[i + 1]);
      const int lo = hex(input[i + 2]);
      if (hi >= 0 && lo >= 0) {
        out.push_back(static_cast<char>((hi << 4) | lo));
        i += 2;
      } else {
        out.push_back(input[i]);
      }
    } else {
      out.push_back(input[i]);
    }
  }
  return out;
}

std::map<std::string, std::string> parse_form(std::string_view body) {
  std::map<std::string, std::string> fields;
  std::size_t start = 0;
  while (start <= body.size()) {
    const std::size_t amp = body.find('&', start);
    const std::string_view pair = body.substr(start, amp == std::string_view::npos ? body.size() - start : amp - start);
    const std::size_t eq = pair.find('=');
    if (eq == std::string_view::npos) {
      fields[url_decode(pair)] = "";
    } else {
      fields[url_decode(pair.substr(0, eq))] = url_decode(pair.substr(eq + 1));
    }
    if (amp == std::string_view::npos) break;
    start = amp + 1;
  }
  return fields;
}

std::map<std::string, std::string> parse_query(std::string_view target) {
  const auto pos = target.find('?');
  if (pos == std::string_view::npos) return {};
  return parse_form(target.substr(pos + 1));
}

std::string safe_token(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  value.erase(std::remove_if(value.begin(), value.end(), [](unsigned char c) {
    return !(std::isalnum(c) || c == '_' || c == '-');
  }), value.end());
  return value;
}

std::string load_file(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return {};
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::string report_status(ExecutionReport::Status status) {
  switch (status) {
    case ExecutionReport::Status::Accepted: return "ACCEPTED";
    case ExecutionReport::Status::PartiallyFilled: return "PARTIAL";
    case ExecutionReport::Status::Filled: return "FILLED";
    case ExecutionReport::Status::Cancelled: return "CANCELLED";
    case ExecutionReport::Status::Replaced: return "REPLACED";
    case ExecutionReport::Status::Rejected: return "REJECTED";
    case ExecutionReport::Status::Expired: return "EXPIRED";
  }
  return "UNKNOWN";
}

std::optional<Side> parse_side(const std::string& value) {
  std::string v = value;
  std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if (v == "buy") return Side::Buy;
  if (v == "sell") return Side::Sell;
  return std::nullopt;
}

std::optional<OrderType> parse_type(const std::string& value) {
  std::string v = value;
  std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if (v == "limit") return OrderType::Limit;
  if (v == "market") return OrderType::Market;
  if (v == "ioc") return OrderType::IOC;
  if (v == "fok") return OrderType::FOK;
  if (v == "post" || v == "postonly") return OrderType::PostOnly;
  return std::nullopt;
}

template <typename T>
T number_field(const std::map<std::string, std::string>& fields, const std::string& key) {
  const auto it = fields.find(key);
  if (it == fields.end()) throw std::runtime_error("missing field: " + key);
  std::size_t consumed = 0;
  const unsigned long long value = std::stoull(it->second, &consumed);
  if (consumed != it->second.size()) throw std::runtime_error("invalid field: " + key);
  return static_cast<T>(value);
}

struct LoggedAction {
  enum class Kind { NewOrder, Cancel, Replace } kind{Kind::NewOrder};
  OrderRequest order{};
  CancelRequest cancel{};
  ReplaceRequest replace{};
  bool accepted{false};
  std::string actor{"MANUAL"};
};

struct DashboardState {
  Exchange exchange{100'000};
  std::deque<Trade> trades;
  std::deque<ExecutionReport> reports;
  std::uint64_t submitted{0};
  std::uint64_t accepted{0};
  std::uint64_t rejected{0};
  std::uint64_t trade_count{0};
  Quantity traded_quantity{0};
  std::vector<LoggedAction> timeline;
  std::mutex mutex;

  DashboardState() { attach(); }

  void attach() {
    exchange.on_trade([this](const Trade& trade) {
      trades.push_front(trade);
      while (trades.size() > 100) trades.pop_back();
      ++trade_count;
      traded_quantity += trade.quantity;
    });
    exchange.on_report([this](const ExecutionReport& report) {
      reports.push_front(report);
      while (reports.size() > 100) reports.pop_back();
      if (report.status == ExecutionReport::Status::Rejected) ++rejected;
      if (report.status == ExecutionReport::Status::Accepted) ++accepted;
    });
  }

  void reset() {
    exchange = Exchange{100'000};
    trades.clear();
    reports.clear();
    submitted = accepted = rejected = trade_count = 0;
    traded_quantity = 0;
    timeline.clear();
    attach();
  }
};

void scenario(DashboardState& state, const std::string& name) {
  state.reset();
  auto submit = [&](OrderId id, ClientId client, SymbolId symbol, Side side, OrderType type, Price price, Quantity qty) {
    ++state.submitted;
    const OrderRequest request{id, client, side, type, price, qty, symbol};
    const bool ok = state.exchange.submit(request);
    state.timeline.push_back({LoggedAction::Kind::NewOrder, request, {}, {}, ok, "SCENARIO"});
  };

  if (name == "fifo") {
    submit(1, 10, 1, Side::Sell, OrderType::Limit, 10000, 50);
    submit(2, 11, 1, Side::Sell, OrderType::Limit, 10000, 25);
    submit(3, 12, 1, Side::Buy, OrderType::Limit, 10000, 60);
  } else if (name == "multisymbol") {
    submit(1, 10, 1, Side::Buy, OrderType::Limit, 9998, 100);
    submit(2, 11, 1, Side::Sell, OrderType::Limit, 10002, 100);
    submit(3, 10, 2, Side::Buy, OrderType::Limit, 19995, 80);
    submit(4, 11, 2, Side::Sell, OrderType::Limit, 20005, 80);
    submit(5, 12, 1, Side::Buy, OrderType::Market, 0, 25);
  } else if (name == "advanced") {
    submit(1, 10, 1, Side::Sell, OrderType::Limit, 10000, 40);
    submit(2, 11, 1, Side::Sell, OrderType::Limit, 10001, 60);
    submit(3, 12, 1, Side::Buy, OrderType::FOK, 10001, 100);
    submit(4, 13, 1, Side::Buy, OrderType::PostOnly, 10002, 10);
    submit(5, 14, 1, Side::Buy, OrderType::PostOnly, 9999, 75);
  } else if (name == "flashcrash") {
    for (int i = 0; i < 12; ++i) submit(static_cast<OrderId>(100 + i), 20, 1, Side::Buy, OrderType::Limit, 10000 - i, static_cast<Quantity>(150 - i * 5));
    for (int i = 0; i < 8; ++i) submit(static_cast<OrderId>(200 + i), 21, 1, Side::Sell, OrderType::Market, 0, 120);
  } else if (name == "liquidityvacuum") {
    submit(1, 10, 1, Side::Buy, OrderType::Limit, 9980, 40);
    submit(2, 11, 1, Side::Sell, OrderType::Limit, 10020, 40);
    submit(3, 12, 1, Side::Buy, OrderType::Market, 0, 25);
    submit(4, 13, 1, Side::Sell, OrderType::Market, 0, 25);
  } else {
    submit(1, 10, 1, Side::Buy, OrderType::Limit, 9998, 75);
    submit(2, 11, 1, Side::Buy, OrderType::Limit, 9999, 100);
    submit(3, 12, 1, Side::Sell, OrderType::Limit, 10001, 90);
    submit(4, 13, 1, Side::Sell, OrderType::Limit, 10002, 110);
  }
}

std::string state_json(DashboardState& state, SymbolId symbol) {
  const OrderBook* book = state.exchange.find_book(symbol);
  std::ostringstream out;
  out << "{\"symbol\":" << symbol
      << ",\"symbols\":[";
  const auto symbols = state.exchange.symbols();
  for (std::size_t i = 0; i < symbols.size(); ++i) {
    if (i) out << ',';
    out << symbols[i];
  }
  out << "],\"metrics\":{\"submitted\":" << state.submitted
      << ",\"accepted\":" << state.accepted
      << ",\"rejected\":" << state.rejected
      << ",\"trades\":" << state.trade_count
      << ",\"tradedQuantity\":" << state.traded_quantity
      << ",\"activeOrders\":" << state.exchange.active_orders()
      << ",\"stateHash\":" << state.exchange.state_hash() << "}";

  auto emit_depth = [&](const char* name, Side side) {
    out << ",\"" << name << "\":[";
    if (book) {
      const auto levels = book->depth(side, 12);
      for (std::size_t i = 0; i < levels.size(); ++i) {
        if (i) out << ',';
        out << "{\"price\":" << levels[i].price
            << ",\"quantity\":" << levels[i].quantity
            << ",\"orders\":" << levels[i].order_count << '}';
      }
    }
    out << ']';
  };
  emit_depth("bids", Side::Buy);
  emit_depth("asks", Side::Sell);

  out << ",\"tradesFeed\":[";
  bool first = true;
  for (const auto& trade : state.trades) {
    if (trade.symbol != symbol) continue;
    if (!first) out << ',';
    first = false;
    out << "{\"sequence\":" << trade.sequence
        << ",\"maker\":" << trade.maker_order_id
        << ",\"taker\":" << trade.taker_order_id
        << ",\"price\":" << trade.price
        << ",\"quantity\":" << trade.quantity
        << ",\"side\":\"" << (trade.taker_side == Side::Buy ? "BUY" : "SELL") << "\"}";
    if (std::count_if(state.trades.begin(), state.trades.end(), [&](const Trade& t){ return t.symbol == symbol && t.sequence >= trade.sequence; }) >= 25) break;
  }
  out << "],\"reports\":[";
  first = true;
  std::size_t count = 0;
  for (const auto& report : state.reports) {
    if (report.symbol != symbol) continue;
    if (!first) out << ',';
    first = false;
    out << "{\"sequence\":" << report.sequence
        << ",\"orderId\":" << report.order_id
        << ",\"status\":\"" << report_status(report.status)
        << "\",\"remaining\":" << report.remaining
        << ",\"reason\":" << static_cast<int>(report.reject_reason) << '}';
    if (++count >= 25) break;
  }
  out << "]}";
  return out.str();
}

void apply_logged_action(DashboardState& state, const LoggedAction& action, bool record) {
  bool ok = false;
  if (action.kind == LoggedAction::Kind::NewOrder) {
    ++state.submitted;
    ok = state.exchange.submit(action.order);
  } else if (action.kind == LoggedAction::Kind::Cancel) {
    ok = state.exchange.cancel(action.cancel);
  } else {
    ok = state.exchange.replace(action.replace);
  }
  if (record) {
    LoggedAction copy = action;
    copy.accepted = ok;
    state.timeline.push_back(std::move(copy));
  }
}

std::string action_json(const LoggedAction& action, std::size_t index) {
  std::ostringstream out;
  out << "{\"index\":" << index << ",\"actor\":\"" << json_escape(action.actor)
      << "\",\"accepted\":" << (action.accepted ? "true" : "false");
  if (action.kind == LoggedAction::Kind::NewOrder) {
    out << ",\"kind\":\"NEW\",\"orderId\":" << action.order.order_id
        << ",\"symbol\":" << action.order.symbol << ",\"side\":\"" << to_string(action.order.side)
        << "\",\"price\":" << action.order.price << ",\"quantity\":" << action.order.quantity;
  } else if (action.kind == LoggedAction::Kind::Cancel) {
    out << ",\"kind\":\"CANCEL\",\"orderId\":" << action.cancel.order_id
        << ",\"symbol\":" << action.cancel.symbol;
  } else {
    out << ",\"kind\":\"REPLACE\",\"orderId\":" << action.replace.order_id
        << ",\"symbol\":" << action.replace.symbol << ",\"price\":" << action.replace.new_price
        << ",\"quantity\":" << action.replace.new_quantity;
  }
  out << '}';
  return out.str();
}

std::string timeline_json(const DashboardState& source, SymbolId symbol, std::size_t frame) {
  DashboardState replay;
  replay.reset();
  const std::size_t end = std::min(frame, source.timeline.size());
  for (std::size_t i = 0; i < end; ++i) apply_logged_action(replay, source.timeline[i], false);
  std::string body = state_json(replay, symbol);
  body.pop_back();
  body += ",\"timeline\":{\"frame\":" + std::to_string(end) + ",\"total\":" + std::to_string(source.timeline.size()) + ",\"actions\":[";
  const std::size_t begin = end > 30 ? end - 30 : 0;
  for (std::size_t i = begin; i < end; ++i) {
    if (i != begin) body += ',';
    body += action_json(source.timeline[i], i + 1);
  }
  body += "]}}";
  return body;
}

void run_arena(DashboardState& state, std::size_t steps, std::uint32_t drop_every, std::uint32_t duplicate_every) {
  state.reset();
  std::mt19937 rng(20260712U);
  std::uniform_int_distribution<int> offset(-8, 8);
  std::uniform_int_distribution<int> qty(1, 80);
  OrderId next_id = 1;
  for (std::size_t i = 0; i < steps; ++i) {
    if (drop_every != 0 && (i + 1) % drop_every == 0) continue;
    const int agent = static_cast<int>(i % 5);
    const Price mid = 10000 + static_cast<Price>((i / 25) % 7) - 3;
    Side side = Side::Buy;
    OrderType type = OrderType::Limit;
    Price price = mid;
    std::string actor;
    if (agent == 0) { actor = "MARKET_MAKER"; side = (i % 2 == 0) ? Side::Buy : Side::Sell; price = mid + (side == Side::Buy ? -2 : 2); }
    else if (agent == 1) { actor = "MOMENTUM"; side = ((i / 20) % 2 == 0) ? Side::Buy : Side::Sell; type = (i % 11 == 0) ? OrderType::Market : OrderType::IOC; price = mid + (side == Side::Buy ? 3 : -3); }
    else if (agent == 2) { actor = "MEAN_REVERSION"; side = offset(rng) > 0 ? Side::Sell : Side::Buy; price = mid + (side == Side::Buy ? -1 : 1); }
    else if (agent == 3) { actor = "INSTITUTIONAL_TWAP"; side = Side::Buy; type = OrderType::IOC; price = mid + 2; }
    else { actor = "NOISE"; side = (rng() & 1U) ? Side::Buy : Side::Sell; price = mid + offset(rng); }
    OrderRequest request{next_id++, static_cast<ClientId>(100 + agent), side, type, type == OrderType::Market ? 0 : price, static_cast<Quantity>(qty(rng)), 1};
    ++state.submitted;
    const bool ok = state.exchange.submit(request);
    state.timeline.push_back({LoggedAction::Kind::NewOrder, request, {}, {}, ok, actor});
    if (duplicate_every != 0 && (i + 1) % duplicate_every == 0) {
      ++state.submitted;
      const bool duplicate_ok = state.exchange.submit(request);
      state.timeline.push_back({LoggedAction::Kind::NewOrder, request, {}, {}, duplicate_ok, "FAULT_DUPLICATE"});
    }
  }
}

std::string explain_trade_json(const DashboardState& state, Sequence sequence) {
  for (const auto& trade : state.trades) {
    if (trade.sequence != sequence) continue;
    std::ostringstream out;
    out << "{\"found\":true,\"sequence\":" << trade.sequence
        << ",\"title\":\"Trade " << trade.sequence << " explanation\""
        << ",\"explanation\":\"Taker order " << trade.taker_order_id
        << " (" << to_string(trade.taker_side) << ") crossed resting maker order " << trade.maker_order_id
        << " at maker price " << trade.price << ". Price priority selected the best opposing price; FIFO selected the oldest order at that price. "
        << trade.quantity << " units executed.\"}";
    return out.str();
  }
  return "{\"found\":false}";
}


std::optional<std::string> json_string_field(const std::string& body,
                                             const std::string& key) {
  const std::string marker = "\"" + key + "\":";
  const std::size_t field = body.find(marker);
  if (field == std::string::npos) return std::nullopt;

  const std::size_t quote_begin = body.find('"', field + marker.size());
  if (quote_begin == std::string::npos) return std::nullopt;

  const std::size_t quote_end = body.find('"', quote_begin + 1);
  if (quote_end == std::string::npos) return std::nullopt;

  return body.substr(quote_begin + 1, quote_end - quote_begin - 1);
}

std::optional<bool> json_bool_field(const std::string& body,
                                    const std::string& key) {
  const std::string marker = "\"" + key + "\":";
  const std::size_t field = body.find(marker);
  if (field == std::string::npos) return std::nullopt;

  const std::size_t value_begin = field + marker.size();

  if (body.compare(value_begin, 4, "true") == 0) return true;
  if (body.compare(value_begin, 5, "false") == 0) return false;

  return std::nullopt;
}

std::optional<double> json_number_field(const std::string& body,
                                        const std::string& key) {
  const std::string marker = "\"" + key + "\":";
  const std::size_t field = body.find(marker);
  if (field == std::string::npos) return std::nullopt;

  const char* begin = body.c_str() + field + marker.size();
  char* end = nullptr;

  const double value = std::strtod(begin, &end);

  if (end == begin) return std::nullopt;
  return value;
}

struct VenueHealth {
  std::string venue;
  std::string status{"missing"};
  bool connected{false};
  bool transport_connected{false};
  double age_ms{-1.0};
  double spread{-1.0};
  double sequence_gaps{-1.0};
  bool healthy{false};
};

VenueHealth read_venue_health(const std::string& venue,
                              const std::string& symbol) {
  VenueHealth health;
  health.venue = venue;

  const std::string path =
      "data/live/" + venue + "_" + symbol + ".json";

  const std::string body = load_file(path);

  if (body.empty()) {
    health.status = "missing";
    return health;
  }

  health.status =
      json_string_field(body, "status").value_or("unknown");

  health.connected =
      json_bool_field(body, "connected").value_or(false);

  health.transport_connected =
      json_bool_field(body, "transportConnected").value_or(false);

  health.age_ms =
      json_number_field(body, "ageMs").value_or(-1.0);

  health.spread =
      json_number_field(body, "spread").value_or(-1.0);

  health.sequence_gaps =
      json_number_field(body, "sequenceGaps").value_or(-1.0);

  health.healthy =
      health.status == "live" &&
      health.connected &&
      health.transport_connected &&
      health.age_ms >= 0.0 &&
      health.age_ms <= 15000.0 &&
      health.spread > 0.0 &&
      health.sequence_gaps == 0.0;

  return health;
}

std::string venue_health_json(const VenueHealth& health) {
  std::ostringstream out;

  out << "{"
      << "\"status\":\"" << json_escape(health.status) << "\","
      << "\"connected\":"
      << (health.connected ? "true" : "false") << ","
      << "\"transportConnected\":"
      << (health.transport_connected ? "true" : "false") << ","
      << "\"ageMs\":" << health.age_ms << ","
      << "\"spread\":" << health.spread << ","
      << "\"sequenceGaps\":" << health.sequence_gaps << ","
      << "\"healthy\":"
      << (health.healthy ? "true" : "false")
      << "}";

  return out.str();
}



std::optional<std::string> json_object_field(const std::string& body,
                                             const std::string& key) {
  const std::string marker = "\"" + key + "\":";
  const std::size_t field = body.find(marker);
  if (field == std::string::npos) return std::nullopt;

  const std::size_t object_begin = body.find('{', field + marker.size());
  if (object_begin == std::string::npos) return std::nullopt;

  std::size_t depth = 0;

  for (std::size_t index = object_begin; index < body.size(); ++index) {
    if (body[index] == '{') {
      ++depth;
    } else if (body[index] == '}') {
      if (--depth == 0) {
        return body.substr(
            object_begin,
            index - object_begin + 1
        );
      }
    }
  }

  return std::nullopt;
}

struct BookLevelMetric {
  double price{-1.0};
  double quantity{-1.0};
  bool valid{false};
};

BookLevelMetric json_first_book_level(const std::string& body,
                                      const std::string& side) {
  BookLevelMetric level;

  const std::string marker = "\"" + side + "\":[";
  const std::size_t array_begin = body.find(marker);

  if (array_begin == std::string::npos) return level;

  const std::size_t object_begin =
      body.find('{', array_begin + marker.size());

  if (object_begin == std::string::npos) return level;

  const std::size_t object_end = body.find('}', object_begin);

  if (object_end == std::string::npos) return level;

  const std::string object =
      body.substr(object_begin, object_end - object_begin + 1);

  const auto price = json_number_field(object, "price");
  const auto quantity = json_number_field(object, "quantity");

  if (!price || !quantity) return level;

  level.price = *price;
  level.quantity = *quantity;
  level.valid = true;

  return level;
}


struct ConsolidatedVenueQuote {
  std::string venue;
  bool healthy{false};
  double age_ms{-1.0};
  double bid{-1.0};
  double bid_quantity{-1.0};
  double ask{-1.0};
  double ask_quantity{-1.0};
};

ConsolidatedVenueQuote read_consolidated_quote(
    const std::string& venue,
    const std::string& symbol) {
  ConsolidatedVenueQuote quote;
  quote.venue = venue;

  const VenueHealth health = read_venue_health(venue, symbol);
  quote.healthy = health.healthy;
  quote.age_ms = health.age_ms;

  const std::string body = load_file(
      "data/live/" + venue + "_" + symbol + ".json"
  );

  if (body.empty()) {
    quote.healthy = false;
    return quote;
  }

  const BookLevelMetric bid =
      json_first_book_level(body, "bids");

  const BookLevelMetric ask =
      json_first_book_level(body, "asks");

  if (!bid.valid || !ask.valid) {
    quote.healthy = false;
    return quote;
  }

  quote.bid = bid.price;
  quote.bid_quantity = bid.quantity;
  quote.ask = ask.price;
  quote.ask_quantity = ask.quantity;

  if (quote.ask <= quote.bid) {
    quote.healthy = false;
  }

  return quote;
}



double environment_double(
    const char* name,
    double fallback) {
  const char* raw = std::getenv(name);

  if (raw == nullptr || *raw == '\0') {
    return fallback;
  }

  try {
    const double value = std::stod(raw);

    if (!std::isfinite(value)) {
      return fallback;
    }

    return value;
  } catch (...) {
    return fallback;
  }
}


std::vector<minimatch::VenueLevel> json_book_levels(
    const std::string& body,
    const std::string& side,
    std::size_t maximum_levels = 20) {
  std::vector<minimatch::VenueLevel> levels;

  const std::string marker =
      "\"" + side + "\":[";

  const std::size_t array_begin =
      body.find(marker);

  if (array_begin == std::string::npos) {
    return levels;
  }

  std::size_t position =
      array_begin + marker.size();

  while (levels.size() < maximum_levels) {
    const std::size_t object_begin =
        body.find('{', position);

    const std::size_t array_end =
        body.find(']', position);

    if (object_begin == std::string::npos ||
        array_end == std::string::npos ||
        object_begin > array_end) {
      break;
    }

    const std::size_t object_end =
        body.find('}', object_begin);

    if (object_end == std::string::npos ||
        object_end > array_end) {
      break;
    }

    const std::string object =
        body.substr(
            object_begin,
            object_end - object_begin + 1
        );

    const auto price =
        json_number_field(object, "price");

    const auto quantity =
        json_number_field(object, "quantity");

    if (price &&
        quantity &&
        *price > 0.0 &&
        *quantity > 0.0) {
      levels.push_back(
          minimatch::VenueLevel{
              *price,
              *quantity
          }
      );
    }

    position = object_end + 1;
  }

  return levels;
}

std::vector<minimatch::VenueQuote> read_router_quotes(
    const std::string& symbol) {
  const std::array<std::string, 3> venues{
      "coinbase", "kraken", "binance"
  };

  std::vector<minimatch::VenueQuote> quotes;
  quotes.reserve(venues.size());

  for (const auto& venue : venues) {
    const VenueHealth health =
        read_venue_health(venue, symbol);

    const std::string body = load_file(
        "data/live/" + venue + "_" + symbol + ".json"
    );

    const auto bids =
        json_book_levels(body, "bids", 20);

    const auto asks =
        json_book_levels(body, "asks", 20);

    double taker_fee_bps = 0.0;
    double latency_penalty_ms = 0.0;

    if (venue == "coinbase") {
      taker_fee_bps = environment_double(
          "ROUTER_COINBASE_TAKER_FEE_BPS",
          60.0
      );

      latency_penalty_ms = environment_double(
          "ROUTER_COINBASE_LATENCY_MS",
          0.0
      );
    } else if (venue == "kraken") {
      taker_fee_bps = environment_double(
          "ROUTER_KRAKEN_TAKER_FEE_BPS",
          40.0
      );

      latency_penalty_ms = environment_double(
          "ROUTER_KRAKEN_LATENCY_MS",
          0.0
      );
    } else if (venue == "binance") {
      taker_fee_bps = environment_double(
          "ROUTER_BINANCE_TAKER_FEE_BPS",
          10.0
      );

      latency_penalty_ms = environment_double(
          "ROUTER_BINANCE_LATENCY_MS",
          0.0
      );
    }

    const double latency_ms =
        std::max(0.0, health.age_ms) +
        std::max(0.0, latency_penalty_ms);

    const double latency_cost_bps_per_ms =
        std::max(
            0.0,
            environment_double(
                "ROUTER_LATENCY_COST_BPS_PER_MS",
                0.0
            )
        );

    const bool healthy =
        health.healthy &&
        !bids.empty() &&
        !asks.empty();

    quotes.push_back(
        minimatch::VenueQuote{
            venue,
            bids,
            asks,
            taker_fee_bps,
            latency_ms,
            latency_cost_bps_per_ms,
            healthy
        }
    );
  }

  return quotes;
}

std::string route_plan_json(
    const minimatch::RouteRequest& request,
    const minimatch::RoutePlan& plan,
    const std::string& symbol) {
  std::ostringstream out;
  out << std::setprecision(15);

  out << "{"
      << "\"symbol\":\"" << json_escape(symbol) << "\","
      << "\"side\":\""
      << (request.side == minimatch::RouteSide::Buy
              ? "BUY"
              : "SELL")
      << "\","
      << "\"requestedQuantity\":" << plan.requested_quantity << ","
      << "\"routedQuantity\":" << plan.routed_quantity << ","
      << "\"complete\":" << (plan.complete ? "true" : "false") << ","
      << "\"averagePrice\":" << plan.average_price << ","
      << "\"estimatedNotional\":" << plan.estimated_notional << ","
      << "\"estimatedFees\":" << plan.estimated_fees << ","
      << "\"legs\":[";

  for (std::size_t index = 0; index < plan.legs.size(); ++index) {
    if (index > 0) {
      out << ",";
    }

    const auto& leg = plan.legs[index];

    out << "{"
        << "\"venue\":\"" << json_escape(leg.venue) << "\","
        << "\"price\":" << leg.price << ","
        << "\"quantity\":" << leg.quantity << ","
        << "\"estimatedFee\":" << leg.estimated_fee << ","
        << "\"effectivePrice\":" << leg.effective_price << ","
        << "\"latencyMs\":" << leg.latency_ms << ","
        << "\"takerFeeBps\":" << leg.taker_fee_bps << ","
        << "\"latencyCostBpsPerMs\":"
        << leg.latency_cost_bps_per_ms << ","
        << "\"levelIndex\":" << leg.level_index
        << "}";
  }

  out << "]}";

  return out.str();
}

std::string consolidated_market_json(const std::string& symbol) {
  const std::array<std::string, 3> venues{
      "coinbase", "kraken", "binance"
  };

  std::array<ConsolidatedVenueQuote, 3> quotes;

  bool found_bid = false;
  bool found_ask = false;

  ConsolidatedVenueQuote best_bid;
  ConsolidatedVenueQuote best_ask;

  for (std::size_t index = 0; index < venues.size(); ++index) {
    quotes[index] = read_consolidated_quote(
        venues[index],
        symbol
    );

    const auto& quote = quotes[index];

    if (!quote.healthy) {
      continue;
    }

    if (!found_bid || quote.bid > best_bid.bid) {
      best_bid = quote;
      found_bid = true;
    }

    if (!found_ask || quote.ask < best_ask.ask) {
      best_ask = quote;
      found_ask = true;
    }
  }

  std::ostringstream out;
  out << std::setprecision(15);

  out << "{"
      << "\"symbol\":\"" << json_escape(symbol) << "\","
      << "\"status\":\""
      << (found_bid && found_ask ? "live" : "unavailable")
      << "\",";

  if (found_bid && found_ask) {
    const double spread = best_ask.ask - best_bid.bid;
    const double midpoint =
        (best_bid.bid + best_ask.ask) / 2.0;

    out << "\"bestBid\":{"
        << "\"venue\":\""
        << json_escape(best_bid.venue) << "\","
        << "\"price\":" << best_bid.bid << ","
        << "\"quantity\":" << best_bid.bid_quantity
        << "},"
        << "\"bestAsk\":{"
        << "\"venue\":\""
        << json_escape(best_ask.venue) << "\","
        << "\"price\":" << best_ask.ask << ","
        << "\"quantity\":" << best_ask.ask_quantity
        << "},"
        << "\"spread\":" << spread << ","
        << "\"midpoint\":" << midpoint << ","
        << "\"crossed\":"
        << (spread < 0.0 ? "true" : "false") << ",";
  } else {
    out << "\"bestBid\":null,"
        << "\"bestAsk\":null,"
        << "\"spread\":null,"
        << "\"midpoint\":null,"
        << "\"crossed\":false,";
  }

  out << "\"venues\":{";

  for (std::size_t index = 0; index < quotes.size(); ++index) {
    if (index > 0) {
      out << ",";
    }

    const auto& quote = quotes[index];

    out << "\"" << json_escape(quote.venue) << "\":{"
        << "\"healthy\":"
        << (quote.healthy ? "true" : "false") << ","
        << "\"ageMs\":" << quote.age_ms << ","
        << "\"bid\":"
        << (quote.bid >= 0.0
                ? std::to_string(quote.bid)
                : "null")
        << ","
        << "\"bidQuantity\":"
        << (quote.bid_quantity >= 0.0
                ? std::to_string(quote.bid_quantity)
                : "null")
        << ","
        << "\"ask\":"
        << (quote.ask >= 0.0
                ? std::to_string(quote.ask)
                : "null")
        << ","
        << "\"askQuantity\":"
        << (quote.ask_quantity >= 0.0
                ? std::to_string(quote.ask_quantity)
                : "null")
        << "}";
  }

  out << "}}";

  return out.str();
}

std::string prometheus_metrics(const std::string& symbol) {
  const std::array<std::string, 3> venues{
      "coinbase", "kraken", "binance"
  };

  std::ostringstream out;

  out
      << "# HELP minimatch_venue_up Whether the venue feed is healthy\n"
      << "# TYPE minimatch_venue_up gauge\n"
      << "# HELP minimatch_feed_age_milliseconds Age of the latest event\n"
      << "# TYPE minimatch_feed_age_milliseconds gauge\n"
      << "# HELP minimatch_spread Best ask minus best bid\n"
      << "# TYPE minimatch_spread gauge\n"
      << "# HELP minimatch_sequence_gaps_total Detected sequence gaps\n"
      << "# TYPE minimatch_sequence_gaps_total counter\n"
      << "# HELP minimatch_events_total Normalized events processed\n"
      << "# TYPE minimatch_events_total counter\n"
      << "# HELP minimatch_trades_total Normalized trades processed\n"
      << "# TYPE minimatch_trades_total counter\n"
      << "# HELP minimatch_updates_per_second Current normalized update rate\n"
      << "# TYPE minimatch_updates_per_second gauge\n"
      << "# HELP minimatch_reconnects_total Feed reconnect attempts\n"
      << "# TYPE minimatch_reconnects_total counter\n"
      << "# HELP minimatch_dropped_events_total Dropped normalized events\n"
      << "# TYPE minimatch_dropped_events_total counter\n"
      << "# HELP minimatch_best_bid Current best bid price\n"
      << "# TYPE minimatch_best_bid gauge\n"
      << "# HELP minimatch_best_ask Current best ask price\n"
      << "# TYPE minimatch_best_ask gauge\n"
      << "# HELP minimatch_midprice Current midpoint\n"
      << "# TYPE minimatch_midprice gauge\n"
      << "# HELP minimatch_microprice Queue-weighted microprice\n"
      << "# TYPE minimatch_microprice gauge\n";

  for (const auto& venue : venues) {
    const std::string path =
        "data/live/" + venue + "_" + symbol + ".json";

    const std::string body = load_file(path);
    const VenueHealth health = read_venue_health(venue, symbol);

    const std::string labels =
        "{venue=\"" + venue + "\",symbol=\"" + symbol + "\"}";

    const std::string metrics =
        json_object_field(body, "metrics").value_or("{}");

    const double events =
        json_number_field(metrics, "events").value_or(0.0);

    const double trades =
        json_number_field(metrics, "trades").value_or(0.0);

    const double updates_per_second =
        json_number_field(metrics, "updatesPerSecond").value_or(0.0);

    const double reconnects =
        json_number_field(metrics, "reconnects").value_or(0.0);

    const double dropped_events =
        json_number_field(metrics, "droppedEvents").value_or(0.0);

    const BookLevelMetric bid =
        json_first_book_level(body, "bids");

    const BookLevelMetric ask =
        json_first_book_level(body, "asks");

    const double best_bid = bid.valid ? bid.price : -1.0;
    const double best_ask = ask.valid ? ask.price : -1.0;

    const double spread =
        bid.valid && ask.valid ? best_ask - best_bid : -1.0;

    const double midprice =
        bid.valid && ask.valid
            ? (best_bid + best_ask) / 2.0
            : -1.0;

    double microprice =
        json_number_field(metrics, "microprice").value_or(-1.0);

    if (microprice < 0.0 &&
        bid.valid &&
        ask.valid &&
        bid.quantity + ask.quantity > 0.0) {
      microprice =
          (best_ask * bid.quantity +
           best_bid * ask.quantity) /
          (bid.quantity + ask.quantity);
    }

    out << "minimatch_venue_up" << labels << " "
        << (health.healthy ? 1 : 0) << "\n";

    out << "minimatch_feed_age_milliseconds" << labels << " "
        << health.age_ms << "\n";

    out << "minimatch_spread" << labels << " "
        << spread << "\n";

    out << "minimatch_sequence_gaps_total" << labels << " "
        << health.sequence_gaps << "\n";

    out << "minimatch_events_total" << labels << " "
        << events << "\n";

    out << "minimatch_trades_total" << labels << " "
        << trades << "\n";

    out << "minimatch_updates_per_second" << labels << " "
        << updates_per_second << "\n";

    out << "minimatch_reconnects_total" << labels << " "
        << reconnects << "\n";

    out << "minimatch_dropped_events_total" << labels << " "
        << dropped_events << "\n";

    out << "minimatch_best_bid" << labels << " "
        << best_bid << "\n";

    out << "minimatch_best_ask" << labels << " "
        << best_ask << "\n";

    out << "minimatch_midprice" << labels << " "
        << midprice << "\n";

    out << "minimatch_microprice" << labels << " "
        << microprice << "\n";
  }

  return out.str();
}


http::response<http::string_body> response(http::status status, std::string body,
                                           std::string_view content_type = "application/json") {
  http::response<http::string_body> res{status, 11};
  res.set(http::field::server, "MiniMatch-Web");
  res.set(http::field::content_type, content_type);
  res.set(http::field::cache_control, "no-store");
  res.body() = std::move(body);
  res.prepare_payload();
  return res;
}



std::string execution_database_path() {
  const char* configured =
      std::getenv("EXECUTION_DB_PATH");

  if (configured != nullptr && *configured != '\0') {
    return configured;
  }

  return "data/executions/router_executions.db";
}

std::string routed_execution_json(
    const minimatch::RoutedExecutionSummary& summary,
    const std::string& symbol,
    minimatch::RouteSide side,
    const minimatch::ExecutionSimulationConfig& config,
    std::int64_t execution_id) {
  std::ostringstream out;
  out << std::setprecision(15);

  out << "{"
      << "\"executionId\":" << execution_id << ","
      << "\"symbol\":\"" << json_escape(symbol) << "\","
      << "\"side\":\""
      << (side == minimatch::RouteSide::Buy ? "BUY" : "SELL")
      << "\","
      << "\"complete\":"
      << (summary.complete ? "true" : "false") << ","
      << "\"requestedQuantity\":"
      << summary.requested_quantity << ","
      << "\"filledQuantity\":"
      << summary.filled_quantity << ","
      << "\"remainingQuantity\":"
      << summary.remaining_quantity << ","
      << "\"averageFillPrice\":"
      << summary.average_fill_price << ","
      << "\"totalNotional\":"
      << summary.total_notional << ","
      << "\"totalFees\":"
      << summary.total_fees << ","
      << "\"totalLatencyMs\":"
      << summary.total_latency_ms << ","
      << "\"simulation\":{"
      << "\"fillRatio\":" << config.fill_ratio << ","
      << "\"rejectionProbability\":"
      << config.rejection_probability << ","
      << "\"baseLatencyMs\":"
      << config.base_latency_ms << ","
      << "\"latencyJitterMs\":"
      << config.latency_jitter_ms << ","
      << "\"seed\":" << config.seed
      << "},"
      << "\"children\":[";

  for (std::size_t index = 0;
       index < summary.children.size();
       ++index) {
    if (index > 0) {
      out << ",";
    }

    const auto& child = summary.children[index];

    out << "{"
        << "\"venue\":\""
        << json_escape(child.venue) << "\","
        << "\"levelIndex\":"
        << child.level_index << ","
        << "\"requestedQuantity\":"
        << child.requested_quantity << ","
        << "\"filledQuantity\":"
        << child.filled_quantity << ","
        << "\"remainingQuantity\":"
        << child.remaining_quantity << ","
        << "\"price\":"
        << child.price << ","
        << "\"notional\":"
        << child.notional << ","
        << "\"fee\":"
        << child.fee << ","
        << "\"latencyMs\":"
        << child.latency_ms << ","
        << "\"status\":\""
        << minimatch::to_string(child.status)
        << "\""
        << "}";
  }

  out << "]}";
  return out.str();
}

http::response<http::string_body> handle_request(DashboardState& state,
                                                  const http::request<http::string_body>& req,
                                                  const std::string& frontend_dir) {
  const std::string target(req.target());
  const std::string path = target.substr(0, target.find('?'));

  try {
    std::lock_guard<std::mutex> lock(state.mutex);
    if (req.method() == http::verb::get && path == "/metrics") {
      const auto query = parse_query(target);
      const std::string symbol = safe_token(
          query.count("symbol") ? query.at("symbol") : "btcusd"
      );

      if (symbol.empty()) {
        throw std::runtime_error("invalid symbol");
      }

      return response(
          http::status::ok,
          prometheus_metrics(symbol),
          "text/plain; version=0.0.4; charset=utf-8"
      );
    }

    if (req.method() == http::verb::get &&
        path == "/api/market/consolidated") {
      const auto query = parse_query(target);

      const std::string symbol = safe_token(
          query.count("symbol")
              ? query.at("symbol")
              : "btcusd"
      );

      if (symbol.empty()) {
        throw std::runtime_error("invalid symbol");
      }

      return response(
          http::status::ok,
          consolidated_market_json(symbol)
      );
    }

    if (req.method() == http::verb::get && path == "/api/health") {
      const auto query = parse_query(target);

      const std::string symbol = safe_token(
          query.count("symbol") ? query.at("symbol") : "btcusd"
      );

      if (symbol.empty()) {
        throw std::runtime_error("invalid symbol");
      }

      const VenueHealth coinbase =
          read_venue_health("coinbase", symbol);

      const VenueHealth kraken =
          read_venue_health("kraken", symbol);

      const VenueHealth binance =
          read_venue_health("binance", symbol);

      const bool healthy =
          coinbase.healthy &&
          kraken.healthy &&
          binance.healthy;

      std::ostringstream out;

      out << "{"
          << "\"status\":\""
          << (healthy ? "healthy" : "degraded")
          << "\","
          << "\"symbol\":\"" << json_escape(symbol) << "\","
          << "\"venues\":{"
          << "\"coinbase\":" << venue_health_json(coinbase) << ","
          << "\"kraken\":" << venue_health_json(kraken) << ","
          << "\"binance\":" << venue_health_json(binance)
          << "}"
          << "}";

      return response(
          healthy
              ? http::status::ok
              : http::status::service_unavailable,
          out.str()
      );
    }

    if (req.method() == http::verb::get && path == "/api/live/state") {
      const auto query = parse_query(target);
      const std::string venue = safe_token(query.count("venue") ? query.at("venue") : "binance");
      const std::string symbol = safe_token(query.count("symbol") ? query.at("symbol") : "btcusdt");
      if (venue.empty() || symbol.empty()) throw std::runtime_error("invalid venue or symbol");
      const std::string live_path = "data/live/" + venue + "_" + symbol + ".json";
      std::string body = load_file(live_path);
      if (body.empty()) {
        return response(http::status::service_unavailable,
                        "{\"connected\":false,\"status\":\"No live adapter state. Start scripts/run_live_" + venue + ".sh " + symbol + "\",\"venue\":\"" + json_escape(venue) + "\",\"symbol\":\"" + json_escape(symbol) + "\",\"bids\":[],\"asks\":[],\"trades\":[],\"metrics\":{}}");
      }
      return response(http::status::ok, std::move(body));
    }

    if (req.method() == http::verb::get && path == "/api/state") {
      const auto query = parse_query(target);
      SymbolId symbol = 1;
      if (const auto it = query.find("symbol"); it != query.end()) symbol = static_cast<SymbolId>(std::stoul(it->second));
      return response(http::status::ok, state_json(state, symbol));
    }

    if (req.method() == http::verb::post &&
        path == "/api/router/execute") {
      const auto fields = parse_form(req.body());

      const std::string symbol = safe_token(
          fields.count("symbol")
              ? fields.at("symbol")
              : "btcusd"
      );

      if (symbol.empty()) {
        throw std::runtime_error("invalid symbol");
      }

      const std::string side_value =
          fields.count("side")
              ? fields.at("side")
              : "";

      minimatch::RouteSide side;

      if (side_value == "buy" || side_value == "BUY") {
        side = minimatch::RouteSide::Buy;
      } else if (side_value == "sell" ||
                 side_value == "SELL") {
        side = minimatch::RouteSide::Sell;
      } else {
        throw std::runtime_error(
            "side must be buy or sell"
        );
      }

      const double quantity =
          fields.count("quantity")
              ? std::stod(fields.at("quantity"))
              : 0.0;

      const double fill_ratio =
          fields.count("fillRatio")
              ? std::stod(fields.at("fillRatio"))
              : 1.0;

      const double rejection_probability =
          fields.count("rejectionProbability")
              ? std::stod(
                    fields.at("rejectionProbability")
                )
              : 0.0;

      const double base_latency_ms =
          fields.count("baseLatencyMs")
              ? std::stod(fields.at("baseLatencyMs"))
              : 0.0;

      const double latency_jitter_ms =
          fields.count("latencyJitterMs")
              ? std::stod(fields.at("latencyJitterMs"))
              : 0.0;

      const std::uint64_t seed =
          fields.count("seed")
              ? static_cast<std::uint64_t>(
                    std::stoull(fields.at("seed"))
                )
              : 1ULL;

      const minimatch::RouteRequest route_request{
          side,
          quantity
      };

      const auto quotes =
          read_router_quotes(symbol);

      const minimatch::RoutePlan plan =
          minimatch::build_route_plan(
              route_request,
              quotes
          );

      const minimatch::ExecutionSimulationConfig config{
          .fill_ratio = fill_ratio,
          .rejection_probability =
              rejection_probability,
          .base_latency_ms =
              base_latency_ms,
          .latency_jitter_ms =
              latency_jitter_ms,
          .seed = seed
      };

      const minimatch::RoutedExecutionSummary summary =
          minimatch::simulate_route_execution(
              plan,
              config
          );

      static minimatch::ExecutionStore execution_store(
          execution_database_path()
      );

      const std::int64_t execution_id =
          execution_store.save(
              symbol,
              side,
              config,
              summary
          );

      return response(
          http::status::ok,
          routed_execution_json(
              summary,
              symbol,
              side,
              config,
              execution_id
          )
      );
    }

    if (req.method() == http::verb::post &&
        path == "/api/router/preview") {
      const auto fields = parse_form(req.body());

      const std::string symbol = safe_token(
          fields.count("symbol")
              ? fields.at("symbol")
              : "btcusd"
      );

      if (symbol.empty()) {
        throw std::runtime_error("invalid symbol");
      }

      const std::string side_value =
          fields.count("side")
              ? fields.at("side")
              : "";

      minimatch::RouteSide side;

      if (side_value == "buy" || side_value == "BUY") {
        side = minimatch::RouteSide::Buy;
      } else if (side_value == "sell" || side_value == "SELL") {
        side = minimatch::RouteSide::Sell;
      } else {
        throw std::runtime_error(
            "side must be buy or sell"
        );
      }

      const double quantity =
          fields.count("quantity")
              ? std::stod(fields.at("quantity"))
              : 0.0;

      const minimatch::RouteRequest route_request{
          side,
          quantity
      };

      const auto quotes = read_router_quotes(symbol);

      const minimatch::RoutePlan plan =
          minimatch::build_route_plan(
              route_request,
              quotes
          );

      return response(
          http::status::ok,
          route_plan_json(
              route_request,
              plan,
              symbol
          )
      );
    }

    if (req.method() == http::verb::post && path == "/api/order") {
      const auto fields = parse_form(req.body());
      const auto side = parse_side(fields.at("side"));
      const auto type = parse_type(fields.at("type"));
      if (!side || !type) throw std::runtime_error("invalid side or type");
      OrderRequest order{
        number_field<OrderId>(fields, "orderId"),
        number_field<ClientId>(fields, "clientId"),
        *side,
        *type,
        number_field<Price>(fields, "price"),
        number_field<Quantity>(fields, "quantity"),
        number_field<SymbolId>(fields, "symbol")
      };
      ++state.submitted;
      const bool ok = state.exchange.submit(order);
      state.timeline.push_back({LoggedAction::Kind::NewOrder, order, {}, {}, ok, "MANUAL"});
      return response(ok ? http::status::ok : http::status::unprocessable_entity,
                      std::string("{\"ok\":") + (ok ? "true" : "false") + "}");
    }

    if (req.method() == http::verb::post && path == "/api/cancel") {
      const auto fields = parse_form(req.body());
      const CancelRequest request{number_field<OrderId>(fields, "orderId"),
                                  number_field<ClientId>(fields, "clientId"),
                                  number_field<SymbolId>(fields, "symbol")};
      const bool ok = state.exchange.cancel(request);
      state.timeline.push_back({LoggedAction::Kind::Cancel, {}, request, {}, ok, "MANUAL"});
      return response(ok ? http::status::ok : http::status::unprocessable_entity,
                      std::string("{\"ok\":") + (ok ? "true" : "false") + "}");
    }

    if (req.method() == http::verb::post && path == "/api/replace") {
      const auto fields = parse_form(req.body());
      const ReplaceRequest request{number_field<OrderId>(fields, "orderId"),
                                    number_field<ClientId>(fields, "clientId"),
                                    number_field<Price>(fields, "price"),
                                    number_field<Quantity>(fields, "quantity"),
                                    number_field<SymbolId>(fields, "symbol")};
      const bool ok = state.exchange.replace(request);
      state.timeline.push_back({LoggedAction::Kind::Replace, {}, {}, request, ok, "MANUAL"});
      return response(ok ? http::status::ok : http::status::unprocessable_entity,
                      std::string("{\"ok\":") + (ok ? "true" : "false") + "}");
    }

    if (req.method() == http::verb::post && path == "/api/reset") {
      state.reset();
      return response(http::status::ok, "{\"ok\":true}");
    }

    if (req.method() == http::verb::post && path == "/api/scenario") {
      const auto fields = parse_form(req.body());
      scenario(state, fields.at("name"));
      return response(http::status::ok, "{\"ok\":true}");
    }

    if (req.method() == http::verb::get && path == "/api/timeline") {
      const auto query = parse_query(target);
      const SymbolId symbol = query.count("symbol") ? static_cast<SymbolId>(std::stoul(query.at("symbol"))) : 1;
      const std::size_t frame = query.count("frame") ? static_cast<std::size_t>(std::stoull(query.at("frame"))) : state.timeline.size();
      return response(http::status::ok, timeline_json(state, symbol, frame));
    }

    if (req.method() == http::verb::get && path == "/api/explain") {
      const auto query = parse_query(target);
      return response(http::status::ok, explain_trade_json(state, static_cast<Sequence>(std::stoull(query.at("sequence")))));
    }

    if (req.method() == http::verb::post && path == "/api/arena") {
      const auto fields = parse_form(req.body());
      const std::size_t steps = fields.count("steps") ? static_cast<std::size_t>(std::stoull(fields.at("steps"))) : 500;
      const std::uint32_t drop_every = fields.count("dropEvery") ? static_cast<std::uint32_t>(std::stoul(fields.at("dropEvery"))) : 0;
      const std::uint32_t duplicate_every = fields.count("duplicateEvery") ? static_cast<std::uint32_t>(std::stoul(fields.at("duplicateEvery"))) : 0;
      run_arena(state, std::min<std::size_t>(steps, 5000), drop_every, duplicate_every);
      return response(http::status::ok, "{\"ok\":true,\"frames\":" + std::to_string(state.timeline.size()) + "}");
    }

    if (req.method() == http::verb::get && (path == "/" || path == "/index.html")) {
      std::string html = load_file(frontend_dir + "/index.html");
      if (html.empty()) return response(http::status::not_found, "frontend/index.html not found", "text/plain");
      return response(http::status::ok, std::move(html), "text/html; charset=utf-8");
    }
    if (req.method() == http::verb::get && path == "/app.js") {
      return response(http::status::ok, load_file(frontend_dir + "/app.js"), "text/javascript; charset=utf-8");
    }
    if (req.method() == http::verb::get && path == "/styles.css") {
      return response(http::status::ok, load_file(frontend_dir + "/styles.css"), "text/css; charset=utf-8");
    }
    return response(http::status::not_found, "{\"error\":\"not found\"}");
  } catch (const std::exception& error) {
    return response(http::status::bad_request,
                    "{\"error\":\"" + json_escape(error.what()) + "\"}");
  }
}

void session(tcp::socket socket, DashboardState& state, const std::string& frontend_dir) {
  beast::flat_buffer buffer;
  beast::error_code ec;
  http::request<http::string_body> req;
  http::read(socket, buffer, req, ec);
  if (ec) return;
  auto res = handle_request(state, req, frontend_dir);
  res.keep_alive(false);
  http::write(socket, res, ec);
  socket.shutdown(tcp::socket::shutdown_send, ec);
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const unsigned short port = argc > 1 ? static_cast<unsigned short>(std::stoul(argv[1])) : 8080;
    const std::string frontend_dir = argc > 2 ? argv[2] : "frontend";
    asio::io_context io;
    tcp::acceptor acceptor{io, {tcp::v4(), port}};
    DashboardState state;
    scenario(state, "basic");
    std::cout << "MiniMatch dashboard: http://127.0.0.1:" << port << '\n';
    std::cout << "Serving frontend from: " << frontend_dir << '\n';
    for (;;) {
      tcp::socket socket{io};
      acceptor.accept(socket);
      session(std::move(socket), state, frontend_dir);
    }
  } catch (const std::exception& error) {
    std::cerr << "web server error: " << error.what() << '\n';
    return 1;
  }
}
