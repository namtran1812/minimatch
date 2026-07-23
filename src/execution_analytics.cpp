#include "minimatch/execution_analytics.hpp"

#include <utility>
#include <vector>

namespace minimatch {

ExecutionQuality ExecutionAnalytics::analyze(
    const ParentOrder &parent, const RoutePlan &route_plan,
    const std::vector<OmsExecutionReport> &fills, double arrival_price,
    double market_vwap, double ending_price,
    const std::vector<ExecutionMarkout> &markouts) const {
  RouteRequest request;
  request.side = parent.side;
  request.quantity = parent.quantity;

  std::vector<ExecutionFill> tca_fills;
  tca_fills.reserve(fills.size());

  for (const auto &fill : fills) {
    tca_fills.push_back(ExecutionFill{
        .order_id = fill.child_id,
        .venue = fill.venue,
        .price = fill.price,
        .quantity = fill.quantity,
        .fee = fill.fee,
        .timestamp_ns = fill.timestamp_ns,
    });
  }

  const TcaReport tca = analyze_execution(
      request, route_plan, tca_fills, arrival_price, market_vwap, ending_price);

  ExecutionQuality result;

  result.parent_order_id = parent.id;
  result.symbol = parent.symbol;
  result.side = parent.side;

  result.requested_quantity = tca.ordered_quantity;

  result.filled_quantity = tca.filled_quantity;

  result.unfilled_quantity = tca.unfilled_quantity;

  result.fill_rate = tca.fill_rate;

  result.arrival_price = tca.arrival_price;

  result.market_vwap = tca.market_vwap;

  result.ending_price = tca.ending_price;

  result.average_fill_price = tca.average_fill_price;

  result.total_notional = tca.gross_notional;

  result.total_fees = tca.fees;

  result.arrival_slippage_bps = tca.arrival_slippage_bps;

  result.vwap_slippage_bps = tca.vwap_slippage_bps;

  result.realized_cost = tca.realized_cost;

  result.opportunity_cost = tca.opportunity_cost;

  result.implementation_shortfall = tca.implementation_shortfall;

  result.implementation_shortfall_bps = tca.implementation_shortfall_bps;

  // TCA implementation shortfall already includes explicit fees.
  result.fee_adjusted_shortfall_bps = tca.implementation_shortfall_bps;

  result.routing_regret = tca.routing_regret;

  result.routing_regret_bps = tca.routing_regret_bps;

  result.venues = tca.venues;

  result.markouts = markouts;

  return result;
}


ExecutionQuality ExecutionAnalytics::analyze(
    const ParentOrder &parent,
    const std::vector<OmsExecutionReport> &fills,
    double arrival_price) const {
  RoutePlan empty_plan;

  double reference_price = arrival_price;

  if (reference_price <= 0.0) {
    for (const auto &fill : fills) {
      if (fill.price > 0.0) {
        reference_price = fill.price;
        break;
      }
    }
  }

  if (reference_price <= 0.0) {
    reference_price = 1.0;
  }

  return analyze(parent, empty_plan, fills, reference_price, reference_price,
                 reference_price);
}


} // namespace minimatch
