#include "minimatch/market_recorder.hpp"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>

namespace {

void usage(const char* program) {
  std::cerr
      << "Usage: "
      << program
      << " <recording.bin>"
      << " [symbol]"
      << " [speed]"
      << " [maximum-records]\n"
      << "\n"
      << "speed=0 replays as fast as possible\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc < 2) {
      usage(argv[0]);
      return EXIT_FAILURE;
    }

    const std::string path = argv[1];

    const std::string symbol =
        argc > 2
            ? argv[2]
            : "BTC-USD";

    const double speed =
        argc > 3
            ? std::stod(argv[3])
            : 0.0;

    const std::size_t maximum_records =
        argc > 4
            ? static_cast<std::size_t>(
                  std::stoull(argv[4])
              )
            : 0;

    minimatch::MarketRecordReader reader(
        path
    );

    minimatch::ConsolidatedMarketData market;

    const auto result =
        minimatch::replay_market_records(
            reader,
            market,
            minimatch::MarketReplayOptions{
                .speed = speed,
                .maximum_records =
                    maximum_records
            }
        );

    const auto checksum =
        minimatch::market_state_checksum(
            market,
            symbol
        );

    const auto bbo =
        market.consolidated_bbo(symbol);

    std::cout
        << std::fixed
        << std::setprecision(8)
        << "records_read="
        << result.records_read
        << '\n'
        << "records_applied="
        << result.records_applied
        << '\n'
        << "snapshots_applied="
        << result.snapshots_applied
        << '\n'
        << "update_batches_applied="
        << result.update_batches_applied
        << '\n'
        << "rejected_records="
        << result.rejected_records
        << '\n'
        << "first_timestamp_ns="
        << result.first_timestamp_ns
        << '\n'
        << "last_timestamp_ns="
        << result.last_timestamp_ns
        << '\n'
        << "elapsed_wall_time_ns="
        << result.elapsed_wall_time_ns
        << '\n'
        << "records_per_second="
        << result.records_per_second
        << '\n'
        << "state_checksum="
        << checksum
        << '\n'
        << "bbo_valid="
        << (
               bbo.valid
                   ? "true"
                   : "false"
           )
        << '\n';

    if (bbo.valid) {
      std::cout
          << "bid_price="
          << bbo.bid_price
          << '\n'
          << "bid_quantity="
          << bbo.bid_quantity
          << '\n'
          << "bid_venue="
          << bbo.bid_venue
          << '\n'
          << "ask_price="
          << bbo.ask_price
          << '\n'
          << "ask_quantity="
          << bbo.ask_quantity
          << '\n'
          << "ask_venue="
          << bbo.ask_venue
          << '\n'
          << "midpoint="
          << bbo.midpoint
          << '\n'
          << "spread="
          << bbo.spread
          << '\n';
    }

    return result.rejected_records == 0
        ? EXIT_SUCCESS
        : EXIT_FAILURE;
  } catch (const std::exception& error) {
    std::cerr
        << "Market replay failed: "
        << error.what()
        << '\n';

    return EXIT_FAILURE;
  }
}
