#include "minimatch/router.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <unordered_set>
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

  if (!std::isfinite(request.limit_price) ||
      request.limit_price < 0.0) {
    throw std::invalid_argument(
        "limit price must be non-negative"
    );
  }

  if (!std::isfinite(request.max_slippage_bps) ||
      request.max_slippage_bps < 0.0) {
    throw std::invalid_argument(
        "maximum slippage must be non-negative"
    );
  }

  if (request.max_venue_count == 0) {
    throw std::invalid_argument(
        "maximum venue count must be positive"
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

  if (candidates.empty()) {
    return plan;
  }

  const double arrival_effective_price =
      candidates.front().effective_price;

  const double slippage_fraction =
      request.max_slippage_bps / 10000.0;

  double remaining = request.quantity;
  std::unordered_set<std::string> used_venues;

  for (const auto& candidate : candidates) {
    if (remaining <= 1e-12) {
      break;
    }

    const bool violates_limit =
        request.limit_price > 0.0 &&
        (
            request.side == RouteSide::Buy
                ? candidate.price > request.limit_price
                : candidate.price < request.limit_price
        );

    if (violates_limit) {
      continue;
    }

    const bool violates_slippage =
        request.side == RouteSide::Buy
            ? candidate.effective_price >
                  arrival_effective_price *
                      (1.0 + slippage_fraction)
            : candidate.effective_price <
                  arrival_effective_price *
                      (1.0 - slippage_fraction);

    if (violates_slippage) {
      continue;
    }

    const bool new_venue =
        !used_venues.contains(candidate.venue);

    if (new_venue &&
        used_venues.size() >=
            request.max_venue_count) {
      continue;
    }

    const double routed =
        std::min(remaining, candidate.quantity);

    const double notional =
        routed * candidate.price;

    const double fee =
        notional *
        candidate.taker_fee_bps /
        10000.0;

    used_venues.insert(candidate.venue);

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

  if (request.all_or_none && !plan.complete) {
    plan.legs.clear();
    plan.routed_quantity = 0.0;
    plan.estimated_notional = 0.0;
    plan.estimated_fees = 0.0;
    plan.average_price = 0.0;
    return plan;
  }

  if (plan.routed_quantity > 0.0) {
    plan.average_price =
        plan.estimated_notional /
        plan.routed_quantity;
  }

  return plan;
}

}  // namespace minimatch
