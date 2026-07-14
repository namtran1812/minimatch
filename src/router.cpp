#include "minimatch/router.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace minimatch {

namespace {

double executable_price(
    const VenueQuote& quote,
    RouteSide side
) {
  return side == RouteSide::Buy
      ? quote.ask
      : quote.bid;
}

double executable_quantity(
    const VenueQuote& quote,
    RouteSide side
) {
  return side == RouteSide::Buy
      ? quote.ask_quantity
      : quote.bid_quantity;
}

double effective_price(
    const VenueQuote& quote,
    RouteSide side
) {
  const double price = executable_price(quote, side);

  const double total_cost_bps =
      quote.taker_fee_bps +
      quote.latency_ms * quote.latency_cost_bps_per_ms;

  const double cost_rate = total_cost_bps / 10000.0;

  if (side == RouteSide::Buy) {
    return price * (1.0 + cost_rate);
  }

  return price * (1.0 - cost_rate);
}

}  // namespace

RoutePlan build_route_plan(
    const RouteRequest& request,
    const std::vector<VenueQuote>& quotes
) {
  if (!std::isfinite(request.quantity) ||
      request.quantity <= 0.0) {
    throw std::invalid_argument(
        "route quantity must be positive"
    );
  }

  std::vector<VenueQuote> candidates;

  for (const auto& quote : quotes) {
    const double price =
        executable_price(quote, request.side);

    const double quantity =
        executable_quantity(quote, request.side);

    if (!quote.healthy ||
        !std::isfinite(price) ||
        !std::isfinite(quantity) ||
        price <= 0.0 ||
        quantity <= 0.0) {
      continue;
    }

    candidates.push_back(quote);
  }

  std::sort(
      candidates.begin(),
      candidates.end(),
      [&](const VenueQuote& left,
          const VenueQuote& right) {
        const double left_effective =
            effective_price(left, request.side);

        const double right_effective =
            effective_price(right, request.side);

        if (left_effective != right_effective) {
          return request.side == RouteSide::Buy
              ? left_effective < right_effective
              : left_effective > right_effective;
        }

        if (left.latency_ms != right.latency_ms) {
          return left.latency_ms < right.latency_ms;
        }

        return left.venue < right.venue;
      }
  );

  RoutePlan plan;
  plan.requested_quantity = request.quantity;

  double remaining = request.quantity;

  for (const auto& quote : candidates) {
    if (remaining <= 0.0) {
      break;
    }

    const double price =
        executable_price(quote, request.side);

    const double available =
        executable_quantity(quote, request.side);

    const double routed =
        std::min(remaining, available);

    const double notional = routed * price;
    const double fee =
        notional * quote.taker_fee_bps / 10000.0;

    plan.legs.push_back(
        RouteLeg{
            quote.venue,
            price,
            routed,
            fee,
            effective_price(quote, request.side),
            quote.latency_ms,
            quote.taker_fee_bps
        }
    );

    plan.routed_quantity += routed;
    plan.estimated_notional += notional;
    plan.estimated_fees += fee;
    remaining -= routed;
  }

  plan.complete =
      plan.routed_quantity + 1e-12 >= request.quantity;

  if (plan.routed_quantity > 0.0) {
    plan.average_price =
        plan.estimated_notional /
        plan.routed_quantity;
  }

  return plan;
}

}  // namespace minimatch
