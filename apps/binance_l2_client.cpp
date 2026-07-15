#include "minimatch/binance_feed.hpp"
#include "minimatch/market_data.hpp"

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

void print_book(
    const minimatch::ConsolidatedMarketData&
        market,
    const std::string& symbol,
    const minimatch::BinanceFeedStats& stats
) {
  const auto book =
      market.find_book(
          "BINANCE",
          symbol
      );

  if (!book.has_value()) {
    return;
  }

  const auto bid = book->best_bid();
  const auto ask = book->best_ask();

  if (!bid.has_value() ||
      !ask.has_value()) {
    return;
  }

  std::cout
      << std::fixed
      << std::setprecision(2)
      << symbol
      << " bid="
      << bid->price
      << " x "
      << std::setprecision(8)
      << bid->quantity
      << std::setprecision(2)
      << " ask="
      << ask->price
      << " x "
      << std::setprecision(8)
      << ask->quantity
      << std::setprecision(2)
      << " spread="
      << ask->price - bid->price
      << " localSequence="
      << book->sequence()
      << " binanceUpdateId="
      << stats.last_update_id
      << " accepted="
      << stats.accepted_updates
      << " ignored="
      << stats.ignored_updates
      << '\n';
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const std::string exchange_symbol =
        argc > 1
            ? argv[1]
            : "BTCUSDT";

    const std::string normalized_symbol =
        argc > 2
            ? argv[2]
            : "BTC-USD";

    const std::size_t maximum_updates =
        argc > 3
            ? static_cast<std::size_t>(
                  std::stoull(argv[3])
              )
            : 25;

    std::atomic<bool> running{true};

    minimatch::ConsolidatedMarketData market;
    minimatch::BinanceFeedStats latest_stats;
    std::size_t applied_updates = 0;

    minimatch::BinanceFeed feed(
        minimatch::BinanceFeedConfig{
            .exchange_symbol =
                exchange_symbol,
            .normalized_symbol =
                normalized_symbol
        }
    );

    std::cout
        << "Starting reusable Binance feed"
        << " exchangeSymbol="
        << exchange_symbol
        << " normalizedSymbol="
        << normalized_symbol
        << '\n';

    feed.run(
        running,
        [&market](
            const minimatch::
                MarketDataSnapshot& snapshot
        ) {
          const auto decision =
              market.apply_snapshot(
                  snapshot
              );

          if (!decision.accepted) {
            throw std::runtime_error(
                "Binance snapshot rejected: " +
                decision.message
            );
          }
        },
        [
            &market,
            &running,
            maximum_updates,
            normalized_symbol,
            &latest_stats,
            &applied_updates
        ](
            const std::vector<
                minimatch::MarketDataUpdate
            >& updates
        ) {
          const auto decision =
              market.apply_batch(
                  updates
              );

          if (!decision.accepted) {
            throw std::runtime_error(
                "Binance updates rejected: " +
                decision.message
            );
          }

          ++applied_updates;

          auto display_stats =
              latest_stats;

          display_stats.accepted_updates =
              applied_updates;

          print_book(
              market,
              normalized_symbol,
              display_stats
          );

          if (
              maximum_updates > 0 &&
              applied_updates >=
                  maximum_updates
          ) {
            running.store(false);
          }
        },
        [&latest_stats](
            const minimatch::
                BinanceFeedStats& stats
        ) {
          latest_stats = stats;
        }
    );

    std::cout
        << "Binance stream summary"
        << " received="
        << latest_stats.received_messages
        << " accepted="
        << latest_stats.accepted_updates
        << " ignored="
        << latest_stats.ignored_updates
        << " rejected="
        << latest_stats.rejected_updates
        << " reconnects="
        << latest_stats.reconnects
        << " synchronized="
        << (
               latest_stats.synchronized
                   ? "true"
                   : "false"
           )
        << '\n';

    return EXIT_SUCCESS;
  } catch (const std::exception& error) {
    std::cerr
        << "Binance Level-2 client failed: "
        << error.what()
        << '\n';

    return EXIT_FAILURE;
  }
}
