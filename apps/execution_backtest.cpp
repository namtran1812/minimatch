#include "minimatch/backtest_engine.hpp"
#include "minimatch/execution_algo.hpp"
#include "minimatch/market_replay.hpp"
#include "minimatch/router.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Arguments {
  std::string market_path;
  std::string algorithm{"TWAP"};
  std::string symbol{"btcusd"};
  std::string side{"buy"};

  double quantity{1.0};
  int slices{5};
  double duration_seconds{60.0};

  double taker_fee_bps{0.0};
  double latency_ms{0.0};
  double latency_cost_bps_per_ms{0.0};

  double participation_rate{0.1};
  double displayed_quantity{0.0};

  std::vector<double> volume_profile;
};

std::string uppercase(std::string value) {
  std::transform(
      value.begin(),
      value.end(),
      value.begin(),
      [](unsigned char character) {
        return static_cast<char>(
            std::toupper(character)
        );
      }
  );

  return value;
}

std::vector<double> parse_profile(
    const std::string& value
) {
  std::vector<double> profile;
  std::string token;

  for (char character : value) {
    if (character == ',') {
      if (!token.empty()) {
        profile.push_back(std::stod(token));
        token.clear();
      }

      continue;
    }

    token.push_back(character);
  }

  if (!token.empty()) {
    profile.push_back(std::stod(token));
  }

  return profile;
}

minimatch::ExecutionAlgorithm parse_algorithm(
    const std::string& value
) {
  const std::string normalized = uppercase(value);

  if (normalized == "MARKET") {
    return minimatch::ExecutionAlgorithm::Market;
  }

  if (normalized == "TWAP") {
    return minimatch::ExecutionAlgorithm::TWAP;
  }

  if (normalized == "VWAP") {
    return minimatch::ExecutionAlgorithm::VWAP;
  }

  if (normalized == "POV") {
    return minimatch::ExecutionAlgorithm::POV;
  }

  if (normalized == "ICEBERG") {
    return minimatch::ExecutionAlgorithm::Iceberg;
  }

  throw std::invalid_argument(
      "unsupported algorithm: " + value
  );
}

minimatch::RouteSide parse_side(
    const std::string& value
) {
  const std::string normalized = uppercase(value);

  if (normalized == "BUY") {
    return minimatch::RouteSide::Buy;
  }

  if (normalized == "SELL") {
    return minimatch::RouteSide::Sell;
  }

  throw std::invalid_argument(
      "side must be buy or sell"
  );
}

void print_usage(const char* executable) {
  std::cerr
      << "Usage:\n"
      << "  " << executable
      << " --market PATH"
      << " --algo MARKET|TWAP|VWAP|POV|ICEBERG"
      << " --symbol SYMBOL"
      << " --side buy|sell"
      << " --qty QUANTITY"
      << " [--slices N]"
      << " [--duration SECONDS]"
      << " [--volume-profile 1,2,4,3]"
      << " [--participation-rate 0.1]"
      << " [--displayed-quantity 0.25]"
      << " [--fee-bps 1.0]"
      << " [--latency-ms 0.0]"
      << " [--latency-cost-bps-per-ms 0.0]\n";
}

Arguments parse_arguments(
    int argc,
    char** argv
) {
  Arguments arguments;

  for (int index = 1;
       index < argc;
       ++index) {
    const std::string option = argv[index];

    const auto require_value = [&]() -> std::string {
      if (index + 1 >= argc) {
        throw std::invalid_argument(
            "missing value for " + option
        );
      }

      return argv[++index];
    };

    if (option == "--market") {
      arguments.market_path = require_value();
    } else if (option == "--algo") {
      arguments.algorithm = require_value();
    } else if (option == "--symbol") {
      arguments.symbol = require_value();
    } else if (option == "--side") {
      arguments.side = require_value();
    } else if (option == "--qty") {
      arguments.quantity =
          std::stod(require_value());
    } else if (option == "--slices") {
      arguments.slices =
          std::stoi(require_value());
    } else if (option == "--duration") {
      arguments.duration_seconds =
          std::stod(require_value());
    } else if (option == "--volume-profile") {
      arguments.volume_profile =
          parse_profile(require_value());
    } else if (option == "--participation-rate") {
      arguments.participation_rate =
          std::stod(require_value());
    } else if (option == "--displayed-quantity") {
      arguments.displayed_quantity =
          std::stod(require_value());
    } else if (option == "--fee-bps") {
      arguments.taker_fee_bps =
          std::stod(require_value());
    } else if (option == "--latency-ms") {
      arguments.latency_ms =
          std::stod(require_value());
    } else if (
        option ==
        "--latency-cost-bps-per-ms"
    ) {
      arguments.latency_cost_bps_per_ms =
          std::stod(require_value());
    } else if (
        option == "--help" ||
        option == "-h"
    ) {
      print_usage(argv[0]);
      std::exit(0);
    } else {
      throw std::invalid_argument(
          "unknown argument: " + option
      );
    }
  }

  if (arguments.market_path.empty()) {
    throw std::invalid_argument(
        "--market is required"
    );
  }

  return arguments;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const Arguments arguments =
        parse_arguments(argc, argv);

    const auto algorithm =
        parse_algorithm(arguments.algorithm);

    const auto side =
        parse_side(arguments.side);

    const minimatch::MarketReplay replay =
        minimatch::MarketReplay::from_csv(
            arguments.market_path
        );

    const minimatch::ExecutionScheduleRequest schedule{
        .algorithm = algorithm,
        .quantity = arguments.quantity,
        .slices = arguments.slices,
        .duration_seconds =
            arguments.duration_seconds,
        .volume_profile =
            arguments.volume_profile,
        .participation_rate =
            arguments.participation_rate,
        .displayed_quantity =
            arguments.displayed_quantity
    };

    const minimatch::BacktestRequest request{
        .symbol = arguments.symbol,
        .side = side,
        .quantity = arguments.quantity,
        .schedule = schedule,
        .taker_fee_bps =
            arguments.taker_fee_bps,
        .latency_ms =
            arguments.latency_ms,
        .latency_cost_bps_per_ms =
            arguments.latency_cost_bps_per_ms
    };

    const minimatch::BacktestResult result =
        minimatch::run_historical_backtest(
            request,
            replay
        );

    const double fill_rate =
        result.requested_quantity > 0.0
            ? result.filled_quantity /
                  result.requested_quantity
            : 0.0;

    std::cout
        << std::fixed
        << std::setprecision(8)
        << "algorithm="
        << uppercase(arguments.algorithm)
        << '\n'
        << "symbol="
        << arguments.symbol
        << '\n'
        << "side="
        << uppercase(arguments.side)
        << '\n'
        << "complete="
        << (result.complete ? "true" : "false")
        << '\n'
        << "parent_order_id="
        << result.parent_order_id
        << '\n'
        << "parent_status="
        << minimatch::to_string(
               result.parent_status
           )
        << '\n'
        << "requested_quantity="
        << result.requested_quantity
        << '\n'
        << "filled_quantity="
        << result.filled_quantity
        << '\n'
        << "remaining_quantity="
        << result.remaining_quantity
        << '\n'
        << "fill_rate="
        << fill_rate
        << '\n'
        << "arrival_price="
        << result.arrival_price
        << '\n'
        << "average_fill_price="
        << result.average_fill_price
        << '\n'
        << "market_vwap="
        << result.market_vwap
        << '\n'
        << "implementation_shortfall_bps="
        << result.implementation_shortfall_bps
        << '\n'
        << "vwap_slippage_bps="
        << result.vwap_slippage_bps
        << '\n'
        << "total_notional="
        << result.total_notional
        << '\n'
        << "total_fees="
        << result.total_fees
        << '\n'
        << "fill_count="
        << result.fills.size()
        << '\n';

    for (const auto& fill : result.fills) {
      std::cout
          << "fill"
          << " slice=" << fill.slice_index
          << " timestamp_ns="
          << fill.timestamp_ns
          << " requested="
          << fill.requested_quantity
          << " filled="
          << fill.filled_quantity
          << " remaining="
          << fill.remaining_quantity
          << " price="
          << fill.price
          << " fee="
          << fill.fee
          << '\n';
    }

    return result.complete ? 0 : 2;
  } catch (const std::exception& error) {
    std::cerr
        << "execution backtest failed: "
        << error.what()
        << '\n';

    print_usage(argv[0]);
    return 1;
  }
}
