#include "minimatch/exchange.hpp"
#include "minimatch/fix_store.hpp"
#include "minimatch/latency_stats.hpp"
#include "minimatch/oms.hpp"
#include "minimatch/execution_algo.hpp"
#include "minimatch/market_replay.hpp"
#include "minimatch/backtest_engine.hpp"
#include "minimatch/risk.hpp"
#include "minimatch/router.hpp"
#include "minimatch/execution_engine.hpp"
#include "minimatch/execution_store.hpp"
#include "minimatch/live_router_quotes.hpp"
#include "minimatch/types.hpp"

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <limits>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

using tcp = asio::ip::tcp;

using minimatch::CancelRequest;
using minimatch::ClientId;
using minimatch::Exchange;
using minimatch::ExecutionReport;
using minimatch::LatencyStats;
using minimatch::OrderId;
using minimatch::OrderRequest;
using minimatch::OrderType;
using minimatch::OrderManagementSystem;
using minimatch::ParentOrderRequest;
using minimatch::ChildOrderRequest;
using minimatch::ParentOrderId;
using minimatch::RouteSide;
using minimatch::ExecutionAlgorithm;
using minimatch::Price;
using minimatch::Quantity;
using minimatch::ReplaceRequest;
using minimatch::RiskEngine;
using minimatch::Side;
using minimatch::SymbolId;
using minimatch::Trade;

namespace {

struct ApiState {
    Exchange exchange{100'000};
    RiskEngine risk;
    LatencyStats latency;
    OrderManagementSystem oms;

    std::uint64_t submitted{0};
    std::uint64_t accepted{0};
    std::uint64_t rejected{0};
    std::uint64_t trades{0};
    std::uint64_t reports{0};

    std::deque<Trade> trade_feed;
    std::deque<ExecutionReport> report_feed;

    std::unordered_map<OrderId, OrderRequest> active_order_metadata;

    std::mutex mutex;

    ApiState() {
        exchange.on_trade([this](const Trade& trade) {
            ++trades;

            trade_feed.push_front(trade);

            while (trade_feed.size() > 100) {
                trade_feed.pop_back();
            }
        });

        exchange.on_report([this](const ExecutionReport& report) {
            ++reports;

            report_feed.push_front(report);

            while (report_feed.size() > 100) {
                report_feed.pop_back();
            }

            if (report.status == ExecutionReport::Status::Accepted) {
                ++accepted;
            }

            if (report.status == ExecutionReport::Status::Rejected) {
                ++rejected;
            }
        });
    }
};

ApiState state;

http::response<http::string_body>
json_response(
    http::status status,
    const std::string& body
) {
    http::response<http::string_body> res{status, 11};

    res.set(
        http::field::content_type,
        "application/json"
    );

    res.set(
        http::field::access_control_allow_origin,
        "*"
    );

    res.set(
        http::field::access_control_allow_headers,
        "Content-Type"
    );

    res.set(
        http::field::access_control_allow_methods,
        "GET,POST,DELETE,OPTIONS"
    );

    res.body() = body;
    res.prepare_payload();

    return res;
}

std::unordered_map<std::string, std::string>
parse_json_like_body(const std::string& body) {
    std::unordered_map<std::string, std::string> result;

    std::string s = body;

    for (char& c : s) {
        if (
            c == '{' ||
            c == '}' ||
            c == '"' ||
            c == ' '
        ) {
            c = ' ';
        }
    }

    std::stringstream stream(s);
    std::string token;

    while (std::getline(stream, token, ',')) {
        const auto pos = token.find(':');

        if (pos == std::string::npos) {
            continue;
        }

        std::string key = token.substr(0, pos);
        std::string value = token.substr(pos + 1);

        auto trim = [](std::string& value) {
            value.erase(
                value.begin(),
                std::find_if(
                    value.begin(),
                    value.end(),
                    [](unsigned char c) {
                        return !std::isspace(c);
                    }
                )
            );

            value.erase(
                std::find_if(
                    value.rbegin(),
                    value.rend(),
                    [](unsigned char c) {
                        return !std::isspace(c);
                    }
                ).base(),
                value.end()
            );
        };

        trim(key);
        trim(value);

        result[key] = value;
    }

    return result;
}

std::optional<Side>
parse_side(std::string value) {
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char c) {
            return static_cast<char>(
                std::tolower(c)
            );
        }
    );

    if (value == "buy") {
        return Side::Buy;
    }

    if (value == "sell") {
        return Side::Sell;
    }

    return std::nullopt;
}

std::optional<OrderType>
parse_type(std::string value) {
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char c) {
            return static_cast<char>(
                std::tolower(c)
            );
        }
    );

    if (value == "limit") {
        return OrderType::Limit;
    }

    if (value == "market") {
        return OrderType::Market;
    }

    if (value == "ioc") {
        return OrderType::IOC;
    }

    if (value == "fok") {
        return OrderType::FOK;
    }

    if (
        value == "post" ||
        value == "postonly" ||
        value == "post-only"
    ) {
        return OrderType::PostOnly;
    }

    return std::nullopt;
}

std::optional<SymbolId>
symbol_from_target(
    const std::string& target
) {
    const std::string prefix =
        "/api/market/book/";

    if (target.rfind(prefix, 0) != 0) {
        return std::nullopt;
    }

    std::string value =
        target.substr(prefix.size());

    if (
        value == "BTC-USD" ||
        value == "BTCUSD"
    ) {
        return 1;
    }

    try {
        return static_cast<SymbolId>(
            std::stoul(value)
        );
    } catch (...) {
        return std::nullopt;
    }
}

std::string
book_json(SymbolId symbol) {
    const auto* book =
        state.exchange.find_book(symbol);

    std::ostringstream out;

    out << "{"
        << "\"symbol\":" << symbol << ","
        << "\"sequence\":"
        << (book ? book->sequence() : 0)
        << ",\"bids\":[";

    if (book) {
        const auto bids =
            book->depth(Side::Buy, 20);

        for (
            std::size_t i = 0;
            i < bids.size();
            ++i
        ) {
            if (i) {
                out << ",";
            }

            out
                << "{"
                << "\"price\":"
                << bids[i].price
                << ",\"quantity\":"
                << bids[i].quantity
                << ",\"orderCount\":"
                << bids[i].order_count
                << "}";
        }
    }

    out << "],\"asks\":[";

    if (book) {
        const auto asks =
            book->depth(Side::Sell, 20);

        for (
            std::size_t i = 0;
            i < asks.size();
            ++i
        ) {
            if (i) {
                out << ",";
            }

            out
                << "{"
                << "\"price\":"
                << asks[i].price
                << ",\"quantity\":"
                << asks[i].quantity
                << ",\"orderCount\":"
                << asks[i].order_count
                << "}";
        }
    }

    out << "]}";

    return out.str();
}



std::string
execution_algorithm_name(
    ExecutionAlgorithm algorithm
) {
    switch (algorithm) {
        case ExecutionAlgorithm::Market:
            return "MARKET";
        case ExecutionAlgorithm::TWAP:
            return "TWAP";
        case ExecutionAlgorithm::VWAP:
            return "VWAP";
        case ExecutionAlgorithm::POV:
            return "POV";
        case ExecutionAlgorithm::Iceberg:
            return "ICEBERG";
    }

    return "UNKNOWN";
}

std::string
oms_parents_json() {
    const auto parents =
        state.oms.parents();

    std::ostringstream out;
    out << "[";

    for (std::size_t i = 0; i < parents.size(); ++i) {
        if (i) {
            out << ",";
        }

        const auto& parent = parents[i];

        out
            << "{"
            << "\"id\":\"" << parent.id << "\""
            << ",\"symbol\":\"" << parent.symbol << "\""
            << ",\"side\":\""
            << (
                parent.side == RouteSide::Buy
                    ? "BUY"
                    : "SELL"
            )
            << "\""
            << ",\"status\":\""
            << minimatch::to_string(parent.status)
            << "\""
            << ",\"quantity\":"
            << parent.quantity
            << ",\"filledQuantity\":"
            << parent.filled_quantity
            << ",\"remainingQuantity\":"
            << parent.remaining_quantity
            << ",\"strategy\":\""
            << execution_algorithm_name(parent.algorithm)
            << "\""
            << "}";
    }

    out << "]";
    return out.str();
}

std::string
oms_children_json(
    ParentOrderId parent_id
) {
    const auto children =
        state.oms.children_for_parent(
            parent_id
        );

    std::ostringstream out;
    out << "[";

    for (std::size_t i = 0; i < children.size(); ++i) {
        if (i) {
            out << ",";
        }

        const auto& child = children[i];

        out
            << "{"
            << "\"id\":\""
            << child.id
            << "\""
            << ",\"parentId\":\""
            << child.parent_id
            << "\""
            << ",\"venue\":\""
            << child.venue
            << "\""
            << ",\"status\":\""
            << minimatch::to_string(
                   child.status
               )
            << "\""
            << ",\"price\":"
            << child.price
            << ",\"quantity\":"
            << child.quantity
            << ",\"filledQuantity\":"
            << child.filled_quantity
            << ",\"remainingQuantity\":"
            << child.remaining_quantity
            << "}";

    }

    out << "]";
    return out.str();
}

std::string
oms_fills_json(
    ParentOrderId parent_id
) {
    const auto fills =
        state.oms.fills_for_parent(
            parent_id
        );

    std::ostringstream out;
    out << "[";

    for (std::size_t i = 0; i < fills.size(); ++i) {
        if (i) {
            out << ",";
        }

        const auto& fill = fills[i];

        out
            << "{"
            << "\"id\":\""
            << fill.execution_report_id
            << "\""
            << ",\"childOrderId\":\""
            << fill.child_id
            << "\""
            << ",\"parentId\":\""
            << fill.parent_id
            << "\""
            << ",\"venue\":\""
            << fill.venue
            << "\""
            << ",\"price\":"
            << fill.price
            << ",\"quantity\":"
            << fill.quantity
            << ",\"notional\":"
            << fill.notional
            << ",\"fee\":"
            << fill.fee
            << ",\"timestamp\":"
            << fill.timestamp_ns
            << "}";

    }

    out << "]";
    return out.str();
}


std::string execution_database_path() {
    const char* configured =
        std::getenv("EXECUTION_DB_PATH");

    if (
        configured != nullptr &&
        *configured != '\0'
    ) {
        return configured;
    }

    return "data/executions/router_executions.db";
}

std::string route_plan_json(
    const minimatch::RouteRequest& request,
    const minimatch::RoutePlan& plan,
    const std::string& symbol
) {
    std::ostringstream out;

    out << "{"
        << "\"symbol\":\"" << symbol << "\","
        << "\"side\":\""
        << (
            request.side ==
                minimatch::RouteSide::Buy
                ? "BUY"
                : "SELL"
        )
        << "\","
        << "\"requestedQuantity\":"
        << plan.requested_quantity
        << ",\"routedQuantity\":"
        << plan.routed_quantity
        << ",\"complete\":"
        << (
            plan.complete
                ? "true"
                : "false"
        )
        << ",\"averagePrice\":"
        << plan.average_price
        << ",\"estimatedNotional\":"
        << plan.estimated_notional
        << ",\"estimatedFees\":"
        << plan.estimated_fees
        << ",\"legs\":[";

    for (
        std::size_t i = 0;
        i < plan.legs.size();
        ++i
    ) {
        if (i) {
            out << ",";
        }

        const auto& leg =
            plan.legs[i];

        out
            << "{"
            << "\"venue\":\""
            << leg.venue
            << "\""
            << ",\"price\":"
            << leg.price
            << ",\"quantity\":"
            << leg.quantity
            << ",\"estimatedFee\":"
            << leg.estimated_fee
            << ",\"effectivePrice\":"
            << leg.effective_price
            << ",\"latencyMs\":"
            << leg.latency_ms
            << ",\"takerFeeBps\":"
            << leg.taker_fee_bps
            << ",\"latencyCostBpsPerMs\":"
            << leg.latency_cost_bps_per_ms
            << ",\"levelIndex\":"
            << leg.level_index
            << "}";
    }

    out << "]}";

    return out.str();
}

std::string routed_execution_json(
    const minimatch::RoutedExecutionSummary& summary,
    const std::string& symbol,
    minimatch::RouteSide side,
    const minimatch::ExecutionSimulationConfig& config,
    std::int64_t execution_id
) {
    std::ostringstream out;

    out
        << "{"
        << "\"executionId\":"
        << execution_id
        << ",\"symbol\":\""
        << symbol
        << "\""
        << ",\"side\":\""
        << (
            side ==
                minimatch::RouteSide::Buy
                ? "BUY"
                : "SELL"
        )
        << "\""
        << ",\"complete\":"
        << (
            summary.complete
                ? "true"
                : "false"
        )
        << ",\"requestedQuantity\":"
        << summary.requested_quantity
        << ",\"filledQuantity\":"
        << summary.filled_quantity
        << ",\"remainingQuantity\":"
        << summary.remaining_quantity
        << ",\"averageFillPrice\":"
        << summary.average_fill_price
        << ",\"totalNotional\":"
        << summary.total_notional
        << ",\"totalFees\":"
        << summary.total_fees
        << ",\"totalLatencyMs\":"
        << summary.total_latency_ms
        << ",\"simulation\":{"
        << "\"fillRatio\":"
        << config.fill_ratio
        << ",\"rejectionProbability\":"
        << config.rejection_probability
        << ",\"baseLatencyMs\":"
        << config.base_latency_ms
        << ",\"latencyJitterMs\":"
        << config.latency_jitter_ms
        << ",\"seed\":"
        << config.seed
        << "}"
        << ",\"children\":[";

    for (
        std::size_t i = 0;
        i < summary.children.size();
        ++i
    ) {
        if (i) {
            out << ",";
        }

        const auto& child =
            summary.children[i];

        out
            << "{"
            << "\"venue\":\""
            << child.venue
            << "\""
            << ",\"levelIndex\":"
            << child.level_index
            << ",\"requestedQuantity\":"
            << child.requested_quantity
            << ",\"filledQuantity\":"
            << child.filled_quantity
            << ",\"remainingQuantity\":"
            << child.remaining_quantity
            << ",\"price\":"
            << child.price
            << ",\"notional\":"
            << child.notional
            << ",\"fee\":"
            << child.fee
            << ",\"latencyMs\":"
            << child.latency_ms
            << ",\"status\":\""
            << minimatch::to_string(
                   child.status
               )
            << "\""
            << "}";
    }

    out << "]}";

    return out.str();
}

std::string stored_execution_json(
    const minimatch::StoredRouteExecution& execution
) {
    std::ostringstream out;

    out
        << "{"
        << "\"executionId\":"
        << execution.execution_id
        << ",\"createdAt\":\""
        << execution.created_at
        << "\""
        << ",\"symbol\":\""
        << execution.symbol
        << "\""
        << ",\"side\":\""
        << execution.side
        << "\""
        << ",\"requestedQuantity\":"
        << execution.requested_quantity
        << ",\"filledQuantity\":"
        << execution.filled_quantity
        << ",\"remainingQuantity\":"
        << execution.remaining_quantity
        << ",\"averageFillPrice\":"
        << execution.average_fill_price
        << ",\"totalNotional\":"
        << execution.total_notional
        << ",\"totalFees\":"
        << execution.total_fees
        << ",\"totalLatencyMs\":"
        << execution.total_latency_ms
        << ",\"complete\":"
        << (
            execution.complete
                ? "true"
                : "false"
        )
        << ",\"children\":[";

    for (
        std::size_t i = 0;
        i < execution.children.size();
        ++i
    ) {
        if (i) {
            out << ",";
        }

        const auto& child =
            execution.children[i];

        out
            << "{"
            << "\"venue\":\""
            << child.venue
            << "\""
            << ",\"levelIndex\":"
            << child.level_index
            << ",\"requestedQuantity\":"
            << child.requested_quantity
            << ",\"filledQuantity\":"
            << child.filled_quantity
            << ",\"remainingQuantity\":"
            << child.remaining_quantity
            << ",\"price\":"
            << child.price
            << ",\"notional\":"
            << child.notional
            << ",\"fee\":"
            << child.fee
            << ",\"latencyMs\":"
            << child.latency_ms
            << ",\"status\":\""
            << minimatch::to_string(
                   child.status
               )
            << "\""
            << "}";
    }

    out << "]}";

    return out.str();
}


std::string fix_database_path() {
    const char* configured =
        std::getenv("FIX_DB_PATH");

    if (
        configured != nullptr &&
        *configured != '\0'
    ) {
        return configured;
    }

    return "data/fix/minimatch_fix.sqlite";
}

std::string fix_session_id() {
    const char* configured =
        std::getenv("FIX_SESSION_ID");

    if (
        configured != nullptr &&
        *configured != '\0'
    ) {
        return configured;
    }

    return "MINIMATCH->CLIENT";
}

std::string json_escape(
    const std::string& value
) {
    std::ostringstream out;

    for (const char c : value) {
        switch (c) {
            case '"':
                out << "\\\"";
                break;

            case '\\':
                out << "\\\\";
                break;

            case '\n':
                out << "\\n";
                break;

            case '\r':
                out << "\\r";
                break;

            case '\t':
                out << "\\t";
                break;

            case '\x01':
                out << "|";
                break;

            default:
                out << c;
                break;
        }
    }

    return out.str();
}

std::string fix_message_json(
    const minimatch::StoredFixMessage& message
) {
    std::ostringstream out;

    out
        << "{"
        << "\"id\":"
        << message.id
        << ",\"sessionId\":\""
        << json_escape(
               message.session_id
           )
        << "\""
        << ",\"direction\":\""
        << json_escape(
               message.direction
           )
        << "\""
        << ",\"sequenceNumber\":"
        << message.sequence_number
        << ",\"messageType\":\""
        << json_escape(
               message.message_type
           )
        << "\""
        << ",\"wireMessage\":\""
        << json_escape(
               message.wire_message
           )
        << "\""
        << ",\"timestampNs\":"
        << message.timestamp_ns
        << "}";

    return out.str();
}

std::string fix_session_json(
    const minimatch::FixSessionSnapshot& snapshot,
    const std::vector<minimatch::StoredFixMessage>& messages
) {
    std::size_t inbound = 0;
    std::size_t outbound = 0;
    std::size_t resend_requests = 0;
    std::size_t sequence_resets = 0;
    std::size_t rejects = 0;
    std::size_t execution_reports = 0;

    int last_inbound_sequence = 0;
    int last_outbound_sequence = 0;

    for (const auto& message : messages) {
        if (message.direction == "IN") {
            ++inbound;
            last_inbound_sequence =
                std::max(
                    last_inbound_sequence,
                    message.sequence_number
                );
        }

        if (message.direction == "OUT") {
            ++outbound;
            last_outbound_sequence =
                std::max(
                    last_outbound_sequence,
                    message.sequence_number
                );
        }

        if (message.message_type == "2") {
            ++resend_requests;
        }

        if (message.message_type == "4") {
            ++sequence_resets;
        }

        if (message.message_type == "3") {
            ++rejects;
        }

        if (message.message_type == "8") {
            ++execution_reports;
        }
    }

    std::ostringstream out;

    out
        << "{"
        << "\"sessionId\":\""
        << json_escape(snapshot.session_id)
        << "\""
        << ",\"nextIncomingSequence\":"
        << snapshot.next_incoming_sequence
        << ",\"nextOutgoingSequence\":"
        << snapshot.next_outgoing_sequence
        << ",\"lastReceivedTimeNs\":"
        << snapshot.last_received_time_ns
        << ",\"lastSentTimeNs\":"
        << snapshot.last_sent_time_ns
        << ",\"messageCount\":"
        << messages.size()
        << ",\"inboundCount\":"
        << inbound
        << ",\"outboundCount\":"
        << outbound
        << ",\"lastInboundSequence\":"
        << last_inbound_sequence
        << ",\"lastOutboundSequence\":"
        << last_outbound_sequence
        << ",\"resendRequestCount\":"
        << resend_requests
        << ",\"sequenceResetCount\":"
        << sequence_resets
        << ",\"rejectCount\":"
        << rejects
        << ",\"executionReportCount\":"
        << execution_reports
        << "}";

    return out.str();
}

http::response<http::string_body>
handle_request(
    const http::request<
        http::string_body
    >& req
) {
    const std::string target =
        std::string(req.target());

    if (
        req.method() ==
        http::verb::options
    ) {
        return json_response(
            http::status::ok,
            "{}"
        );
    }

    if (
        req.method() ==
            http::verb::get &&
        target ==
            "/api/health"
    ) {
        return json_response(
            http::status::ok,
            R"({"status":"ok","engine":"online"})"
        );
    }

    if (
        req.method() ==
            http::verb::get &&
        target ==
            "/api/system"
    ) {
        std::lock_guard<std::mutex>
            lock(state.mutex);

        const auto latency =
            state.latency.snapshot(
                "order_submit"
            );

        std::ostringstream out;

        out
            << "{"
            << "\"submitted\":"
            << state.submitted
            << ",\"accepted\":"
            << state.accepted
            << ",\"rejected\":"
            << state.rejected
            << ",\"trades\":"
            << state.trades
            << ",\"reports\":"
            << state.reports
            << ",\"activeOrders\":"
            << state.exchange.active_orders()
            << ",\"stateHash\":\""
            << state.exchange.state_hash()
            << "\""
            << ",\"p50Ns\":"
            << latency.p50_ns
            << ",\"p95Ns\":"
            << latency.p95_ns
            << ",\"p99Ns\":"
            << latency.p99_ns
            << "}";

        return json_response(
            http::status::ok,
            out.str()
        );
    }

    if (
        req.method() ==
            http::verb::get &&
        target.rfind(
            "/api/market/book/",
            0
        ) == 0
    ) {
        const auto symbol =
            symbol_from_target(target);

        if (!symbol) {
            return json_response(
                http::status::bad_request,
                R"({"error":"invalid symbol"})"
            );
        }

        std::lock_guard<std::mutex>
            lock(state.mutex);

        return json_response(
            http::status::ok,
            book_json(*symbol)
        );
    }


    if (
        req.method() ==
            http::verb::get &&
        target ==
            "/api/trades"
    ) {
        std::lock_guard<std::mutex>
            lock(state.mutex);

        std::ostringstream out;

        out << "[";

        for (
            std::size_t i = 0;
            i < state.trade_feed.size();
            ++i
        ) {
            if (i) {
                out << ",";
            }

            const auto& trade =
                state.trade_feed[i];

            out
                << "{"
                << "\"sequence\":"
                << trade.sequence
                << ",\"makerOrderId\":"
                << trade.maker_order_id
                << ",\"takerOrderId\":"
                << trade.taker_order_id
                << ",\"price\":"
                << trade.price
                << ",\"quantity\":"
                << trade.quantity
                << ",\"side\":\""
                << minimatch::to_string(
                       trade.taker_side
                   )
                << "\""
                << ",\"symbol\":"
                << trade.symbol
                << "}";
        }

        out << "]";

        return json_response(
            http::status::ok,
            out.str()
        );
    }

    if (
        req.method() ==
            http::verb::get &&
        target ==
            "/api/reports"
    ) {
        std::lock_guard<std::mutex>
            lock(state.mutex);

        std::ostringstream out;

        out << "[";

        for (
            std::size_t i = 0;
            i < state.report_feed.size();
            ++i
        ) {
            if (i) {
                out << ",";
            }

            const auto& report =
                state.report_feed[i];

            out
                << "{"
                << "\"sequence\":"
                << report.sequence
                << ",\"orderId\":"
                << report.order_id
                << ",\"remaining\":"
                << report.remaining
                << ",\"symbol\":"
                << report.symbol
                << ",\"status\":"
                << static_cast<int>(
                       report.status
                   )
                << ",\"rejectReason\":"
                << static_cast<int>(
                       report.reject_reason
                   )
                << "}";
        }

        out << "]";

        return json_response(
            http::status::ok,
            out.str()
        );
    }

    if (
        req.method() ==
            http::verb::get &&
        target.rfind(
            "/api/risk/",
            0
        ) == 0
    ) {
        try {
            const auto suffix =
                target.substr(
                    std::string(
                        "/api/risk/"
                    ).size()
                );

            const auto slash =
                suffix.find('/');

            if (
                slash ==
                std::string::npos
            ) {
                throw std::runtime_error(
                    "expected /api/risk/{client}/{symbol}"
                );
            }

            const auto client =
                static_cast<ClientId>(
                    std::stoul(
                        suffix.substr(
                            0,
                            slash
                        )
                    )
                );

            const auto symbol =
                static_cast<SymbolId>(
                    std::stoul(
                        suffix.substr(
                            slash + 1
                        )
                    )
                );

            std::lock_guard<std::mutex>
                lock(state.mutex);

            const auto risk =
                state.risk.state(
                    client,
                    symbol
                );

            std::ostringstream out;

            out
                << "{"
                << "\"clientId\":"
                << client
                << ",\"symbol\":"
                << symbol
                << ",\"position\":"
                << risk.position
                << ",\"openNotional\":"
                << risk.open_notional
                << ",\"realizedPnl\":"
                << risk.realized_pnl
                << ",\"killed\":"
                << (
                    risk.killed
                        ? "true"
                        : "false"
                )
                << "}";

            return json_response(
                http::status::ok,
                out.str()
            );

        } catch (
            const std::exception& ex
        ) {
            return json_response(
                http::status::bad_request,
                std::string(
                    "{\"error\":\""
                ) +
                    ex.what() +
                    "\"}"
            );
        }
    }

    if (
        req.method() ==
            http::verb::post &&
        target ==
            "/api/cancel"
    ) {
        try {
            const auto fields =
                parse_json_like_body(
                    req.body()
                );

            CancelRequest request{
                std::stoull(
                    fields.at("orderId")
                ),
                static_cast<ClientId>(
                    std::stoul(
                        fields.at(
                            "clientId"
                        )
                    )
                ),
                static_cast<SymbolId>(
                    std::stoul(
                        fields.at(
                            "symbol"
                        )
                    )
                )
            };

            std::lock_guard<std::mutex>
                lock(state.mutex);

            const bool ok =
                state.exchange.cancel(
                    request
                );

            std::ostringstream out;

            out
                << "{"
                << "\"ok\":"
                << (
                    ok
                        ? "true"
                        : "false"
                )
                << ",\"activeOrders\":"
                << state.exchange
                       .active_orders()
                << ",\"stateHash\":\""
                << state.exchange
                       .state_hash()
                << "\""
                << "}";

            return json_response(
                ok
                    ? http::status::ok
                    : http::status::
                          unprocessable_entity,
                out.str()
            );

        } catch (
            const std::exception& ex
        ) {
            return json_response(
                http::status::bad_request,
                std::string(
                    "{\"error\":\""
                ) +
                    ex.what() +
                    "\"}"
            );
        }
    }

    if (
        req.method() ==
            http::verb::post &&
        target ==
            "/api/replace"
    ) {
        try {
            const auto fields =
                parse_json_like_body(
                    req.body()
                );

            ReplaceRequest request{
                std::stoull(
                    fields.at("orderId")
                ),
                static_cast<ClientId>(
                    std::stoul(
                        fields.at(
                            "clientId"
                        )
                    )
                ),
                std::stoll(
                    fields.at(
                        "price"
                    )
                ),
                static_cast<Quantity>(
                    std::stoul(
                        fields.at(
                            "quantity"
                        )
                    )
                ),
                static_cast<SymbolId>(
                    std::stoul(
                        fields.at(
                            "symbol"
                        )
                    )
                )
            };

            std::lock_guard<std::mutex>
                lock(state.mutex);

            const bool ok =
                state.exchange.replace(
                    request
                );

            std::ostringstream out;

            out
                << "{"
                << "\"ok\":"
                << (
                    ok
                        ? "true"
                        : "false"
                )
                << ",\"activeOrders\":"
                << state.exchange
                       .active_orders()
                << ",\"stateHash\":\""
                << state.exchange
                       .state_hash()
                << "\""
                << "}";

            return json_response(
                ok
                    ? http::status::ok
                    : http::status::
                          unprocessable_entity,
                out.str()
            );

        } catch (
            const std::exception& ex
        ) {
            return json_response(
                http::status::bad_request,
                std::string(
                    "{\"error\":\""
                ) +
                    ex.what() +
                    "\"}"
            );
        }
    }



    if (
        req.method() ==
            http::verb::post &&
        target ==
            "/api/oms/parents"
    ) {
        try {
            const auto fields =
                parse_json_like_body(
                    req.body()
                );

            const auto side =
                parse_side(
                    fields.at("side")
                );

            if (!side) {
                throw std::runtime_error(
                    "invalid side"
                );
            }

            const auto quantity =
                std::stod(
                    fields.at(
                        "quantity"
                    )
                );

            std::string strategy =
                fields.count(
                    "strategy"
                )
                    ? fields.at(
                          "strategy"
                      )
                    : "MARKET";

            std::transform(
                strategy.begin(),
                strategy.end(),
                strategy.begin(),
                [](unsigned char c) {
                    return static_cast<
                        char
                    >(
                        std::toupper(c)
                    );
                }
            );

            ExecutionAlgorithm algorithm =
                ExecutionAlgorithm::Market;

            if (
                strategy ==
                "TWAP"
            ) {
                algorithm =
                    ExecutionAlgorithm::TWAP;
            } else if (
                strategy ==
                "VWAP"
            ) {
                algorithm =
                    ExecutionAlgorithm::VWAP;
            } else if (
                strategy ==
                "POV"
            ) {
                algorithm =
                    ExecutionAlgorithm::POV;
            } else if (
                strategy ==
                "ICEBERG"
            ) {
                algorithm =
                    ExecutionAlgorithm::Iceberg;
            }

            const auto now =
                minimatch::
                    steady_now_ns();

            std::lock_guard<std::mutex>
                lock(state.mutex);

            const auto parent_id =
                state.oms.create_parent(
                    ParentOrderRequest{
                        fields.at(
                            "symbol"
                        ),
                        *side ==
                                Side::Buy
                            ? RouteSide::Buy
                            : RouteSide::Sell,
                        quantity,
                        algorithm
                    },
                    now
                );

            std::ostringstream out;

            out
                << "{"
                << "\"ok\":true"
                << ",\"parentId\":\""
                << parent_id
                << "\""
                << "}";

            return json_response(
                http::status::
                    created,
                out.str()
            );

        } catch (
            const std::exception& ex
        ) {
            return json_response(
                http::status::
                    bad_request,
                std::string(
                    "{\"error\":\""
                ) +
                    ex.what() +
                    "\"}"
            );
        }
    }




    if (
        req.method() ==
            http::verb::post &&
        target ==
            "/api/oms/children/cancel"
    ) {
        try {
            const auto fields =
                parse_json_like_body(
                    req.body()
                );

            const auto child_id =
                static_cast<
                    minimatch::ChildOrderId
                >(
                    std::stoull(
                        fields.at(
                            "childId"
                        )
                    )
                );

            const auto now =
                minimatch::
                    steady_now_ns();

            std::lock_guard<std::mutex>
                lock(state.mutex);

            const bool ok =
                state.oms.cancel_child(
                    child_id,
                    now
                );

            return json_response(
                ok
                    ? http::status::ok
                    : http::status::
                          unprocessable_entity,
                std::string(
                    "{\"ok\":"
                ) +
                    (
                        ok
                            ? "true"
                            : "false"
                    ) +
                    "}"
            );

        } catch (
            const std::exception& ex
        ) {
            return json_response(
                http::status::bad_request,
                std::string(
                    "{\"error\":\""
                ) +
                    ex.what() +
                    "\"}"
            );
        }
    }

    if (
        req.method() ==
            http::verb::post &&
        target ==
            "/api/oms/fills"
    ) {
        try {
            const auto fields =
                parse_json_like_body(
                    req.body()
                );

            const auto child_id =
                static_cast<
                    minimatch::ChildOrderId
                >(
                    std::stoull(
                        fields.at(
                            "childId"
                        )
                    )
                );

            const auto price =
                std::stod(
                    fields.at("price")
                );

            const auto quantity =
                std::stod(
                    fields.at(
                        "quantity"
                    )
                );

            const auto fee =
                fields.count("fee")
                    ? std::stod(
                          fields.at(
                              "fee"
                          )
                      )
                    : 0.0;

            const auto now =
                minimatch::
                    steady_now_ns();

            std::lock_guard<std::mutex>
                lock(state.mutex);

            const auto report =
                state.oms.record_fill(
                    child_id,
                    price,
                    quantity,
                    fee,
                    now
                );

            std::ostringstream out;

            out
                << "{"
                << "\"ok\":true"
                << ",\"executionReportId\":\""
                << report.execution_report_id
                << "\""
                << ",\"parentId\":\""
                << report.parent_id
                << "\""
                << ",\"childId\":\""
                << report.child_id
                << "\""
                << ",\"quantity\":"
                << report.quantity
                << ",\"price\":"
                << report.price
                << ",\"notional\":"
                << report.notional
                << ",\"fee\":"
                << report.fee
                << "}";

            return json_response(
                http::status::created,
                out.str()
            );

        } catch (
            const std::exception& ex
        ) {
            return json_response(
                http::status::bad_request,
                std::string(
                    "{\"error\":\""
                ) +
                    ex.what() +
                    "\"}"
            );
        }
    }

    if (
        req.method() ==
            http::verb::post &&
        target ==
            "/api/oms/children"
    ) {
        try {
            const auto fields =
                parse_json_like_body(
                    req.body()
                );

            const auto parent_id =
                static_cast<ParentOrderId>(
                    std::stoull(
                        fields.at(
                            "parentId"
                        )
                    )
                );

            const auto venue =
                fields.at("venue");

            const auto price =
                std::stod(
                    fields.at("price")
                );

            const auto quantity =
                std::stod(
                    fields.at(
                        "quantity"
                    )
                );

            const auto now =
                minimatch::
                    steady_now_ns();

            std::lock_guard<std::mutex>
                lock(state.mutex);

            const auto child_id =
                state.oms.create_child(
                    ChildOrderRequest{
                        parent_id,
                        venue,
                        price,
                        quantity,
                        now
                    }
                );

            state.oms.mark_child_working(
                child_id,
                now
            );

            std::ostringstream out;

            out
                << "{"
                << "\"ok\":true"
                << ",\"childId\":\""
                << child_id
                << "\""
                << "}";

            return json_response(
                http::status::created,
                out.str()
            );

        } catch (
            const std::exception& ex
        ) {
            return json_response(
                http::status::bad_request,
                std::string(
                    "{\"error\":\""
                ) +
                    ex.what() +
                    "\"}"
            );
        }
    }

    if (
        req.method() ==
            http::verb::get &&
        target ==
            "/api/oms/parents"
    ) {
        std::lock_guard<std::mutex>
            lock(state.mutex);

        return json_response(
            http::status::ok,
            oms_parents_json()
        );
    }

    if (
        req.method() ==
            http::verb::get &&
        target.rfind(
            "/api/oms/parents/",
            0
        ) == 0
    ) {
        try {
            const std::string prefix =
                "/api/oms/parents/";

            const auto suffix =
                target.substr(
                    prefix.size()
                );

            const auto slash =
                suffix.find('/');

            if (
                slash ==
                std::string::npos
            ) {
                return json_response(
                    http::status::
                        bad_request,
                    R"({"error":"expected children or fills"})"
                );
            }

            const auto parent_id =
                static_cast<
                    ParentOrderId
                >(
                    std::stoull(
                        suffix.substr(
                            0,
                            slash
                        )
                    )
                );

            const auto resource =
                suffix.substr(
                    slash + 1
                );

            std::lock_guard<std::mutex>
                lock(state.mutex);

            if (
                resource ==
                "children"
            ) {
                return json_response(
                    http::status::ok,
                    oms_children_json(
                        parent_id
                    )
                );
            }

            if (
                resource ==
                "fills"
            ) {
                return json_response(
                    http::status::ok,
                    oms_fills_json(
                        parent_id
                    )
                );
            }

            return json_response(
                http::status::
                    not_found,
                R"({"error":"unknown OMS resource"})"
            );

        } catch (
            const std::exception& ex
        ) {
            return json_response(
                http::status::
                    bad_request,
                std::string(
                    "{\"error\":\""
                ) +
                    ex.what() +
                    "\"}"
            );
        }
    }


    if (
        req.method() ==
            http::verb::post &&
        target ==
            "/api/backtest"
    ) {
        try {
            const auto fields =
                parse_json_like_body(
                    req.body()
                );

            const std::string symbol =
                fields.at("symbol");

            std::string side_text =
                fields.at("side");

            std::transform(
                side_text.begin(),
                side_text.end(),
                side_text.begin(),
                [](unsigned char c) {
                    return static_cast<char>(
                        std::toupper(c)
                    );
                }
            );

            const RouteSide route_side =
                side_text == "SELL"
                    ? RouteSide::Sell
                    : RouteSide::Buy;

            const double quantity =
                std::stod(
                    fields.at("quantity")
                );

            std::string algorithm_text =
                fields.count("algorithm")
                    ? fields.at("algorithm")
                    : "MARKET";

            std::transform(
                algorithm_text.begin(),
                algorithm_text.end(),
                algorithm_text.begin(),
                [](unsigned char c) {
                    return static_cast<char>(
                        std::toupper(c)
                    );
                }
            );

            ExecutionAlgorithm algorithm =
                ExecutionAlgorithm::Market;

            if (algorithm_text == "TWAP") {
                algorithm =
                    ExecutionAlgorithm::TWAP;
            } else if (
                algorithm_text == "VWAP"
            ) {
                algorithm =
                    ExecutionAlgorithm::VWAP;
            } else if (
                algorithm_text == "POV"
            ) {
                algorithm =
                    ExecutionAlgorithm::POV;
            } else if (
                algorithm_text == "ICEBERG"
            ) {
                algorithm =
                    ExecutionAlgorithm::Iceberg;
            }

            const int slices =
                fields.count("slices")
                    ? std::stoi(
                          fields.at("slices")
                      )
                    : 1;

            const double duration_seconds =
                fields.count(
                    "durationSeconds"
                )
                    ? std::stod(
                          fields.at(
                              "durationSeconds"
                          )
                      )
                    : 0.0;

            const double participation_rate =
                fields.count(
                    "participationRate"
                )
                    ? std::stod(
                          fields.at(
                              "participationRate"
                          )
                      )
                    : 0.10;

            const double displayed_quantity =
                fields.count(
                    "displayedQuantity"
                )
                    ? std::stod(
                          fields.at(
                              "displayedQuantity"
                          )
                      )
                    : 0.0;

            const double taker_fee_bps =
                fields.count(
                    "takerFeeBps"
                )
                    ? std::stod(
                          fields.at(
                              "takerFeeBps"
                          )
                      )
                    : 0.0;

            const auto replay =
                minimatch::MarketReplay::from_csv(
                    "data/replay/sample_market.csv"
                );

            minimatch::ExecutionScheduleRequest
                schedule;

            schedule.algorithm =
                algorithm;

            schedule.quantity =
                quantity;

            schedule.slices =
                slices;

            schedule.duration_seconds =
                duration_seconds;

            schedule.participation_rate =
                participation_rate;

            schedule.displayed_quantity =
                displayed_quantity;

            /*
             * VWAP and POV require a volume profile.
             * Start with an equal synthetic profile.
             * Later we will derive this from replay data.
             */
            if (
                algorithm ==
                    ExecutionAlgorithm::VWAP ||
                algorithm ==
                    ExecutionAlgorithm::POV
            ) {
                schedule.volume_profile =
                    std::vector<double>(
                        static_cast<std::size_t>(
                            slices
                        ),
                        100.0
                    );
            }

            const minimatch::BacktestRequest
                backtest_request{
                    .symbol = symbol,
                    .side = route_side,
                    .quantity = quantity,
                    .schedule = schedule,
                    .taker_fee_bps =
                        taker_fee_bps
                };

            std::lock_guard<std::mutex>
                lock(state.mutex);

            const auto result =
                minimatch::
                    run_historical_backtest(
                        backtest_request,
                        replay,
                        &state.oms
                    );

            std::ostringstream out;

            out
                << "{"
                << "\"ok\":true"
                << ",\"complete\":"
                << (
                    result.complete
                        ? "true"
                        : "false"
                )
                << ",\"parentOrderId\":\""
                << result.parent_order_id
                << "\""
                << ",\"requestedQuantity\":"
                << result.requested_quantity
                << ",\"filledQuantity\":"
                << result.filled_quantity
                << ",\"remainingQuantity\":"
                << result.remaining_quantity
                << ",\"arrivalPrice\":"
                << result.arrival_price
                << ",\"averageFillPrice\":"
                << result.average_fill_price
                << ",\"marketVwap\":"
                << result.market_vwap
                << ",\"implementationShortfallBps\":"
                << result
                       .implementation_shortfall_bps
                << ",\"vwapSlippageBps\":"
                << result.vwap_slippage_bps
                << ",\"totalNotional\":"
                << result.total_notional
                << ",\"totalFees\":"
                << result.total_fees
                << ",\"acceptedChildren\":"
                << result.accepted_child_count
                << ",\"rejectedChildren\":"
                << result.rejected_child_count
                << ",\"fillCount\":"
                << result.fills.size()
                << "}";

            return json_response(
                http::status::ok,
                out.str()
            );

        } catch (
            const std::exception& ex
        ) {
            return json_response(
                http::status::bad_request,
                std::string(
                    "{\"error\":\""
                ) +
                    ex.what() +
                    "\"}"
            );
        }
    }


    if (
        req.method() ==
            http::verb::post &&
        target ==
            "/api/router/preview"
    ) {
        try {
            const auto fields =
                parse_json_like_body(
                    req.body()
                );

            const std::string symbol =
                fields.count("symbol")
                    ? fields.at("symbol")
                    : "btcusd";

            std::string side_text =
                fields.at("side");

            std::transform(
                side_text.begin(),
                side_text.end(),
                side_text.begin(),
                [](unsigned char c) {
                    return static_cast<char>(
                        std::toupper(c)
                    );
                }
            );

            const auto side =
                side_text == "SELL"
                    ? minimatch::RouteSide::Sell
                    : minimatch::RouteSide::Buy;

            minimatch::RouteRequest request;

            request.side = side;
            request.quantity =
                std::stod(
                    fields.at("quantity")
                );

            request.limit_price =
                fields.count("limitPrice")
                    ? std::stod(
                          fields.at("limitPrice")
                      )
                    : 0.0;

            request.max_slippage_bps =
                fields.count("maxSlippageBps")
                    ? std::stod(
                          fields.at(
                              "maxSlippageBps"
                          )
                      )
                    : 10000.0;

            request.max_venue_count =
                fields.count("maxVenueCount")
                    ? static_cast<std::size_t>(
                          std::stoull(
                              fields.at(
                                  "maxVenueCount"
                              )
                          )
                      )
                    : std::numeric_limits<
                          std::size_t
                      >::max();

            request.all_or_none =
                fields.count("allOrNone")
                    ? (
                          fields.at("allOrNone") ==
                              "true" ||
                          fields.at("allOrNone") ==
                              "1"
                      )
                    : false;

            const auto quotes =
                minimatch::
                    read_live_router_quotes(
                        symbol
                    );

            const auto plan =
                minimatch::
                    build_route_plan(
                        request,
                        quotes
                    );

            return json_response(
                http::status::ok,
                route_plan_json(
                    request,
                    plan,
                    symbol
                )
            );

        } catch (
            const std::exception& ex
        ) {
            return json_response(
                http::status::bad_request,
                std::string(
                    "{\"error\":\""
                ) +
                    ex.what() +
                    "\"}"
            );
        }
    }

    if (
        req.method() ==
            http::verb::post &&
        target ==
            "/api/executions"
    ) {
        try {
            const auto fields =
                parse_json_like_body(
                    req.body()
                );

            const std::string symbol =
                fields.count("symbol")
                    ? fields.at("symbol")
                    : "btcusd";

            std::string side_text =
                fields.at("side");

            std::transform(
                side_text.begin(),
                side_text.end(),
                side_text.begin(),
                [](unsigned char c) {
                    return static_cast<char>(
                        std::toupper(c)
                    );
                }
            );

            const auto side =
                side_text == "SELL"
                    ? minimatch::RouteSide::Sell
                    : minimatch::RouteSide::Buy;

            minimatch::RouteRequest route_request;

            route_request.side = side;
            route_request.quantity =
                std::stod(
                    fields.at("quantity")
                );

            route_request.limit_price =
                fields.count("limitPrice")
                    ? std::stod(
                          fields.at("limitPrice")
                      )
                    : 0.0;

            route_request.max_slippage_bps =
                fields.count("maxSlippageBps")
                    ? std::stod(
                          fields.at(
                              "maxSlippageBps"
                          )
                      )
                    : 10000.0;

            route_request.max_venue_count =
                fields.count("maxVenueCount")
                    ? static_cast<std::size_t>(
                          std::stoull(
                              fields.at(
                                  "maxVenueCount"
                              )
                          )
                      )
                    : std::numeric_limits<
                          std::size_t
                      >::max();

            route_request.all_or_none =
                fields.count("allOrNone")
                    ? (
                          fields.at("allOrNone") ==
                              "true" ||
                          fields.at("allOrNone") ==
                              "1"
                      )
                    : false;

            minimatch::ExecutionSimulationConfig
                config;

            config.fill_ratio =
                fields.count("fillRatio")
                    ? std::stod(
                          fields.at("fillRatio")
                      )
                    : 1.0;

            config.rejection_probability =
                fields.count(
                    "rejectionProbability"
                )
                    ? std::stod(
                          fields.at(
                              "rejectionProbability"
                          )
                      )
                    : 0.0;

            config.base_latency_ms =
                fields.count("baseLatencyMs")
                    ? std::stod(
                          fields.at(
                              "baseLatencyMs"
                          )
                      )
                    : 0.0;

            config.latency_jitter_ms =
                fields.count(
                    "latencyJitterMs"
                )
                    ? std::stod(
                          fields.at(
                              "latencyJitterMs"
                          )
                      )
                    : 0.0;

            config.seed =
                fields.count("seed")
                    ? std::stoull(
                          fields.at("seed")
                      )
                    : 1;

            const auto quotes =
                minimatch::
                    read_live_router_quotes(
                        symbol
                    );

            const auto plan =
                minimatch::
                    build_route_plan(
                        route_request,
                        quotes
                    );

            const auto summary =
                minimatch::
                    simulate_route_execution(
                        plan,
                        config
                    );

            static minimatch::ExecutionStore
                execution_store(
                    execution_database_path()
                );

            const auto execution_id =
                execution_store.save(
                    symbol,
                    side,
                    config,
                    summary
                );

            return json_response(
                http::status::ok,
                routed_execution_json(
                    summary,
                    symbol,
                    side,
                    config,
                    execution_id
                )
            );

        } catch (
            const std::exception& ex
        ) {
            return json_response(
                http::status::bad_request,
                std::string(
                    "{\"error\":\""
                ) +
                    ex.what() +
                    "\"}"
            );
        }
    }

    if (
        req.method() ==
            http::verb::get &&
        target ==
            "/api/executions"
    ) {
        static minimatch::ExecutionStore
            execution_store(
                execution_database_path()
            );

        const auto executions =
            execution_store.recent(50);

        std::ostringstream out;
        out << "[";

        for (
            std::size_t i = 0;
            i < executions.size();
            ++i
        ) {
            if (i) {
                out << ",";
            }

            out << stored_execution_json(
                executions[i]
            );
        }

        out << "]";

        return json_response(
            http::status::ok,
            out.str()
        );
    }

    if (
        req.method() ==
            http::verb::get &&
        target.rfind(
            "/api/executions/",
            0
        ) == 0
    ) {
        try {
            const auto id =
                std::stoll(
                    target.substr(
                        std::string(
                            "/api/executions/"
                        ).size()
                    )
                );

            static minimatch::ExecutionStore
                execution_store(
                    execution_database_path()
                );

            const auto result =
                execution_store.find(id);

            if (!result) {
                return json_response(
                    http::status::not_found,
                    R"({"error":"execution not found"})"
                );
            }

            return json_response(
                http::status::ok,
                stored_execution_json(
                    *result
                )
            );

        } catch (
            const std::exception& ex
        ) {
            return json_response(
                http::status::bad_request,
                std::string(
                    "{\"error\":\""
                ) +
                    ex.what() +
                    "\"}"
            );
        }
    }


    if (
        req.method() ==
            http::verb::get &&
        target ==
            "/api/fix/session"
    ) {
        try {
            minimatch::FixStore store(
                fix_database_path()
            );

            const std::string session_id =
                fix_session_id();

            const auto snapshot =
                store.load_session(
                    session_id
                );

            const auto messages =
                store.messages_for_session(
                    session_id
                );

            return json_response(
                http::status::ok,
                fix_session_json(
                    snapshot,
                    messages
                )
            );

        } catch (
            const std::exception& ex
        ) {
            return json_response(
                http::status::
                    internal_server_error,
                std::string(
                    "{\"error\":\""
                ) +
                    json_escape(
                        ex.what()
                    ) +
                    "\"}"
            );
        }
    }

    if (
        req.method() ==
            http::verb::get &&
        target ==
            "/api/fix/messages"
    ) {
        try {
            minimatch::FixStore store(
                fix_database_path()
            );

            const auto messages =
                store.messages_for_session(
                    fix_session_id()
                );

            std::ostringstream out;

            out << "[";

            for (
                std::size_t i = 0;
                i < messages.size();
                ++i
            ) {
                if (i) {
                    out << ",";
                }

                out << fix_message_json(
                    messages[i]
                );
            }

            out << "]";

            return json_response(
                http::status::ok,
                out.str()
            );

        } catch (
            const std::exception& ex
        ) {
            return json_response(
                http::status::
                    internal_server_error,
                std::string(
                    "{\"error\":\""
                ) +
                    json_escape(
                        ex.what()
                    ) +
                    "\"}"
            );
        }
    }

    if (
        req.method() ==
            http::verb::post &&
        target ==
            "/api/orders"
    ) {
        try {
            const auto fields =
                parse_json_like_body(
                    req.body()
                );

            const auto side =
                parse_side(
                    fields.at("side")
                );

            const auto type =
                parse_type(
                    fields.at("type")
                );

            if (!side || !type) {
                return json_response(
                    http::status::
                        bad_request,
                    R"({"error":"invalid side or order type"})"
                );
            }

            OrderRequest order{
                std::stoull(
                    fields.at("orderId")
                ),
                static_cast<ClientId>(
                    std::stoul(
                        fields.at("clientId")
                    )
                ),
                *side,
                *type,
                std::stoll(
                    fields.at("price")
                ),
                static_cast<Quantity>(
                    std::stoul(
                        fields.at(
                            "quantity"
                        )
                    )
                ),
                static_cast<SymbolId>(
                    std::stoul(
                        fields.at(
                            "symbol"
                        )
                    )
                )
            };

            std::lock_guard<std::mutex>
                lock(state.mutex);

            ++state.submitted;

            if (!state.risk.allow(order)) {
                ++state.rejected;

                return json_response(
                    http::status::
                        unprocessable_entity,
                    R"({"ok":false,"status":"RISK_REJECTED"})"
                );
            }

            const auto start =
                minimatch::steady_now_ns();

            const bool ok =
                state.exchange.submit(
                    order
                );

            const auto elapsed =
                minimatch::steady_now_ns()
                - start;

            state.latency.record(
                "order_submit",
                elapsed
            );

            if (ok) {
                state.risk
                    .on_order_accepted(
                        order
                    );

                if (
                    order.type ==
                        OrderType::Limit ||
                    order.type ==
                        OrderType::PostOnly
                ) {
                    state.active_order_metadata[
                        order.order_id
                    ] = order;
                }
            }

            std::ostringstream out;

            out
                << "{"
                << "\"ok\":"
                << (
                    ok
                        ? "true"
                        : "false"
                )
                << ",\"orderId\":"
                << order.order_id
                << ",\"activeOrders\":"
                << state.exchange
                       .active_orders()
                << ",\"stateHash\":\""
                << state.exchange
                       .state_hash()
                << "\""
                << "}";

            return json_response(
                ok
                    ? http::status::ok
                    : http::status::
                          unprocessable_entity,
                out.str()
            );

        } catch (
            const std::exception& ex
        ) {
            return json_response(
                http::status::bad_request,
                std::string(
                    "{\"error\":\""
                ) +
                    ex.what() +
                    "\"}"
            );
        }
    }

    return json_response(
        http::status::not_found,
        R"({"error":"not found"})"
    );
}

} // namespace

int main() {
    try {
        asio::io_context io;

        tcp::acceptor acceptor(
            io,
            {
                asio::ip::make_address(
                    "127.0.0.1"
                ),
                8081
            }
        );

        std::cout
            << "MiniMatch API listening on "
            << "http://127.0.0.1:8081\n";

        while (true) {
            tcp::socket socket(io);

            acceptor.accept(socket);

            beast::flat_buffer buffer;

            http::request<
                http::string_body
            > req;

            http::read(
                socket,
                buffer,
                req
            );

            auto res =
                handle_request(req);

            http::write(
                socket,
                res
            );

            beast::error_code ec;

            socket.shutdown(
                tcp::socket::
                    shutdown_send,
                ec
            );
        }

    } catch (
        const std::exception& ex
    ) {
        std::cerr
            << "API server error: "
            << ex.what()
            << '\n';

        return 1;
    }
}
