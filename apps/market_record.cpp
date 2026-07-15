#include "minimatch/market_recorder.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

std::uint64_t now_ns() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<
          std::chrono::nanoseconds
      >(
          std::chrono::system_clock::now()
              .time_since_epoch()
      ).count()
  );
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const std::string path =
        argc > 1
            ? argv[1]
            : "market_session.bin";

    const std::size_t update_count =
        argc > 2
            ? static_cast<std::size_t>(
                  std::stoull(argv[2])
              )
            : 1000;

    minimatch::MarketRecorder recorder(
        path
    );

    recorder.write_snapshot(
        minimatch::MarketDataSnapshot{
            .venue = "SIMULATED",
            .symbol = "BTC-USD",
            .sequence = 0,
            .timestamp_ns = now_ns(),
            .bids = {
                {64'999.90, 2.0},
                {64'999.80, 3.0}
            },
            .asks = {
                {65'000.10, 2.0},
                {65'000.20, 3.0}
            }
        }
    );

    for (std::size_t index = 1;
         index <= update_count;
         ++index) {
      const auto timestamp = now_ns();

      const double price =
          64'999.90 +
          static_cast<double>(
              index % 10
          ) *
              0.01;

      recorder.write_update_batch(
          {
              minimatch::MarketDataUpdate{
                  .venue = "SIMULATED",
                  .symbol = "BTC-USD",
                  .sequence =
                      static_cast<std::uint64_t>(
                          index
                      ),
                  .timestamp_ns = timestamp,
                  .side =
                      minimatch::MarketDataSide::Bid,
                  .type =
                      minimatch::
                          MarketDataUpdateType::
                              Upsert,
                  .price = price,
                  .quantity =
                      1.0 +
                      static_cast<double>(
                          index % 5
                      )
              }
          },
          timestamp
      );
    }

    recorder.flush();

    std::cout
        << "recording="
        << path
        << '\n'
        << "records_written="
        << recorder.records_written()
        << '\n'
        << "bytes_written="
        << recorder.bytes_written()
        << '\n';

    return EXIT_SUCCESS;
  } catch (const std::exception& error) {
    std::cerr
        << "Market recorder failed: "
        << error.what()
        << '\n';

    return EXIT_FAILURE;
  }
}
