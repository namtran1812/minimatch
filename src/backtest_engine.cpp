#include "minimatch/backtest_engine.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace minimatch {

namespace {

constexpr double epsilon = 1e-12;

bool symbol_matches(
    const MarketReplayEvent& event,
    const std::string& symbol
) {
  return event.symbol == symbol;
}

double quote_price(
    const MarketReplayEvent& event,
    RouteSide side
) {
  return side == RouteSide::Buy
      ? event.ask_price
      : event.bid_price;
}

double quote_quantity(
    const MarketReplayEvent& event,
    RouteSide side
) {
  return side == RouteSide::Buy
      ? event.ask_quantity
      : event.bid_quantity;
}

}  // namespace

BacktestResult run_historical_backtest(
    const BacktestRequest& request,
    const MarketReplay& replay,
    OrderManagementSystem* oms
) {
  if (request.symbol.empty()) {
    throw std::invalid_argument(
        "backtest symbol cannot be empty"
    );
  }

  if (!std::isfinite(request.quantity) ||
      request.quantity <= 0.0) {
    throw std::invalid_argument(
        "backtest quantity must be positive"
    );
  }

  const auto schedule =
      build_execution_schedule(request.schedule);

  BacktestResult result;
  result.requested_quantity = request.quantity;

  OrderManagementSystem local_oms;
  OrderManagementSystem& order_manager =
      oms == nullptr ? local_oms : *oms;

  const ParentOrderId parent_order_id =
      order_manager.create_parent(
          ParentOrderRequest{
              .symbol = request.symbol,
              .side = request.side,
              .quantity = request.quantity,
              .algorithm = request.schedule.algorithm
          },
          replay.stats().first_timestamp_ns
      );

  result.parent_order_id = parent_order_id;

  std::vector<const MarketReplayEvent*> quotes;
  quotes.reserve(replay.size());

  double traded_notional = 0.0;
  double traded_quantity = 0.0;

  for (const auto& event : replay.events()) {
    if (!symbol_matches(event, request.symbol)) {
      continue;
    }

    if (event.type == MarketEventType::Quote) {
      quotes.push_back(&event);
    } else if (event.type == MarketEventType::Trade) {
      traded_notional +=
          event.trade_price *
          event.trade_quantity;

      traded_quantity +=
          event.trade_quantity;
    }
  }

  if (quotes.empty()) {
    throw std::runtime_error(
        "no matching quote events in replay"
    );
  }

  result.arrival_price =
      quote_price(*quotes.front(), request.side);

  double remaining_parent = request.quantity;

  for (std::size_t slice_index = 0;
       slice_index < schedule.size();
       ++slice_index) {
    if (remaining_parent <= epsilon) {
      break;
    }

    const auto& slice = schedule[slice_index];

    const std::uint64_t target_timestamp =
        replay.stats().first_timestamp_ns +
        static_cast<std::uint64_t>(
            slice.delay_seconds * 1'000'000'000.0
        );

    const MarketReplayEvent* selected = nullptr;

    for (const auto* quote : quotes) {
      if (quote->timestamp_ns >= target_timestamp) {
        selected = quote;
        break;
      }
    }

    if (selected == nullptr) {
      break;
    }

    const double available =
        quote_quantity(*selected, request.side);

    const double requested_child =
        std::min(
            slice.quantity,
            remaining_parent
        );

    const double filled =
        std::min(requested_child, available);

    const double price =
        quote_price(*selected, request.side);

    const double notional =
        filled * price;

    const double fee =
        notional *
        request.taker_fee_bps /
        10000.0;

    const ChildOrderId child_order_id =
        order_manager.create_child(
            ChildOrderRequest{
                .parent_id = parent_order_id,
                .venue = selected->venue,
                .price = price,
                .quantity = requested_child,
                .created_timestamp_ns =
                    selected->timestamp_ns
            }
        );

    order_manager.mark_child_working(
        child_order_id,
        selected->timestamp_ns
    );

    std::uint64_t execution_report_id = 0;

    if (filled > epsilon) {
      const auto execution_report =
          order_manager.record_fill(
              child_order_id,
              price,
              filled,
              fee,
              selected->timestamp_ns
          );

      execution_report_id =
          execution_report.execution_report_id;
    }

    if (filled + epsilon < requested_child) {
      order_manager.cancel_child(
          child_order_id,
          selected->timestamp_ns
      );
    }

    result.fills.push_back(
        BacktestFill{
            .slice_index = slice_index,
            .parent_order_id = parent_order_id,
            .child_order_id = child_order_id,
            .execution_report_id =
                execution_report_id,
            .timestamp_ns =
                selected->timestamp_ns,
            .requested_quantity =
                requested_child,
            .filled_quantity = filled,
            .remaining_quantity =
                requested_child - filled,
            .price = price,
            .notional = notional,
            .fee = fee
        }
    );

    result.filled_quantity += filled;
    result.total_notional += notional;
    result.total_fees += fee;

    remaining_parent -= filled;
  }

  result.remaining_quantity =
      std::max(
          0.0,
          request.quantity -
              result.filled_quantity
      );

  result.complete =
      result.remaining_quantity <= epsilon;

  if (!result.complete) {
    order_manager.cancel_parent(
        parent_order_id,
        replay.stats().last_timestamp_ns
    );
  }

  const auto parent =
      order_manager.find_parent(
          parent_order_id
      );

  if (parent.has_value()) {
    result.parent_status = parent->status;
  }

  if (result.filled_quantity > epsilon) {
    result.average_fill_price =
        result.total_notional /
        result.filled_quantity;
  }

  if (traded_quantity > epsilon) {
    result.market_vwap =
        traded_notional /
        traded_quantity;
  }

  if (result.arrival_price > epsilon &&
      result.average_fill_price > epsilon) {
    const double signed_difference =
        request.side == RouteSide::Buy
            ? result.average_fill_price -
                  result.arrival_price
            : result.arrival_price -
                  result.average_fill_price;

    result.implementation_shortfall_bps =
        signed_difference /
        result.arrival_price *
        10000.0;
  }

  if (result.market_vwap > epsilon &&
      result.average_fill_price > epsilon) {
    const double signed_difference =
        request.side == RouteSide::Buy
            ? result.average_fill_price -
                  result.market_vwap
            : result.market_vwap -
                  result.average_fill_price;

    result.vwap_slippage_bps =
        signed_difference /
        result.market_vwap *
        10000.0;
  }

  return result;
}

}  // namespace minimatch
