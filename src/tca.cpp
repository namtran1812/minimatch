#include "minimatch/tca.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>
#include <utility>

namespace minimatch {
namespace {

constexpr double epsilon = 1e-12;

void require_positive_finite(
    double value,
    const char* name
) {
  if (!std::isfinite(value) || value <= 0.0) {
    throw std::invalid_argument(
        std::string(name) + " must be positive and finite"
    );
  }
}

double signed_price_cost(
    RouteSide side,
    double actual_price,
    double reference_price,
    double quantity
) {
  const double difference =
      side == RouteSide::Buy
          ? actual_price - reference_price
          : reference_price - actual_price;

  return difference * quantity;
}

double signed_slippage_bps(
    RouteSide side,
    double actual_price,
    double reference_price
) {
  if (reference_price <= 0.0) {
    return 0.0;
  }

  const double difference =
      side == RouteSide::Buy
          ? actual_price - reference_price
          : reference_price - actual_price;

  return difference / reference_price * 10000.0;
}

struct VenueAccumulator {
  double quantity{0.0};
  double notional{0.0};
  double fees{0.0};
};

}  // namespace

TcaReport analyze_execution(
    const RouteRequest& request,
    const RoutePlan& route_plan,
    const std::vector<ExecutionFill>& fills,
    double arrival_price,
    double market_vwap,
    double ending_price
) {
  require_positive_finite(
      request.quantity,
      "ordered quantity"
  );

  require_positive_finite(
      arrival_price,
      "arrival price"
  );

  require_positive_finite(
      market_vwap,
      "market VWAP"
  );

  require_positive_finite(
      ending_price,
      "ending price"
  );

  TcaReport report;
  report.side = request.side;
  report.ordered_quantity = request.quantity;
  report.arrival_price = arrival_price;
  report.market_vwap = market_vwap;
  report.ending_price = ending_price;

  std::map<std::string, VenueAccumulator>
      venue_totals;

  for (const auto& fill : fills) {
    require_positive_finite(
        fill.price,
        "fill price"
    );

    require_positive_finite(
        fill.quantity,
        "fill quantity"
    );

    if (!std::isfinite(fill.fee) ||
        fill.fee < 0.0) {
      throw std::invalid_argument(
          "fill fee must be non-negative and finite"
      );
    }

    if (fill.venue.empty()) {
      throw std::invalid_argument(
          "fill venue must not be empty"
      );
    }

    if (report.filled_quantity +
            fill.quantity >
        request.quantity + epsilon) {
      throw std::invalid_argument(
          "filled quantity exceeds ordered quantity"
      );
    }

    const double notional =
        fill.price * fill.quantity;

    report.filled_quantity +=
        fill.quantity;

    report.gross_notional +=
        notional;

    report.fees +=
        fill.fee;

    auto& venue =
        venue_totals[fill.venue];

    venue.quantity += fill.quantity;
    venue.notional += notional;
    venue.fees += fill.fee;
  }

  report.unfilled_quantity =
      std::max(
          0.0,
          report.ordered_quantity -
              report.filled_quantity
      );

  report.fill_rate =
      report.filled_quantity /
      report.ordered_quantity;

  if (report.filled_quantity > epsilon) {
    report.average_fill_price =
        report.gross_notional /
        report.filled_quantity;

    report.arrival_slippage_bps =
        signed_slippage_bps(
            report.side,
            report.average_fill_price,
            arrival_price
        );

    report.vwap_slippage_bps =
        signed_slippage_bps(
            report.side,
            report.average_fill_price,
            market_vwap
        );
  }

  report.realized_cost =
      signed_price_cost(
          report.side,
          report.average_fill_price,
          arrival_price,
          report.filled_quantity
      ) +
      report.fees;

  report.opportunity_cost =
      signed_price_cost(
          report.side,
          ending_price,
          arrival_price,
          report.unfilled_quantity
      );

  report.implementation_shortfall =
      report.realized_cost +
      report.opportunity_cost;

  const double arrival_notional =
      report.ordered_quantity *
      arrival_price;

  report.implementation_shortfall_bps =
      report.implementation_shortfall /
      arrival_notional *
      10000.0;

  double planned_effective_notional = 0.0;
  double planned_quantity = 0.0;

  for (const auto& leg : route_plan.legs) {
    if (!std::isfinite(leg.quantity) ||
        leg.quantity <= 0.0 ||
        !std::isfinite(leg.effective_price) ||
        leg.effective_price <= 0.0) {
      continue;
    }

    planned_effective_notional +=
        leg.effective_price *
        leg.quantity;

    planned_quantity +=
        leg.quantity;
  }

  if (planned_quantity > epsilon &&
      report.filled_quantity > epsilon) {
    const double planned_average =
        planned_effective_notional /
        planned_quantity;

    report.routing_regret =
        signed_price_cost(
            report.side,
            report.average_fill_price,
            planned_average,
            report.filled_quantity
        ) +
        report.fees;

    report.routing_regret_bps =
        report.routing_regret /
        (
            planned_average *
            report.filled_quantity
        ) *
        10000.0;
  }

  report.venues.reserve(
      venue_totals.size()
  );

  for (const auto& [name, aggregate] :
       venue_totals) {
    VenueTca venue;
    venue.venue = name;
    venue.filled_quantity =
        aggregate.quantity;
    venue.notional =
        aggregate.notional;
    venue.fees =
        aggregate.fees;

    venue.average_fill_price =
        aggregate.notional /
        aggregate.quantity;

    venue.arrival_slippage_bps =
        signed_slippage_bps(
            report.side,
            venue.average_fill_price,
            arrival_price
        );

    venue.vwap_slippage_bps =
        signed_slippage_bps(
            report.side,
            venue.average_fill_price,
            market_vwap
        );

    report.venues.push_back(
        std::move(venue)
    );
  }

  return report;
}

}  // namespace minimatch
