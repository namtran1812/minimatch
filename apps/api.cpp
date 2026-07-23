#include "minimatch/reconciliation.hpp"
#include "minimatch/analytics.hpp"
#include "minimatch/quant.hpp"
#include "minimatch/exchange.hpp"
#include "minimatch/fix_store.hpp"
#include "minimatch/latency_stats.hpp"
#include "minimatch/oms.hpp"
#include "minimatch/oms_store.hpp"
#include "minimatch/execution_algo.hpp"
#include "minimatch/market_replay.hpp"
#include "minimatch/replay_control.hpp"
#include "minimatch/backtest_engine.hpp"
#include "minimatch/risk.hpp"
#include "minimatch/router.hpp"
#include "minimatch/execution_engine.hpp"
#include "minimatch/execution_store.hpp"
#include "minimatch/live_router_quotes.hpp"
#include "minimatch/live_algo_executor.hpp"
#include "minimatch/algo_scheduler.hpp"
#include "minimatch/algo_store.hpp"
#include "minimatch/position_manager.hpp"
#include "minimatch/portfolio_risk.hpp"
#include "minimatch/drop_copy.hpp"
#include "minimatch/system_settings_store.hpp"
#include "minimatch/execution_analytics.hpp"
#include "minimatch/midpoint_history.hpp"
#include "minimatch/midpoint_store.hpp"
#include "minimatch/types.hpp"

#include <fstream>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
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
#include <thread>
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

std::string execution_status_string(
    ExecutionReport::Status status
) {
    switch (status) {
        case ExecutionReport::Status::Accepted:
            return "ACCEPTED";

        case ExecutionReport::Status::PartiallyFilled:
            return "PARTIALLY_FILLED";

        case ExecutionReport::Status::Filled:
            return "FILLED";

        case ExecutionReport::Status::Cancelled:
            return "CANCELLED";

        case ExecutionReport::Status::Replaced:
            return "REPLACED";

        case ExecutionReport::Status::Rejected:
            return "REJECTED";

        case ExecutionReport::Status::Expired:
            return "EXPIRED";
    }

    return "UNKNOWN";
}

std::string reject_reason_string(
    minimatch::RejectReason reason
) {
    switch (reason) {
        case minimatch::RejectReason::None:
            return "NONE";

        case minimatch::RejectReason::DuplicateOrderId:
            return "DUPLICATE_ORDER_ID";

        case minimatch::RejectReason::UnknownOrder:
            return "UNKNOWN_ORDER";

        case minimatch::RejectReason::InvalidQuantity:
            return "INVALID_QUANTITY";

        case minimatch::RejectReason::InvalidPrice:
            return "INVALID_PRICE";

        case minimatch::RejectReason::CapacityExceeded:
            return "CAPACITY_EXCEEDED";

        case minimatch::RejectReason::WouldTrade:
            return "WOULD_TRADE";

        case minimatch::RejectReason::InsufficientLiquidity:
            return "INSUFFICIENT_LIQUIDITY";

        case minimatch::RejectReason::ClientMismatch:
            return "CLIENT_MISMATCH";

        case minimatch::RejectReason::InvalidReplacement:
            return "INVALID_REPLACEMENT";

        case minimatch::RejectReason::TradingHalted:
            return "TRADING_HALTED";

        case minimatch::RejectReason::PriceBandViolation:
            return "PRICE_BAND_VIOLATION";
    }

    return "UNKNOWN";
}

std::string stp_policy_string(
    minimatch::
        SelfTradePreventionPolicy policy
) {
    switch (policy) {
        case minimatch::
            SelfTradePreventionPolicy::
                CancelNewest:
            return "CANCEL_NEWEST";

        case minimatch::
            SelfTradePreventionPolicy::
                CancelOldest:
            return "CANCEL_OLDEST";

        case minimatch::
            SelfTradePreventionPolicy::
                CancelBoth:
            return "CANCEL_BOTH";

        case minimatch::
            SelfTradePreventionPolicy::
                None:
            return "NONE";
    }

    return "NONE";
}

struct VolatilityCircuitBreakerConfig {
    bool enabled{
        false
    };

    double threshold_percent{
        5.0
    };

    std::uint64_t window_ns{
        30'000'000'000ULL
    };
};

struct ApiState {
    Exchange exchange{100'000};
    RiskEngine risk;
    LatencyStats latency;
    minimatch::ReplayController replay;
    OrderManagementSystem oms;
    minimatch::OmsStore oms_store{
        "data/oms/minimatch_oms.sqlite"
    };

    minimatch::AlgoScheduler algo_scheduler;

    minimatch::AlgoStore algo_store{
        "data/algo/minimatch_algo.sqlite"
    };
    minimatch::PositionManager positions;
    minimatch::PortfolioRiskManager portfolio_risk;
    minimatch::DropCopyStore drop_copy{
        "data/drop_copy/minimatch_drop_copy.sqlite"
    };

    minimatch::SystemSettingsStore settings_store{
        "data/settings/minimatch_settings.sqlite"
    };
    minimatch::ExecutionAnalytics execution_analytics;

    minimatch::MidpointHistory midpoint_history;

    minimatch::MidpointStore midpoint_store{
        "data/market/minimatch_midpoints.sqlite"
    };

    VolatilityCircuitBreakerConfig
        volatility_breaker;

    std::atomic<bool> market_sampler_running{
        true
    };

    std::thread market_sampler;

    std::uint64_t submitted{0};
    std::uint64_t accepted{0};
    std::uint64_t rejected{0};
    std::uint64_t trades{0};
    std::uint64_t reports{0};

    std::deque<Trade> trade_feed;
    std::deque<ExecutionReport> report_feed;

    std::unordered_map<OrderId, OrderRequest> active_order_metadata;

    std::mutex mutex;

    ApiState()
        : algo_scheduler(
              oms,
              [](
                  const std::string& symbol
              ) {
                  return minimatch::
                      read_live_router_quotes(
                          symbol
                      );
              },
              []() {
                  return static_cast<
                      std::uint64_t
                  >(
                      std::chrono::
                          duration_cast<
                              std::chrono::
                                  nanoseconds
                          >(
                              std::chrono::
                                  system_clock::
                                      now()
                                          .time_since_epoch()
                          )
                          .count()
                  );
              }
          ) {
        const auto midpoint_now =
            static_cast<
                std::uint64_t
            >(
                std::chrono::
                    duration_cast<
                        std::chrono::
                            nanoseconds
                    >(
                        std::chrono::
                            system_clock::
                                now()
                            .time_since_epoch()
                    )
                    .count()
            );

        constexpr std::uint64_t
            midpoint_retention_ns =
                300'000'000'000ULL;

        const auto restored_midpoints =
            midpoint_store.load_since(
                "btcusd",
                midpoint_now >
                        midpoint_retention_ns
                    ? midpoint_now -
                          midpoint_retention_ns
                    : 0
            );

        for (
            const auto& observation :
            restored_midpoints
        ) {
            midpoint_history.record(
                "btcusd",
                observation.timestamp_ns,
                observation.midpoint
            );
        }

        const auto recovered =
            oms_store.load();

        oms.restore(
            recovered.parents,
            recovered.children,
            recovered.fills
        );

        const auto recovered_algos =
            algo_store.load_recoverable();

        std::vector<
            minimatch::AlgoOrderState
        > recovered_algo_states;

        recovered_algo_states.reserve(
            recovered_algos.size()
        );

        for (
            const auto& recovered_algo :
            recovered_algos
        ) {
            recovered_algo_states.push_back(
                recovered_algo.state
            );
        }

        portfolio_risk.set_limits(
            settings_store
                .load_portfolio_limits()
        );

        const auto saved_breaker =
            settings_store
                .load_circuit_breaker();

        volatility_breaker.enabled =
            saved_breaker.enabled;

        volatility_breaker
            .threshold_percent =
            saved_breaker
                .threshold_percent;

        volatility_breaker.window_ns =
            saved_breaker.window_ns;

        algo_scheduler.set_state_callback(
            [this](
                const minimatch::
                    AlgoOrderState& state
            ) {
                algo_store.save(
                    state
                );
            }
        );

        algo_scheduler.restore_recoverable(
            recovered_algos
        );


        for (const auto& fill : recovered.fills) {
            const auto parent =
                oms.find_parent(
                    fill.parent_id
                );

            if (parent) {
                positions.apply_fill(
                    *parent,
                    fill
                );
            }
        }

        oms.set_mutation_callback(
            [this](
                const OrderManagementSystem::
                    MutationEvent& event
            ) {
                if (event.parent) {
                    oms_store.save_parent(
                        *event.parent
                    );
                }

                if (event.child) {
                    oms_store.save_child(
                        *event.child
                    );
                }

                if (event.fill) {
                    oms_store.save_fill(
                        *event.fill
                    );

                    drop_copy.publish(
                        minimatch::DropCopyEvent{
                            .timestamp_ns =
                                event.fill->timestamp_ns,
                            .order_id =
                                static_cast<OrderId>(
                                    event.fill->child_id
                                ),
                            .symbol =
                                minimatch::DEFAULT_SYMBOL,
                            .event_type =
                                "OMS_FILL",
                            .status =
                                "FILLED",
                            .remaining =
                                0,
                            .reject_reason =
                                minimatch::RejectReason::None
                        }
                    );

                    if (event.parent) {
                        positions.apply_fill(
                            *event.parent,
                            *event.fill
                        );

                        const auto portfolio_status =
                            portfolio_risk.evaluate(
                                positions.positions()
                            );

                        if (
                            portfolio_status.breached
                        ) {
                            exchange.halt_all();
                        }
                    }
                }

                if (
                    event.type ==
                        OrderManagementSystem::
                            MutationType::
                                ParentUpdated &&
                    event.parent
                ) {
                    const auto children =
                        oms.children_for_parent(
                            event.parent->id
                        );

                    for (
                        const auto& child :
                        children
                    ) {
                        oms_store.save_child(
                            child
                        );
                    }
                }
            }
        );

        exchange.on_trade([this](const Trade& trade) {
            std::lock_guard<std::mutex> lock(mutex);

            ++trades;

            trade_feed.push_front(trade);

            while (trade_feed.size() > 100) {
                trade_feed.pop_back();
            }
        });

        exchange.on_report([this](const ExecutionReport& report) {

            const auto drop_copy_now =
                static_cast<
                    std::uint64_t
                >(
                    std::chrono::
                        duration_cast<
                            std::chrono::
                                nanoseconds
                        >(
                            std::chrono::
                                system_clock::
                                    now()
                                .time_since_epoch()
                        )
                        .count()
                );

            drop_copy.publish(
                minimatch::DropCopyEvent{
                    .timestamp_ns =
                        drop_copy_now,
                    .order_id =
                        report.order_id,
                    .symbol =
                        report.symbol,
                    .event_type =
                        "EXECUTION_REPORT",
                    .status =
                        execution_status_string(
                            report.status
                        ),
                    .remaining =
                        report.remaining,
                    .reject_reason =
                        report.reject_reason
                }
            );

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
    

        market_sampler =
            std::thread(
                [this]() {
                    while (
                        market_sampler_running.load()
                    ) {
                        try {
                            const auto quotes =
                                minimatch::
                                    read_live_router_quotes(
                                        "btcusd"
                                    );

                            double best_bid = 0.0;
                            double best_ask = 0.0;

                            for (
                                const auto& quote :
                                quotes
                            ) {
                                if (
                                    quote.bids.empty() ||
                                    quote.asks.empty()
                                ) {
                                    continue;
                                }

                                const double bid =
                                    quote.bids
                                        .front()
                                        .price;

                                const double ask =
                                    quote.asks
                                        .front()
                                        .price;

                                if (
                                    best_bid == 0.0 ||
                                    bid > best_bid
                                ) {
                                    best_bid = bid;
                                }

                                if (
                                    best_ask == 0.0 ||
                                    ask < best_ask
                                ) {
                                    best_ask = ask;
                                }
                            }

                            if (
                                best_bid > 0.0 &&
                                best_ask > 0.0
                            ) {
                                const auto now =
                                    static_cast<
                                        std::uint64_t
                                    >(
                                        std::chrono::
                                            duration_cast<
                                                std::chrono::
                                                    nanoseconds
                                            >(
                                                std::chrono::
                                                    system_clock::
                                                        now()
                                                            .time_since_epoch()
                                            )
                                            .count()
                                    );

                                const double midpoint =
                                    (
                                        best_bid +
                                        best_ask
                                    ) /
                                        2.0;

                                midpoint_history.record(
                                    "btcusd",
                                    now,
                                    midpoint
                                );

                                midpoint_store.save(
                                    "btcusd",
                                    now,
                                    midpoint
                                );

                                static std::uint64_t
                                    last_midpoint_prune_ns =
                                        0;

                                if (
                                    last_midpoint_prune_ns ==
                                        0 ||
                                    now -
                                            last_midpoint_prune_ns >=
                                        60'000'000'000ULL
                                ) {
                                    const auto cutoff =
                                        now >
                                                300'000'000'000ULL
                                            ? now -
                                                  300'000'000'000ULL
                                            : 0;

                                    midpoint_store
                                        .prune_before(
                                            cutoff
                                        );

                                    last_midpoint_prune_ns =
                                        now;
                                }

                                if (
                                    volatility_breaker
                                        .enabled &&
                                    now >
                                        volatility_breaker
                                            .window_ns
                                ) {
                                    const auto previous =
                                        midpoint_history
                                            .at_or_before(
                                                "btcusd",
                                                now -
                                                    volatility_breaker
                                                        .window_ns
                                            );

                                    if (
                                        previous &&
                                        previous->midpoint >
                                            0.0
                                    ) {
                                        const double move =
                                            std::abs(
                                                midpoint -
                                                previous
                                                    ->midpoint
                                            ) /
                                            previous
                                                ->midpoint *
                                            100.0;

                                        if (
                                            move >=
                                            volatility_breaker
                                                .threshold_percent
                                        ) {
                                            exchange
                                                .halt_symbol(
                                                    1
                                                );
                                        }
                                    }
                                }
                            }

                        } catch (...) {
                        }

                        std::this_thread::
                            sleep_for(
                                std::chrono::
                                    milliseconds(
                                        100
                                    )
                            );
                    }
                }
            );
}

    ~ApiState() {
        market_sampler_running.store(
            false
        );

        if (
            market_sampler.joinable()
        ) {
            market_sampler.join();
        }
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

http::response<http::string_body>
text_response(
    http::status status,
    const std::string& body,
    const std::string& content_type =
        "text/plain; version=0.0.4"
) {
    http::response<http::string_body> res{
        status,
        11
    };

    res.set(
        http::field::content_type,
        content_type
    );

    res.set(
        http::field::access_control_allow_origin,
        "*"
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

enum class ReconciliationStatus {
    Consistent,
    LegacyUnverifiable,
    Inconsistent
};

const char*
reconciliation_status_string(
    ReconciliationStatus status
) {
    switch (status) {
        case ReconciliationStatus::
            Consistent:
            return "CONSISTENT";

        case ReconciliationStatus::
            LegacyUnverifiable:
            return "LEGACY_UNVERIFIABLE";

        case ReconciliationStatus::
            Inconsistent:
            return "INCONSISTENT";
    }

    return "INCONSISTENT";
}


ReconciliationStatus
parent_reconciliation_status(
    ParentOrderId parent_id
) {
    const auto parent =
        state.oms.find_parent(
            parent_id
        );

    if (!parent) {
        return ReconciliationStatus::
            Inconsistent;
    }

    const auto fills =
        state.oms.fills_for_parent(
            parent_id
        );

    const auto children =
        state.oms.children_for_parent(
            parent_id
        );

    double fill_quantity =
        0.0;

    double child_fill_quantity =
        0.0;

    double fill_notional =
        0.0;

    double total_fees =
        0.0;

    std::size_t drop_copy_fill_count =
        0;

    for (
        const auto& fill :
        fills
    ) {
        fill_quantity +=
            fill.quantity;

        fill_notional +=
            fill.notional;

        total_fees +=
            fill.fee;
    }

    for (
        const auto& child :
        children
    ) {
        child_fill_quantity +=
            child.filled_quantity;

        const auto child_events =
            state.drop_copy.events_for_order(
                static_cast<OrderId>(
                    child.id
                )
            );

        for (
            const auto& event :
            child_events
        ) {
            if (
                event.event_type ==
                    "OMS_FILL"
            ) {
                ++drop_copy_fill_count;
                break;
            }
        }
    }

    const auto algo_state =
        state.algo_scheduler.find(
            parent_id
        );

    const double arrival_price =
        algo_state
            ? algo_state->arrival_price
            : 0.0;

    const auto quality =
        state.execution_analytics.analyze(
            *parent,
            fills,
            arrival_price
        );

    constexpr double epsilon =
        1e-9;

    const bool non_drop_copy_consistent =
        std::abs(
            parent->filled_quantity -
            fill_quantity
        ) <= epsilon &&
        std::abs(
            parent->filled_quantity -
            child_fill_quantity
        ) <= epsilon &&
        std::abs(
            quality.filled_quantity -
            fill_quantity
        ) <= epsilon &&
        std::abs(
            quality.total_notional -
            fill_notional
        ) <= epsilon &&
        std::abs(
            quality.total_fees -
            total_fees
        ) <= epsilon;

    if (!non_drop_copy_consistent) {
        return ReconciliationStatus::
            Inconsistent;
    }

    if (
        drop_copy_fill_count ==
        fills.size()
    ) {
        return ReconciliationStatus::
            Consistent;
    }

    if (
        !fills.empty() &&
        drop_copy_fill_count == 0
    ) {
        return ReconciliationStatus::
            LegacyUnverifiable;
    }

    return ReconciliationStatus::
        Inconsistent;
}


bool
parent_reconciliation_consistent(
    ParentOrderId parent_id
) {
    const auto parent =
        state.oms.find_parent(
            parent_id
        );

    if (!parent) {
        return false;
    }

    const auto fills =
        state.oms.fills_for_parent(
            parent_id
        );

    const auto children =
        state.oms.children_for_parent(
            parent_id
        );

    double fill_quantity =
        0.0;

    double child_fill_quantity =
        0.0;

    double fill_notional =
        0.0;

    double total_fees =
        0.0;

    std::size_t drop_copy_fill_count =
        0;

    for (
        const auto& fill :
        fills
    ) {
        fill_quantity +=
            fill.quantity;

        fill_notional +=
            fill.notional;

        total_fees +=
            fill.fee;
    }

    for (
        const auto& child :
        children
    ) {
        child_fill_quantity +=
            child.filled_quantity;

        const auto child_events =
            state.drop_copy.events_for_order(
                static_cast<OrderId>(
                    child.id
                )
            );

        for (
            const auto& event :
            child_events
        ) {
            if (
                event.event_type ==
                    "OMS_FILL"
            ) {
                ++drop_copy_fill_count;
                break;
            }
        }
    }

    const auto algo_state =
        state.algo_scheduler.find(
            parent_id
        );

    const double arrival_price =
        algo_state
            ? algo_state->arrival_price
            : 0.0;

    const auto quality =
        state.execution_analytics.analyze(
            *parent,
            fills,
            arrival_price
        );

    constexpr double epsilon =
        1e-9;

    return
        std::abs(
            parent->filled_quantity -
            fill_quantity
        ) <= epsilon &&
        std::abs(
            parent->filled_quantity -
            child_fill_quantity
        ) <= epsilon &&
        std::abs(
            quality.filled_quantity -
            fill_quantity
        ) <= epsilon &&
        std::abs(
            quality.total_notional -
            fill_notional
        ) <= epsilon &&
        std::abs(
            quality.total_fees -
            total_fees
        ) <= epsilon &&
        drop_copy_fill_count ==
            fills.size();
}


std::vector<
    minimatch::PositionReconciliationResult
>
reconcile_positions() {
    const auto parents =
        state.oms.parents();

    std::unordered_map<
        std::string,
        minimatch::PositionReconciliationInput
    > inputs_by_symbol;

    for (
        const auto& parent :
        parents
    ) {
        auto& input =
            inputs_by_symbol[
                parent.symbol
            ];

        input.symbol =
            parent.symbol;

        const auto fills =
            state.oms.fills_for_parent(
                parent.id
            );

        for (
            const auto& fill :
            fills
        ) {
            input.fills.push_back(
                minimatch::
                    PositionFillInput{
                        .side =
                            parent.side,
                        .quantity =
                            fill.quantity,
                        .price =
                            fill.price,
                        .fee =
                            fill.fee
                    }
            );
        }
    }

    std::vector<
        minimatch::PositionReconciliationInput
    > inputs;

    inputs.reserve(
        inputs_by_symbol.size()
    );

    for (
        auto& [symbol, input] :
        inputs_by_symbol
    ) {
        const auto quotes =
            minimatch::
                read_live_router_quotes(
                    symbol
                );

        double best_bid =
            0.0;

        double best_ask =
            0.0;

        for (
            const auto& quote :
            quotes
        ) {
            if (
                !quote.healthy ||
                quote.bids.empty() ||
                quote.asks.empty()
            ) {
                continue;
            }

            const double bid =
                quote.bids
                    .front()
                    .price;

            const double ask =
                quote.asks
                    .front()
                    .price;

            if (
                best_bid == 0.0 ||
                bid > best_bid
            ) {
                best_bid =
                    bid;
            }

            if (
                best_ask == 0.0 ||
                ask < best_ask
            ) {
                best_ask =
                    ask;
            }
        }

        if (
            best_bid > 0.0 &&
            best_ask > 0.0
        ) {
            state.positions.mark(
                symbol,
                (
                    best_bid +
                    best_ask
                ) /
                2.0
            );
        }

        input.actual =
            state.positions.position(
                symbol
            );

        inputs.push_back(
            std::move(input)
        );
    }

    return minimatch::
        reconcile_positions(
            inputs
        );
}


bool
positions_reconciliation_consistent() {
    const auto results =
        reconcile_positions();

    return std::all_of(
        results.begin(),
        results.end(),
        [](
            const auto& result
        ) {
            return result.consistent;
        }
    );
}


std::string
position_reconciliation_json() {
    const auto results =
        reconcile_positions();

    const bool all_consistent =
        std::all_of(
            results.begin(),
            results.end(),
            [](
                const auto& result
            ) {
                return result.consistent;
            }
        );

    std::ostringstream out;

    out << "{\"symbols\":[";

    for (
        std::size_t index = 0;
        index < results.size();
        ++index
    ) {
        const auto& result =
            results[index];

        if (index != 0) {
            out << ",";
        }

        out
            << "{"
            << "\"symbol\":\""
            << result.symbol
            << "\""

            << ",\"expectedNetQuantity\":"
            << result.expected_net_quantity

            << ",\"actualNetQuantity\":"
            << result.actual_net_quantity

            << ",\"difference\":"
            << result.quantity_difference

            << ",\"expectedAverageCost\":"
            << result.expected_average_cost

            << ",\"actualAverageCost\":"
            << result.actual_average_cost

            << ",\"averageCostDifference\":"
            << result.average_cost_difference

            << ",\"expectedRealizedPnl\":"
            << result.expected_realized_pnl

            << ",\"actualRealizedPnl\":"
            << result.actual_realized_pnl

            << ",\"realizedPnlDifference\":"
            << result.realized_pnl_difference

            << ",\"markPrice\":"
            << result.mark_price

            << ",\"expectedUnrealizedPnl\":"
            << result.expected_unrealized_pnl

            << ",\"actualUnrealizedPnl\":"
            << result.actual_unrealized_pnl

            << ",\"unrealizedPnlDifference\":"
            << result.unrealized_pnl_difference

            << ",\"expectedTotalPnl\":"
            << result.expected_total_pnl

            << ",\"actualTotalPnl\":"
            << result.actual_total_pnl

            << ",\"totalPnlDifference\":"
            << result.total_pnl_difference

            << ",\"quantityConsistent\":"
            << (
                result.quantity_consistent
                    ? "true"
                    : "false"
            )

            << ",\"averageCostConsistent\":"
            << (
                result.average_cost_consistent
                    ? "true"
                    : "false"
            )

            << ",\"realizedPnlConsistent\":"
            << (
                result.realized_pnl_consistent
                    ? "true"
                    : "false"
            )

            << ",\"unrealizedPnlConsistent\":"
            << (
                result.unrealized_pnl_consistent
                    ? "true"
                    : "false"
            )

            << ",\"totalPnlConsistent\":"
            << (
                result.total_pnl_consistent
                    ? "true"
                    : "false"
            )

            << ",\"consistent\":"
            << (
                result.consistent
                    ? "true"
                    : "false"
            )

            << "}";
    }

    out
        << "]"
        << ",\"symbolCount\":"
        << results.size()
        << ",\"allConsistent\":"
        << (
            all_consistent
                ? "true"
                : "false"
        )
        << "}";

    return out.str();
}


minimatch::GlobalParentReconciliationResult
reconcile_all_parents() {
    const auto parents =
        state.oms.parents();

    std::vector<
        minimatch::ParentReconciliationInput
    > inputs;

    inputs.reserve(
        parents.size()
    );

    for (
        const auto& parent :
        parents
    ) {
        minimatch::ReconciliationClassification
            classification =
                minimatch::
                    ReconciliationClassification::
                        Consistent;

        switch (
            parent_reconciliation_status(
                parent.id
            )
        ) {
            case ReconciliationStatus::
                Consistent:
                classification =
                    minimatch::
                        ReconciliationClassification::
                            Consistent;
                break;

            case ReconciliationStatus::
                LegacyUnverifiable:
                classification =
                    minimatch::
                        ReconciliationClassification::
                            LegacyUnverifiable;
                break;

            case ReconciliationStatus::
                Inconsistent:
                classification =
                    minimatch::
                        ReconciliationClassification::
                            Inconsistent;
                break;
        }

        inputs.push_back(
            minimatch::
                ParentReconciliationInput{
                    .parent_id =
                        parent.id,
                    .classification =
                        classification
                }
        );
    }

    return minimatch::
        reconcile_parents(
            inputs
        );
}


std::string
reconciliation_prometheus_metrics() {
    const auto parent_reconciliation =
        reconcile_all_parents();

    const bool all_verifiable_consistent =
        parent_reconciliation
            .all_verifiable_consistent;

    const bool position_reconciliation_consistent =
        positions_reconciliation_consistent();

    const auto algo_orders =
        state.algo_scheduler.orders();

    std::size_t recovered_orders =
        0;

    std::size_t recovery_total =
        0;

    std::uint64_t last_recovered_at_ns =
        0;

    for (
        const auto& algo :
        algo_orders
    ) {
        recovery_total +=
            algo.recovery_count;

        if (
            algo.recovery_count >
            0
        ) {
            ++recovered_orders;
        }

        last_recovered_at_ns =
            std::max(
                last_recovered_at_ns,
                algo.last_recovered_at_ns
            );
    }

    /*
     * Position reconciliation is already included
     * in global_reconciliation_json(). For now,
     * derive the overall operational state from
     * the same invariant that currently determines
     * whether verifiable reconciliation has failed.
     *
     * We will expose the detailed position gauge
     * separately after extracting position
     * reconciliation into a typed result.
     */

    std::ostringstream out;

    out
        << "# HELP minimatch_reconciliation_parent_count "
        << "Total OMS parent orders\n"
        << "# TYPE minimatch_reconciliation_parent_count gauge\n"
        << "minimatch_reconciliation_parent_count "
        << parent_reconciliation
               .parent_count
        << "\n"

        << "# HELP minimatch_reconciliation_consistent_parents "
        << "Parent orders with consistent reconciliation\n"
        << "# TYPE minimatch_reconciliation_consistent_parents gauge\n"
        << "minimatch_reconciliation_consistent_parents "
        << parent_reconciliation
               .consistent_parents
        << "\n"

        << "# HELP minimatch_reconciliation_legacy_unverifiable_parents "
        << "Legacy parents missing sufficient historical evidence\n"
        << "# TYPE minimatch_reconciliation_legacy_unverifiable_parents gauge\n"
        << "minimatch_reconciliation_legacy_unverifiable_parents "
        << parent_reconciliation
               .legacy_unverifiable_parents
        << "\n"

        << "# HELP minimatch_reconciliation_inconsistent_parents "
        << "Parent orders with reconciliation failures\n"
        << "# TYPE minimatch_reconciliation_inconsistent_parents gauge\n"
        << "minimatch_reconciliation_inconsistent_parents "
        << parent_reconciliation
               .inconsistent_parents
        << "\n"

        << "# HELP minimatch_reconciliation_verifiable_consistent "
        << "Whether all verifiable parents reconcile successfully\n"
        << "# TYPE minimatch_reconciliation_verifiable_consistent gauge\n"
        << "minimatch_reconciliation_verifiable_consistent "
        << (
            all_verifiable_consistent
                ? 1
                : 0
        )
        << "\n"

        << "# HELP minimatch_position_reconciliation_consistent "
        << "Whether reconstructed positions match live position state\n"
        << "# TYPE minimatch_position_reconciliation_consistent gauge\n"
        << "minimatch_position_reconciliation_consistent "
        << (
            position_reconciliation_consistent
                ? 1
                : 0
        )
        << "\n"

        << "# HELP minimatch_algo_recovery_total "
        << "Total algorithm recovery operations across persisted orders\n"
        << "# TYPE minimatch_algo_recovery_total counter\n"
        << "minimatch_algo_recovery_total "
        << recovery_total
        << "\n"

        << "# HELP minimatch_algo_recovered_orders "
        << "Number of algorithm orders recovered at least once\n"
        << "# TYPE minimatch_algo_recovered_orders gauge\n"
        << "minimatch_algo_recovered_orders "
        << recovered_orders
        << "\n"

        << "# HELP minimatch_algo_last_recovered_timestamp_seconds "
        << "Unix timestamp of the most recent algorithm recovery\n"
        << "# TYPE minimatch_algo_last_recovered_timestamp_seconds gauge\n"
        << "minimatch_algo_last_recovered_timestamp_seconds "
        << (
            static_cast<double>(
                last_recovered_at_ns
            ) /
            1'000'000'000.0
        )
        << "\n";

    const std::string benchmark_metrics_path =
        "benchmark_results/benchmark_metrics.prom";

    std::ifstream benchmark_metrics(
        benchmark_metrics_path
    );

    if (benchmark_metrics) {
        out
            << "\n"
            << benchmark_metrics.rdbuf();
    }

    return out.str();
}


std::string
global_reconciliation_json() {
    const auto parent_reconciliation =
        reconcile_all_parents();

    const bool positions_consistent =
        positions_reconciliation_consistent();

    const bool overall_consistent =
        parent_reconciliation
            .all_verifiable_consistent &&
        positions_consistent;

    std::ostringstream out;

    out
        << "{"
        << "\"parentCount\":"
        << parent_reconciliation
               .parent_count

        << ",\"consistentParents\":"
        << parent_reconciliation
               .consistent_parents

        << ",\"legacyUnverifiableParents\":"
        << parent_reconciliation
               .legacy_unverifiable_parents

        << ",\"inconsistentParents\":"
        << parent_reconciliation
               .inconsistent_parents

        << ",\"allVerifiableParentsConsistent\":"
        << (
            parent_reconciliation
                .all_verifiable_consistent
                ? "true"
                : "false"
        )

        << ",\"positionsConsistent\":"
        << (
            positions_consistent
                ? "true"
                : "false"
        )

        << ",\"overallConsistent\":"
        << (
            overall_consistent
                ? "true"
                : "false"
        )

        << ",\"legacyUnverifiableParentIds\":[";

    for (
        std::size_t index = 0;
        index <
            parent_reconciliation
                .legacy_unverifiable_ids
                .size();
        ++index
    ) {
        if (index != 0) {
            out << ",";
        }

        out
            << "\""
            << parent_reconciliation
                   .legacy_unverifiable_ids[
                       index
                   ]
            << "\"";
    }

    out
        << "]"
        << ",\"inconsistentParentIds\":[";

    for (
        std::size_t index = 0;
        index <
            parent_reconciliation
                .inconsistent_ids
                .size();
        ++index
    ) {
        if (index != 0) {
            out << ",";
        }

        out
            << "\""
            << parent_reconciliation
                   .inconsistent_ids[
                       index
                   ]
            << "\"";
    }

    out << "]}";

    return out.str();
}


std::string
parent_reconciliation_json(
    ParentOrderId parent_id
) {
    const auto parent =
        state.oms.find_parent(
            parent_id
        );

    if (!parent) {
        return {};
    }

    const auto fills =
        state.oms.fills_for_parent(
            parent_id
        );

    const auto children =
        state.oms.children_for_parent(
            parent_id
        );

    const auto drop_copy_events =
        state.drop_copy.recent(
            10'000
        );

    double fill_quantity =
        0.0;

    std::size_t drop_copy_fill_count =
        0;

    double child_fill_quantity =
        0.0;

    double fill_notional =
        0.0;

    double total_fees =
        0.0;

    for (
        const auto& fill :
        fills
    ) {
        fill_quantity +=
            fill.quantity;

        fill_notional +=
            fill.notional;

        total_fees +=
            fill.fee;
    }

    for (
        const auto& child :
        children
    ) {
        child_fill_quantity +=
            child.filled_quantity;

        for (
            const auto& event :
            drop_copy_events
        ) {
            if (
                event.event_type ==
                    "OMS_FILL" &&
                event.order_id ==
                    static_cast<OrderId>(
                        child.id
                    )
            ) {
                ++drop_copy_fill_count;
                break;
            }
        }
    }

    const auto algo_state =
        state.algo_scheduler.find(
            parent_id
        );

    const double arrival_price =
        algo_state
            ? algo_state->arrival_price
            : 0.0;

    const auto quality =
        state.execution_analytics.analyze(
            *parent,
            fills,
            arrival_price
        );

    constexpr double epsilon =
        1e-9;

    const bool quantity_consistent =
        std::abs(
            parent->filled_quantity -
            fill_quantity
        ) <= epsilon;

    const bool child_quantity_consistent =
        std::abs(
            parent->filled_quantity -
            child_fill_quantity
        ) <= epsilon;

    const bool analytics_quantity_consistent =
        std::abs(
            quality.filled_quantity -
            fill_quantity
        ) <= epsilon;

    const bool analytics_notional_consistent =
        std::abs(
            quality.total_notional -
            fill_notional
        ) <= epsilon;

    const bool analytics_fees_consistent =
        std::abs(
            quality.total_fees -
            total_fees
        ) <= epsilon;

    const bool drop_copy_consistent =
        drop_copy_fill_count ==
        fills.size();

    const auto reconciliation_status =
        parent_reconciliation_status(
            parent_id
        );

    std::string reconciliation_reason;

    if (
        reconciliation_status ==
        ReconciliationStatus::
            LegacyUnverifiable
    ) {
        reconciliation_reason =
            "missing historical OMS_FILL drop-copy events";
    } else if (
        reconciliation_status ==
        ReconciliationStatus::
            Inconsistent
    ) {
        if (!quantity_consistent) {
            reconciliation_reason =
                "parent filled quantity differs from OMS fills";
        } else if (!child_quantity_consistent) {
            reconciliation_reason =
                "parent filled quantity differs from child fills";
        } else if (
            !analytics_quantity_consistent
        ) {
            reconciliation_reason =
                "execution analytics quantity mismatch";
        } else if (
            !analytics_notional_consistent
        ) {
            reconciliation_reason =
                "execution analytics notional mismatch";
        } else if (
            !analytics_fees_consistent
        ) {
            reconciliation_reason =
                "execution analytics fee mismatch";
        } else if (
            !drop_copy_consistent
        ) {
            reconciliation_reason =
                "drop-copy fill mismatch";
        } else {
            reconciliation_reason =
                "unknown reconciliation mismatch";
        }
    }

    std::ostringstream out;

    out
        << "{"
        << "\"parentOrderId\":\""
        << parent_id
        << "\""
        << ",\"symbol\":\""
        << parent->symbol
        << "\""
        << ",\"reconciliationStatus\":\""
        << reconciliation_status_string(
               reconciliation_status
           )
        << "\""
        << ",\"reason\":\""
        << reconciliation_reason
        << "\""
        << ",\"parentFilledQuantity\":"
        << parent->filled_quantity
        << ",\"omsFillQuantity\":"
        << fill_quantity
        << ",\"childFilledQuantity\":"
        << child_fill_quantity
        << ",\"omsFillNotional\":"
        << fill_notional
        << ",\"totalFees\":"
        << total_fees
        << ",\"analyticsFilledQuantity\":"
        << quality.filled_quantity
        << ",\"analyticsTotalNotional\":"
        << quality.total_notional
        << ",\"analyticsTotalFees\":"
        << quality.total_fees
        << ",\"fillCount\":"
        << fills.size()
        << ",\"dropCopyFillCount\":"
        << drop_copy_fill_count
        << ",\"childCount\":"
        << children.size()
        << ",\"quantityDifference\":"
        << (
            parent->filled_quantity -
            fill_quantity
        )
        << ",\"quantityConsistent\":"
        << (
            quantity_consistent
                ? "true"
                : "false"
        )
        << ",\"childQuantityConsistent\":"
        << (
            child_quantity_consistent
                ? "true"
                : "false"
        )
        << ",\"analyticsQuantityConsistent\":"
        << (
            analytics_quantity_consistent
                ? "true"
                : "false"
        )
        << ",\"analyticsNotionalConsistent\":"
        << (
            analytics_notional_consistent
                ? "true"
                : "false"
        )
        << ",\"analyticsFeesConsistent\":"
        << (
            analytics_fees_consistent
                ? "true"
                : "false"
        )
        << ",\"dropCopyConsistent\":"
        << (
            drop_copy_consistent
                ? "true"
                : "false"
        )
        << ",\"consistent\":"
        << (
            quantity_consistent &&
            child_quantity_consistent &&
            analytics_quantity_consistent &&
            analytics_notional_consistent &&
            analytics_fees_consistent &&
            drop_copy_consistent
                ? "true"
                : "false"
        )
        << "}";

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
            "/metrics"
    ) {
        return text_response(
            http::status::ok,
            reconciliation_prometheus_metrics()
        );
    }


    if (
        req.method() ==
            http::verb::get &&
        target ==
            "/api/system/stp"
    ) {
        std::ostringstream out;

        out
            << "{"
            << "\"policy\":\""
            << stp_policy_string(
                   state.exchange
                       .stp_policy()
               )
            << "\""
            << "}";

        return json_response(
            http::status::ok,
            out.str()
        );
    }

    if (
        req.method() ==
            http::verb::post &&
        target ==
            "/api/system/stp"
    ) {
        try {
            const auto fields =
                parse_json_like_body(
                    req.body()
                );

            std::string policy =
                fields.at("policy");

            std::transform(
                policy.begin(),
                policy.end(),
                policy.begin(),
                [](unsigned char c) {
                    return static_cast<char>(
                        std::toupper(c)
                    );
                }
            );

            minimatch::
                SelfTradePreventionPolicy
                    value =
                        minimatch::
                            SelfTradePreventionPolicy::
                                None;

            if (
                policy ==
                "CANCEL_NEWEST"
            ) {
                value =
                    minimatch::
                        SelfTradePreventionPolicy::
                            CancelNewest;

            } else if (
                policy ==
                "CANCEL_OLDEST"
            ) {
                value =
                    minimatch::
                        SelfTradePreventionPolicy::
                            CancelOldest;

            } else if (
                policy ==
                "CANCEL_BOTH"
            ) {
                value =
                    minimatch::
                        SelfTradePreventionPolicy::
                            CancelBoth;

            } else if (
                policy != "NONE"
            ) {
                return json_response(
                    http::status::
                        bad_request,
                    R"({"error":"invalid STP policy"})"
                );
            }

            state.exchange
                .set_stp_policy(
                    value
                );

            std::ostringstream out;

            out
                << "{"
                << "\"ok\":true"
                << ",\"policy\":\""
                << stp_policy_string(
                       value
                   )
                << "\""
                << "}";

            return json_response(
                http::status::ok,
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
            http::verb::get &&
        target ==
            "/api/system/circuit-breakers"
    ) {
        const auto latest =
            state.midpoint_history.last(
                "btcusd"
            );

        std::optional<
            minimatch::MidpointObservation
        > reference;

        if (
            latest &&
            latest->timestamp_ns >
                state
                    .volatility_breaker
                    .window_ns
        ) {
            reference =
                state.midpoint_history
                    .at_or_before(
                        "btcusd",
                        latest->timestamp_ns -
                            state
                                .volatility_breaker
                                .window_ns
                    );
        }

        double move_percent =
            0.0;

        if (
            latest &&
            reference &&
            reference->midpoint >
                0.0
        ) {
            move_percent =
                std::abs(
                    latest->midpoint -
                    reference->midpoint
                ) /
                reference->midpoint *
                100.0;
        }

        std::ostringstream out;

        out
            << "{"
            << "\"enabled\":"
            << (
                state
                    .volatility_breaker
                    .enabled
                    ? "true"
                    : "false"
            )
            << ",\"thresholdPercent\":"
            << state
                   .volatility_breaker
                   .threshold_percent
            << ",\"windowSeconds\":"
            << (
                state
                    .volatility_breaker
                    .window_ns /
                1'000'000'000ULL
            )
            << ",\"currentMidpoint\":"
            << (
                latest
                    ? latest->midpoint
                    : 0.0
            )
            << ",\"referenceMidpoint\":"
            << (
                reference
                    ? reference->midpoint
                    : 0.0
            )
            << ",\"movePercent\":"
            << move_percent
            << ",\"symbolHalted\":"
            << (
                state.exchange
                        .symbol_halted(1)
                    ? "true"
                    : "false"
            )
            << ",\"observations\":"
            << state.midpoint_history
                   .size("btcusd")
            << "}";

        return json_response(
            http::status::ok,
            out.str()
        );
    }

    if (
        req.method() ==
            http::verb::post &&
        target ==
            "/api/system/circuit-breakers/config"
    ) {
        try {
            const auto fields =
                parse_json_like_body(
                    req.body()
                );

            if (
                fields.count(
                    "enabled"
                )
            ) {
                const auto value =
                    fields.at(
                        "enabled"
                    );

                state
                    .volatility_breaker
                    .enabled =
                    (
                        value == "true" ||
                        value == "1" ||
                        value == "TRUE"
                    );
            }

            if (
                fields.count(
                    "thresholdPercent"
                )
            ) {
                const double threshold =
                    std::stod(
                        fields.at(
                            "thresholdPercent"
                        )
                    );

                if (
                    threshold <= 0.0
                ) {
                    return json_response(
                        http::status::
                            bad_request,
                        R"({"error":"thresholdPercent must be positive"})"
                    );
                }

                state
                    .volatility_breaker
                    .threshold_percent =
                    threshold;
            }

            if (
                fields.count(
                    "windowSeconds"
                )
            ) {
                const auto seconds =
                    std::stoull(
                        fields.at(
                            "windowSeconds"
                        )
                    );

                if (
                    seconds == 0
                ) {
                    return json_response(
                        http::status::
                            bad_request,
                        R"({"error":"windowSeconds must be positive"})"
                    );
                }

                state
                    .volatility_breaker
                    .window_ns =
                    seconds *
                    1'000'000'000ULL;
            }

            state.settings_store
                .save_circuit_breaker(
                    minimatch::
                        CircuitBreakerSettings{
                            state
                                .volatility_breaker
                                .enabled,
                            state
                                .volatility_breaker
                                .threshold_percent,
                            state
                                .volatility_breaker
                                .window_ns
                        }
                );

            return json_response(
                http::status::ok,
                R"({"ok":true})"
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
            http::verb::get &&
        target.rfind(
            "/api/symbols/",
            0
        ) == 0 &&
        target.ends_with(
            "/status"
        )
    ) {
        try {
            const std::string prefix =
                "/api/symbols/";

            const std::string suffix =
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
                    http::status::bad_request,
                    R"({"error":"invalid symbol status route"})"
                );
            }

            const auto symbol =
                static_cast<SymbolId>(
                    std::stoul(
                        suffix.substr(
                            0,
                            slash
                        )
                    )
                );

            std::ostringstream out;

            out
                << "{"
                << "\"symbol\":"
                << symbol
                << ",\"halted\":"
                << (
                    state.exchange
                            .symbol_halted(
                                symbol
                            )
                        ? "true"
                        : "false"
                )
                << ",\"globallyHalted\":"
                << (
                    state.exchange
                            .globally_halted()
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
            http::verb::get &&
        target ==
            "/api/system/trading-status"
    ) {
        return json_response(
            http::status::ok,
            std::string(
                "{\"globallyHalted\":"
            ) +
                (
                    state.exchange
                            .globally_halted()
                        ? "true"
                        : "false"
                ) +
                "}"
        );
    }

    if (
        req.method() ==
            http::verb::post &&
        target ==
            "/api/system/halt"
    ) {
        state.exchange.halt_all();

        return json_response(
            http::status::ok,
            R"({"ok":true,"globallyHalted":true})"
        );
    }

    if (
        req.method() ==
            http::verb::post &&
        target ==
            "/api/system/resume"
    ) {
        state.exchange.resume_all();

        return json_response(
            http::status::ok,
            R"({"ok":true,"globallyHalted":false})"
        );
    }

    if (
        req.method() ==
            http::verb::post &&
        target.rfind(
            "/api/symbols/",
            0
        ) == 0
     &&
        (
            target.ends_with(
                "/halt"
            ) ||
            target.ends_with(
                "/resume"
            )
        )
     &&
        (
            target.ends_with(
                "/halt"
            ) ||
            target.ends_with(
                "/resume"
            )
        )
) {
        try {
            const std::string prefix =
                "/api/symbols/";

            const std::string suffix =
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
                    http::status::bad_request,
                    R"({"error":"invalid symbol halt route"})"
                );
            }

            const auto symbol =
                static_cast<SymbolId>(
                    std::stoul(
                        suffix.substr(
                            0,
                            slash
                        )
                    )
                );

            const auto action =
                suffix.substr(
                    slash + 1
                );

            if (
                action == "halt"
            ) {
                state.exchange
                    .halt_symbol(
                        symbol
                    );

            } else if (
                action == "resume"
            ) {
                state.exchange
                    .resume_symbol(
                        symbol
                    );

            } else {
                // This is not a halt/resume route.
                // Allow later /api/symbols/* handlers
                // such as /price-band to process it.
                goto skip_symbol_halt_route;
            }

            std::ostringstream out;

            out
                << "{"
                << "\"ok\":true"
                << ",\"symbol\":"
                << symbol
                << ",\"halted\":"
                << (
                    state.exchange
                            .symbol_halted(
                                symbol
                            )
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


    skip_symbol_halt_route:

    if (
        target.rfind(
            "/api/symbols/",
            0
        ) == 0 &&
        target.find(
            "/price-band"
        ) != std::string::npos
    ) {
        try {
            const std::string prefix =
                "/api/symbols/";

            const std::string suffix =
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
                    http::status::bad_request,
                    R"({"error":"invalid price-band route"})"
                );
            }

            const auto symbol =
                static_cast<SymbolId>(
                    std::stoul(
                        suffix.substr(
                            0,
                            slash
                        )
                    )
                );

            const auto action =
                suffix.substr(
                    slash + 1
                );

            if (
                action != "price-band"
            ) {
                return json_response(
                    http::status::bad_request,
                    R"({"error":"invalid price-band route"})"
                );
            }

            if (
                req.method() ==
                    http::verb::post
            ) {
                const auto fields =
                    parse_json_like_body(
                        req.body()
                    );

                const auto reference_price =
                    static_cast<Price>(
                        std::stoll(
                            fields.at(
                                "referencePrice"
                            )
                        )
                    );

                const double band_percent =
                    std::stod(
                        fields.at(
                            "bandPercent"
                        )
                    );

                state.exchange
                    .set_price_band(
                        symbol,
                        reference_price,
                        band_percent
                    );

                std::ostringstream out;

                out
                    << "{"
                    << "\"ok\":true"
                    << ",\"symbol\":"
                    << symbol
                    << ",\"referencePrice\":"
                    << reference_price
                    << ",\"bandPercent\":"
                    << band_percent
                    << "}";

                return json_response(
                    http::status::ok,
                    out.str()
                );
            }

            if (
                req.method() ==
                    http::verb::delete_
            ) {
                state.exchange
                    .clear_price_band(
                        symbol
                    );

                std::ostringstream out;

                out
                    << "{"
                    << "\"ok\":true"
                    << ",\"symbol\":"
                    << symbol
                    << "}";

                return json_response(
                    http::status::ok,
                    out.str()
                );
            }

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
            http::verb::get &&
        target ==
            "/api/oms/parents"
    ) {
        const auto parents =
            state.oms.parents();

        std::ostringstream out;

        out << "[";

        for (
            std::size_t i = 0;
            i < parents.size();
            ++i
        ) {
            if (i) {
                out << ",";
            }

            const auto& parent =
                parents[i];

            std::string strategy =
                "MARKET";

            switch (
                parent.algorithm
            ) {
                case ExecutionAlgorithm::TWAP:
                    strategy = "TWAP";
                    break;

                case ExecutionAlgorithm::VWAP:
                    strategy = "VWAP";
                    break;

                case ExecutionAlgorithm::POV:
                    strategy = "POV";
                    break;

                case ExecutionAlgorithm::Iceberg:
                    strategy = "ICEBERG";
                    break;

                case ExecutionAlgorithm::Market:
                    strategy = "MARKET";
                    break;
            }

            out
                << "{"
                << "\"id\":\""
                << parent.id
                << "\""
                << ",\"symbol\":\""
                << parent.symbol
                << "\""
                << ",\"side\":\""
                << (
                    parent.side ==
                            RouteSide::Buy
                        ? "BUY"
                        : "SELL"
                )
                << "\""
                << ",\"status\":\""
                << minimatch::to_string(
                       parent.status
                   )
                << "\""
                << ",\"quantity\":"
                << parent.quantity
                << ",\"filledQuantity\":"
                << parent.filled_quantity
                << ",\"remainingQuantity\":"
                << parent.remaining_quantity
                << ",\"strategy\":\""
                << strategy
                << "\""
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
            "/api/oms/parents/",
            0
        ) == 0
    ) {
        try {
            const std::string prefix =
                "/api/oms/parents/";

            const std::string suffix =
                target.substr(
                    prefix.size()
                );

            const std::string
                children_suffix =
                    "/children";

            const std::string
                fills_suffix =
                    "/fills";

            if (
                suffix.size() >
                    children_suffix.size() &&
                suffix.ends_with(
                    children_suffix
                )
            ) {
                const auto id_text =
                    suffix.substr(
                        0,
                        suffix.size() -
                            children_suffix.size()
                    );

                const auto parent_id =
                    static_cast<
                        ParentOrderId
                    >(
                        std::stoull(
                            id_text
                        )
                    );

                const auto parent =
                    state.oms.find_parent(
                        parent_id
                    );

                if (!parent) {
                    return json_response(
                        http::status::
                            not_found,
                        R"({"error":"not found"})"
                    );
                }

                const auto children =
                    state.oms
                        .children_for_parent(
                            parent_id
                        );

                std::ostringstream out;

                out << "[";

                for (
                    std::size_t i = 0;
                    i < children.size();
                    ++i
                ) {
                    if (i) {
                        out << ",";
                    }

                    const auto& child =
                        children[i];

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
                        << child
                               .filled_quantity
                        << ",\"remainingQuantity\":"
                        << child
                               .remaining_quantity
                        << "}";
                }

                out << "]";

                return json_response(
                    http::status::ok,
                    out.str()
                );
            }

            if (
                suffix.size() >
                    fills_suffix.size() &&
                suffix.ends_with(
                    fills_suffix
                )
            ) {
                const auto id_text =
                    suffix.substr(
                        0,
                        suffix.size() -
                            fills_suffix.size()
                    );

                const auto parent_id =
                    static_cast<
                        ParentOrderId
                    >(
                        std::stoull(
                            id_text
                        )
                    );

                const auto parent =
                    state.oms.find_parent(
                        parent_id
                    );

                if (!parent) {
                    return json_response(
                        http::status::
                            not_found,
                        R"({"error":"not found"})"
                    );
                }

                const auto fills =
                    state.oms
                        .fills_for_parent(
                            parent_id
                        );

                std::ostringstream out;

                out << "[";

                for (
                    std::size_t i = 0;
                    i < fills.size();
                    ++i
                ) {
                    if (i) {
                        out << ",";
                    }

                    const auto& fill =
                        fills[i];

                    out
                        << "{"
                        << "\"id\":\""
                        << fill
                               .execution_report_id
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

                return json_response(
                    http::status::ok,
                    out.str()
                );
            }

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
            http::verb::get &&
        target ==
            "/api/midpoint-history"
    ) {
        const auto first =
            state.midpoint_history.first(
                "btcusd"
            );

        const auto last =
            state.midpoint_history.last(
                "btcusd"
            );

        std::ostringstream out;

        out
            << "{"
            << "\"symbol\":\"btcusd\""
            << ",\"observations\":"
            << state.midpoint_history.size(
                   "btcusd"
               )
            << ",\"firstTimestampNs\":"
            << (
                first
                    ? first->timestamp_ns
                    : 0
            )
            << ",\"firstMidpoint\":"
            << (
                first
                    ? first->midpoint
                    : 0.0
            )
            << ",\"lastTimestampNs\":"
            << (
                last
                    ? last->timestamp_ns
                    : 0
            )
            << ",\"lastMidpoint\":"
            << (
                last
                    ? last->midpoint
                    : 0.0
            )
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
            "/api/execution-quality/",
            0
        ) == 0
    ) {
        try {
            const std::string id_text =
                target.substr(
                    std::string(
                        "/api/execution-quality/"
                    ).size()
                );

            const auto parent_id =
                static_cast<
                    ParentOrderId
                >(
                    std::stoull(
                        id_text
                    )
                );

            const auto parent =
                state.oms.find_parent(
                    parent_id
                );

            if (!parent) {
                return json_response(
                    http::status::not_found,
                    R"({"error":"parent order not found"})"
                );
            }

            const auto algo_state =
                state.algo_scheduler.find(
                    parent_id
                );

            const double arrival_price =
                algo_state
                    ? algo_state
                          ->arrival_price
                    : 0.0;

            const auto fills =
                state.oms
                    .fills_for_parent(
                        parent_id
                    );

            const auto quality =
                state.execution_analytics
                    .analyze(
                        *parent,
                        fills,
                        arrival_price
                    );

            std::ostringstream out;

            out
                << "{"
                << "\"parentOrderId\":\""
                << quality.parent_order_id
                << "\""
                << ",\"symbol\":\""
                << quality.symbol
                << "\""
                << ",\"requestedQuantity\":"
                << quality.requested_quantity
                << ",\"filledQuantity\":"
                << quality.filled_quantity
                << ",\"arrivalPrice\":"
                << quality.arrival_price
                << ",\"averageFillPrice\":"
                << quality.average_fill_price
                << ",\"totalNotional\":"
                << quality.total_notional
                << ",\"totalFees\":"
                << quality.total_fees
                << ",\"implementationShortfallBps\":"
                << quality
                       .implementation_shortfall_bps
                << ",\"feeAdjustedShortfallBps\":"
                << quality
                       .fee_adjusted_shortfall_bps
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
            http::verb::get &&
        target ==
            "/api/reconciliation/positions"
    ) {
        return json_response(
            http::status::ok,
            position_reconciliation_json()
        );
    }


    if (
        req.method() ==
            http::verb::get &&
        target ==
            "/api/reconciliation"
    ) {
        return json_response(
            http::status::ok,
            global_reconciliation_json()
        );
    }


    if (
        req.method() ==
            http::verb::get &&
        target.starts_with(
            "/api/reconciliation/parents/"
        )
    ) {
        try {
            const std::string prefix =
                "/api/reconciliation/parents/";

            const auto id_text =
                target.substr(
                    prefix.size()
                );

            const auto parent_id =
                static_cast<
                    ParentOrderId
                >(
                    std::stoull(
                        id_text
                    )
                );

            const auto body =
                parent_reconciliation_json(
                    parent_id
                );

            if (body.empty()) {
                return json_response(
                    http::status::not_found,
                    R"({"error":"parent order not found"})"
                );
            }

            return json_response(
                http::status::ok,
                body
            );

        } catch (
            const std::exception& ex
        ) {
            return json_response(
                http::status::bad_request,
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
            "/api/algo-orders"
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

            const RouteSide side =
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

            minimatch::ExecutionScheduleRequest schedule;

            schedule.algorithm =
                algorithm;

            schedule.quantity =
                quantity;

            schedule.slices =
                fields.count("slices")
                    ? std::stoi(
                          fields.at("slices")
                      )
                    : 1;

            schedule.duration_seconds =
                fields.count(
                    "durationSeconds"
                )
                    ? std::stod(
                          fields.at(
                              "durationSeconds"
                          )
                      )
                    : 0.0;

            schedule.participation_rate =
                fields.count(
                    "participationRate"
                )
                    ? std::stod(
                          fields.at(
                              "participationRate"
                          )
                      )
                    : 0.10;

            schedule.displayed_quantity =
                fields.count(
                    "displayedQuantity"
                )
                    ? std::stod(
                          fields.at(
                              "displayedQuantity"
                          )
                      )
                    : quantity;

            minimatch::LiveAlgoRequest algo_request;

            algo_request.symbol =
                symbol;

            algo_request.side =
                side;

            algo_request.quantity =
                quantity;

            algo_request.schedule =
                schedule;

            if (
                fields.count(
                    "maxSlippageBps"
                )
            ) {
                algo_request.max_slippage_bps =
                    std::stod(
                        fields.at(
                            "maxSlippageBps"
                        )
                    );
            }

            if (
                fields.count(
                    "maxVenueCount"
                )
            ) {
                algo_request.max_venue_count =
                    static_cast<std::size_t>(
                        std::stoull(
                            fields.at(
                                "maxVenueCount"
                            )
                        )
                    );
            }

            const auto parent_id =
                state.algo_scheduler.submit(
                    algo_request
                );

            state.algo_store.save_request(
                parent_id,
                algo_request
            );

            std::ostringstream out;

            out
                << "{"
                << "\"ok\":true"
                << ",\"parentOrderId\":\""
                << parent_id
                << "\""
                << ",\"status\":\"RUNNING\""
                << "}";

            return json_response(
                http::status::accepted,
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
            "/api/drop-copy"
    ) {
        const auto events =
            state.drop_copy.recent(
                100
            );

        std::ostringstream out;

        out << "[";

        for (
            std::size_t i = 0;
            i < events.size();
            ++i
        ) {
            if (i) {
                out << ",";
            }

            const auto& event =
                events[i];

            out
                << "{"
                << "\"id\":"
                << event.id
                << ",\"timestampNs\":"
                << event.timestamp_ns
                << ",\"orderId\":"
                << event.order_id
                << ",\"symbol\":"
                << event.symbol
                << ",\"eventType\":\""
                << event.event_type
                << "\""
                << ",\"status\":\""
                << event.status
                << "\""
                << ",\"remaining\":"
                << event.remaining
                << ",\"rejectReason\":\""
                << reject_reason_string(
                       event.reject_reason
                   )
                << "\""
                << "}";
        }

        out << "]";

        return json_response(
            http::status::ok,
            out.str()
        );
    }


    // ---------------------------------------------------------
    // Quantitative analytics
    // ---------------------------------------------------------

    if (
        req.method() == http::verb::get &&
        target == "/api/analytics/portfolio"
    ) {
        try {
            const std::vector<double> returns{
                0.012, -0.006, 0.009, 0.004, -0.011,
                0.015, 0.007, -0.003, 0.010, 0.006,
                -0.008, 0.013, 0.005, -0.004, 0.011,
                0.008, -0.002, 0.014, -0.007, 0.009
            };

            std::vector<double> equity{1.0};

            for (const auto r : returns) {
                equity.push_back(
                    equity.back() * (1.0 + r)
                );
            }

            const auto metrics =
                minimatch::calculate_metrics(
                    returns,
                    equity,
                    252.0,
                    0.0
                );

            const auto risk =
                minimatch::historical_risk(
                    returns,
                    0.95
                );

            std::ostringstream out;

            out
                << "{"
                << "\"maxDrawdown\":"
                << metrics.max_drawdown
                << ",\"winRate\":"
                << metrics.win_rate
                << ",\"var\":"
                << risk.value_at_risk
                << ",\"expectedShortfall\":"
                << risk.expected_shortfall
                << "}";

            return json_response(
                http::status::ok,
                out.str()
            );

        } catch (const std::exception& ex) {
            return json_response(
                http::status::internal_server_error,
                std::string("{\"error\":\"") +
                    ex.what() +
                    "\"}"
            );
        }
    }


    if (
        req.method() == http::verb::get &&
        target == "/api/analytics/pairs"
    ) {
        try {
            std::vector<double> x;
            std::vector<double> y;

            x.reserve(100);
            y.reserve(100);

            for (std::size_t i = 0; i < 100; ++i) {
                const double xv =
                    100.0 +
                    static_cast<double>(i) * 0.15 +
                    std::sin(
                        static_cast<double>(i) * 0.2
                    );

                const double noise =
                    0.35 *
                    std::sin(
                        static_cast<double>(i) * 0.7
                    );

                x.push_back(xv);

                y.push_back(
                    12.0 +
                    1.4 * xv +
                    noise
                );
            }

            const auto model =
                minimatch::fit_pairs_model(x, y);

            const auto zscores =
                minimatch::spread_zscores(
                    x,
                    y,
                    model
                );

            std::ostringstream out;

            out
                << "{"
                << "\"beta\":"
                << model.beta
                << ",\"zscore\":"
                << zscores.back()
                << ",\"adfTStatistic\":"
                << model.adf_t_stat
                << ",\"stationary\":"
                << (
                    model.stationary
                        ? "true"
                        : "false"
                )
                << "}";

            return json_response(
                http::status::ok,
                out.str()
            );

        } catch (const std::exception& ex) {
            return json_response(
                http::status::internal_server_error,
                std::string("{\"error\":\"") +
                    ex.what() +
                    "\"}"
            );
        }
    }


    if (
        req.method() == http::verb::get &&
        target == "/api/analytics/options"
    ) {
        try {
            minimatch::OptionInputs inputs{
                100.0,
                105.0,
                0.04,
                0.01,
                0.25,
                0.5
            };

            const auto bs =
                minimatch::black_scholes(
                    minimatch::OptionKind::Call,
                    inputs
                );

            auto iv_inputs = inputs;
            iv_inputs.volatility = 0.15;

            const auto iv =
                minimatch::implied_volatility(
                    minimatch::OptionKind::Call,
                    bs.price,
                    iv_inputs
                );

            const auto mc =
                minimatch::monte_carlo_option_price(
                    minimatch::OptionKind::Call,
                    inputs,
                    100000,
                    42
                );

            std::ostringstream out;

            out
                << "{"
                << "\"price\":"
                << bs.price
                << ",\"impliedVolatility\":"
                << iv
                << ",\"delta\":"
                << bs.delta
                << ",\"gamma\":"
                << bs.gamma
                << ",\"vega\":"
                << bs.vega
                << ",\"theta\":"
                << bs.theta
                << ",\"rho\":"
                << bs.rho
                << ",\"monteCarloPrice\":"
                << mc
                << "}";

            return json_response(
                http::status::ok,
                out.str()
            );

        } catch (const std::exception& ex) {
            return json_response(
                http::status::internal_server_error,
                std::string("{\"error\":\"") +
                    ex.what() +
                    "\"}"
            );
        }
    }


    if (
        req.method() ==
            http::verb::get &&
        target ==
            "/api/portfolio"
    ) {
        const auto positions =
            state.positions.positions();

        const auto summary =
            state.portfolio_risk
                .summarize(
                    positions
                );

        std::ostringstream out;

        out
            << "{"
            << "\"grossExposure\":"
            << summary.gross_exposure
            << ",\"netExposure\":"
            << summary.net_exposure
            << ",\"realizedPnl\":"
            << summary.realized_pnl
            << ",\"unrealizedPnl\":"
            << summary.unrealized_pnl
            << ",\"totalPnl\":"
            << summary.total_pnl
            << ",\"positionCount\":"
            << summary.position_count
            << ",\"largestPositionSymbol\":\""
            << summary
                   .largest_position_symbol
            << "\""
            << ",\"largestPositionExposure\":"
            << summary
                   .largest_position_exposure
            << ",\"largestConcentrationPercent\":"
            << summary
                   .largest_concentration_percent
            << "}";

        return json_response(
            http::status::ok,
            out.str()
        );
    }

    if (
        req.method() ==
            http::verb::get &&
        target ==
            "/api/portfolio/risk"
    ) {
        const auto positions =
            state.positions.positions();

        const auto risk =
            state.portfolio_risk
                .evaluate(
                    positions
                );

        std::ostringstream out;

        out
            << "{"
            << "\"breached\":"
            << (
                risk.breached
                    ? "true"
                    : "false"
            )
            << ",\"grossExposureBreached\":"
            << (
                risk
                    .gross_exposure_breached
                    ? "true"
                    : "false"
            )
            << ",\"netExposureBreached\":"
            << (
                risk
                    .net_exposure_breached
                    ? "true"
                    : "false"
            )
            << ",\"concentrationBreached\":"
            << (
                risk
                    .concentration_breached
                    ? "true"
                    : "false"
            )
            << ",\"dailyLossBreached\":"
            << (
                risk
                    .daily_loss_breached
                    ? "true"
                    : "false"
            )
            << ",\"grossExposure\":"
            << risk.summary
                   .gross_exposure
            << ",\"netExposure\":"
            << risk.summary
                   .net_exposure
            << ",\"totalPnl\":"
            << risk.summary
                   .total_pnl
            << ",\"largestConcentrationPercent\":"
            << risk.summary
                   .largest_concentration_percent
            << ",\"limits\":{"
            << "\"maxGrossExposure\":"
            << risk.limits
                   .max_gross_exposure
            << ",\"maxNetExposure\":"
            << risk.limits
                   .max_net_exposure
            << ",\"maxConcentrationPercent\":"
            << risk.limits
                   .max_concentration_percent
            << ",\"maxDailyLoss\":"
            << risk.limits
                   .max_daily_loss
            << "}"
            << "}";

        return json_response(
            http::status::ok,
            out.str()
        );
    }


    if (
        req.method() ==
            http::verb::post &&
        target ==
            "/api/portfolio/risk/limits"
    ) {
        try {
            const auto fields =
                parse_json_like_body(
                    req.body()
                );

            auto limits =
                state.portfolio_risk
                    .limits();

            if (
                fields.count(
                    "maxGrossExposure"
                )
            ) {
                limits.max_gross_exposure =
                    std::stod(
                        fields.at(
                            "maxGrossExposure"
                        )
                    );
            }

            if (
                fields.count(
                    "maxNetExposure"
                )
            ) {
                limits.max_net_exposure =
                    std::stod(
                        fields.at(
                            "maxNetExposure"
                        )
                    );
            }

            if (
                fields.count(
                    "maxConcentrationPercent"
                )
            ) {
                limits
                    .max_concentration_percent =
                    std::stod(
                        fields.at(
                            "maxConcentrationPercent"
                        )
                    );
            }

            if (
                fields.count(
                    "maxDailyLoss"
                )
            ) {
                limits.max_daily_loss =
                    std::stod(
                        fields.at(
                            "maxDailyLoss"
                        )
                    );
            }

            state.portfolio_risk
                .set_limits(
                    limits
                );

            state.settings_store
                .save_portfolio_limits(
                    limits
                );

            const auto risk_status =
                state.portfolio_risk
                    .evaluate(
                        state.positions
                            .positions()
                    );

            if (
                risk_status.breached
            ) {
                state.exchange
                    .halt_all();
            }

            return json_response(
                http::status::ok,
                R"({"ok":true})"
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
            "/api/positions"
    ) {
        {
            const auto quotes =
                minimatch::
                    read_live_router_quotes(
                        "btcusd"
                    );

            double best_bid = 0.0;
            double best_ask = 0.0;

            for (const auto& quote : quotes) {
                if (
                    !quote.healthy ||
                    quote.bids.empty() ||
                    quote.asks.empty()
                ) {
                    continue;
                }

                const double bid =
                    quote.bids.front().price;

                const double ask =
                    quote.asks.front().price;

                if (
                    best_bid == 0.0 ||
                    bid > best_bid
                ) {
                    best_bid = bid;
                }

                if (
                    best_ask == 0.0 ||
                    ask < best_ask
                ) {
                    best_ask = ask;
                }
            }

            if (
                best_bid > 0.0 &&
                best_ask > 0.0
            ) {
                state.positions.mark(
                    "btcusd",
                    (
                        best_bid +
                        best_ask
                    ) /
                    2.0
                );
            }
        }

        const auto positions =
            state.positions.positions();

        std::ostringstream out;
        out << "[";

        for (
            std::size_t i = 0;
            i < positions.size();
            ++i
        ) {
            if (i) {
                out << ",";
            }

            const auto& position =
                positions[i];

            out
                << "{"
                << "\"symbol\":\""
                << position.symbol
                << "\""
                << ",\"netQuantity\":"
                << position.net_quantity
                << ",\"averageCost\":"
                << position.average_cost
                << ",\"markPrice\":"
                << position.mark_price
                << ",\"realizedPnl\":"
                << position.realized_pnl
                << ",\"unrealizedPnl\":"
                << position.unrealized_pnl
                << ",\"totalPnl\":"
                << (
                    position.realized_pnl +
                    position.unrealized_pnl
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
            "/api/positions/",
            0
        ) == 0
    ) {
        const std::string symbol =
            target.substr(
                std::string(
                    "/api/positions/"
                ).size()
            );

        if (symbol.empty()) {
            return json_response(
                http::status::bad_request,
                R"({"error":"symbol required"})"
            );
        }

        {
            const auto quotes =
                minimatch::
                    read_live_router_quotes(
                        symbol
                    );

            double best_bid = 0.0;
            double best_ask = 0.0;

            for (const auto& quote : quotes) {
                if (
                    !quote.healthy ||
                    quote.bids.empty() ||
                    quote.asks.empty()
                ) {
                    continue;
                }

                const double bid =
                    quote.bids.front().price;

                const double ask =
                    quote.asks.front().price;

                if (
                    best_bid == 0.0 ||
                    bid > best_bid
                ) {
                    best_bid = bid;
                }

                if (
                    best_ask == 0.0 ||
                    ask < best_ask
                ) {
                    best_ask = ask;
                }
            }

            if (
                best_bid > 0.0 &&
                best_ask > 0.0
            ) {
                state.positions.mark(
                    symbol,
                    (
                        best_bid +
                        best_ask
                    ) /
                    2.0
                );
            }
        }

        const auto position =
            state.positions.position(
                symbol
            );

        std::ostringstream out;

        out
            << "{"
            << "\"symbol\":\""
            << position.symbol
            << "\""
            << ",\"netQuantity\":"
            << position.net_quantity
            << ",\"averageCost\":"
            << position.average_cost
            << ",\"markPrice\":"
            << position.mark_price
            << ",\"realizedPnl\":"
            << position.realized_pnl
            << ",\"unrealizedPnl\":"
            << position.unrealized_pnl
            << ",\"totalPnl\":"
            << (
                position.realized_pnl +
                position.unrealized_pnl
            )
            << "}";

        return json_response(
            http::status::ok,
            out.str()
        );
    }


    if (
        req.method() ==
            http::verb::get &&
        target ==
            "/api/algo-orders"
    ) {
        const auto orders =
            state.algo_scheduler.orders();

        std::ostringstream out;
        out << "[";

        for (
            std::size_t i = 0;
            i < orders.size();
            ++i
        ) {
            if (i) {
                out << ",";
            }

            const auto& order =
                orders[i];

            out
                << "{"
                << "\"parentOrderId\":\""
                << order.parent_order_id
                << "\""
                << ",\"status\":\""
                << minimatch::to_string(
                       order.status
                   )
                << "\""
                << ",\"symbol\":\""
                << order.symbol
                << "\""
                << ",\"currentSlice\":"
                << order.current_slice
                << ",\"totalSlices\":"
                << order.total_slices
                << ",\"startedAtNs\":"
                << order.started_at_ns
                << ",\"nextSliceAtNs\":"
                << order.next_slice_at_ns
                << ",\"completedAtNs\":"
                << order.completed_at_ns
                << ",\"requestedQuantity\":"
                << order.requested_quantity
                << ",\"filledQuantity\":"
                << order.filled_quantity
                << ",\"remainingQuantity\":"
                << order.remaining_quantity
                << ",\"recovered\":"
                << (
                    order.recovered
                        ? "true"
                        : "false"
                )
                << ",\"recoveryCount\":"
                << order.recovery_count
                << ",\"lastRecoveredAtNs\":"
                << order.last_recovered_at_ns
                << ",\"arrivalPrice\":"
                << order.arrival_price
                << ",\"error\":\""
                << order.error
                << "\""
                << "}";
        }

        out << "]";

        return json_response(
            http::status::ok,
            out.str()
        );
    }

    if (
        target.rfind(
            "/api/algo-orders/",
            0
        ) == 0
    ) {
        try {
            const std::string suffix =
                target.substr(
                    std::string(
                        "/api/algo-orders/"
                    ).size()
                );

            const std::string cancel_suffix =
                "/cancel";

            if (
                req.method() ==
                    http::verb::post &&
                suffix.size() >
                    cancel_suffix.size() &&
                suffix.ends_with(
                    cancel_suffix
                )
            ) {
                const auto id_text =
                    suffix.substr(
                        0,
                        suffix.size() -
                            cancel_suffix.size()
                    );

                const auto parent_id =
                    static_cast<
                        ParentOrderId
                    >(
                        std::stoull(
                            id_text
                        )
                    );

                const bool cancelled =
                    state.algo_scheduler
                        .cancel(
                            parent_id
                        );

                return json_response(
                    cancelled
                        ? http::status::ok
                        : http::status::
                              not_found,
                    cancelled
                        ? R"({"ok":true})"
                        : R"({"error":"algo order not found or not running"})"
                );
            }

            if (
                req.method() ==
                    http::verb::get
            ) {
                const auto parent_id =
                    static_cast<
                        ParentOrderId
                    >(
                        std::stoull(
                            suffix
                        )
                    );

                const auto order =
                    state.algo_scheduler
                        .find(
                            parent_id
                        );

                if (!order) {
                    return json_response(
                        http::status::
                            not_found,
                        R"({"error":"algo order not found"})"
                    );
                }

                std::ostringstream out;

                out
                    << "{"
                    << "\"parentOrderId\":\""
                    << order
                           ->parent_order_id
                    << "\""
                    << ",\"status\":\""
                    << minimatch::to_string(
                           order->status
                       )
                    << "\""
                    << ",\"symbol\":\""
                    << order->symbol
                    << "\""
                    << ",\"currentSlice\":"
                    << order
                           ->current_slice
                    << ",\"totalSlices\":"
                    << order
                           ->total_slices
                    << ",\"startedAtNs\":"
                    << order
                           ->started_at_ns
                    << ",\"nextSliceAtNs\":"
                    << order
                           ->next_slice_at_ns
                    << ",\"completedAtNs\":"
                    << order
                           ->completed_at_ns
                    << ",\"requestedQuantity\":"
                    << order
                           ->requested_quantity
                    << ",\"filledQuantity\":"
                    << order
                           ->filled_quantity
                    << ",\"remainingQuantity\":"
                    << order
                           ->remaining_quantity
                    << ",\"arrivalPrice\":"
                    << order
                           ->arrival_price
                    << ",\"error\":\""
                    << order->error
                    << "\""
                    << "}";

                return json_response(
                    http::status::ok,
                    out.str()
                );
            }

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

        unsigned short port =
            8081;

        if (
            const char* configured =
                std::getenv(
                    "MINIMATCH_API_PORT"
                )
        ) {
            port =
                static_cast<
                    unsigned short
                >(
                    std::stoul(
                        configured
                    )
                );
        }

        tcp::acceptor acceptor(
            io,
            {
                asio::ip::make_address(
                    "127.0.0.1"
                ),
                port
            }
        );

        std::cout
            << "MiniMatch API listening on "
            << "http://127.0.0.1:"
            << port
            << "\n";

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
