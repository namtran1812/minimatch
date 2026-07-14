#include "minimatch/router.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace minimatch {

namespace {

struct CandidateLevel {
  std::string venue;

  double price{0.0};
  double quantity{0.0};

  double taker_fee_bps{0.0};
  double latency_ms{0.0};
  double latency_cost_bps_per_ms{0.0};

  double effective_price{0.0};

  std::size_t level_index{0};
};

double calculate_effective_price(
    double price,
    RouteSide side,
    double taker_fee_bps,
    double latency_ms,
    double latency_cost_bps_per_ms
) {
  const double total_cost_bps =
      taker_fee_bps +
      latency_ms * latency_cost_bps_per_ms;

  const double cost_rate =
      total_cost_bps / 10000.0;

  if (side == RouteSide::Buy) {
    return price * (1.0 + cost_rate);
  }

  return price * (1.0 - cost_rate);
}

bool valid_level(const VenueLevel& level) {
  return std::isfinite(level.price) &&
         std::isfinite(level.quantity) &&
         level.price > 0.0 &&
         level.quantity > 0.0;
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

  std::vector<CandidateLevel> candidates;

  for (const auto& quote : quotes) {
    if (!quote.healthy) {
      continue;
    }

    const auto& levels =
        request.side == RouteSide::Buy
            ? quote.asks
            : quote.bids;

    for (std::size_t index = 0;
         index < levels.size();
         ++index) {
      const VenueLevel& level = levels[index];

      if (!valid_level(level)) {
        continue;
      }

      candidates.push_back(
          CandidateLevel{
              quote.venue,
              level.price,
              level.quantity,
              quote.taker_fee_bps,
              quote.latency_ms,
              quote.latency_cost_bps_per_ms,
              calculate_effective_price(
                  level.price,
                  request.side,
                  quote.taker_fee_bps,
                  quote.latency_ms,
                  quote.latency_cost_bps_per_ms
              ),
              index
          }
      );
    }
  }

  std::sort(
      candidates.begin(),
      candidates.end(),
      [&](const CandidateLevel& left,
          const CandidateLevel& right) {
        if (left.effective_price !=
            right.effective_price) {
          return request.side == RouteSide::Buy
              ? left.effective_price <
                    right.effective_price
              : left.effective_price >
                    right.effective_price;
        }

        if (left.latency_ms != right.latency_ms) {
          return left.latency_ms < right.latency_ms;
        }

        if (left.venue != right.venue) {
          return left.venue < right.venue;
        }

        return left.level_index < right.level_index;
      }
  );

  RoutePlan plan;
  plan.requested_quantity = request.quantity;

  double remaining = request.quantity;

  for (const auto& candidate : candidates) {
    if (remaining <= 1e-12) {
      break;
    }

    const double routed =
        std::min(remaining, candidate.quantity);

    const double notional =
        routed * candidate.price;

    const double fee =
        notional *
        candidate.taker_fee_bps /
        10000.0;

    plan.legs.push_back(
        RouteLeg{
            candidate.venue,
            candidate.price,
            routed,
            fee,
            candidate.effective_price,
            candidate.latency_ms,
            candidate.taker_fee_bps,
            candidate.latency_cost_bps_per_ms,
            candidate.level_index
        }
    );

    plan.routed_quantity += routed;
    plan.estimated_notional += notional;
    plan.estimated_fees += fee;

    remaining -= routed;
  }

  plan.complete =
      plan.routed_quantity + 1e-12 >=
      request.quantity;

  if (plan.routed_quantity > 0.0) {
    plan.average_price =
        plan.estimated_notional /
        plan.routed_quantity;
  }

  return plan;
}

}  // namespace minimatch
