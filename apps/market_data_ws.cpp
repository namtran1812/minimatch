#include "minimatch/market_data.hpp"
#include "minimatch/router_market_data.hpp"

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;

using tcp = asio::ip::tcp;

constexpr std::string_view symbol = "BTCUSD";

std::string escape_json(const std::string& value) {
  std::ostringstream output;

  for (const char character : value) {
    switch (character) {
      case '"':
        output << "\\\"";
        break;
      case '\\':
        output << "\\\\";
        break;
      case '\n':
        output << "\\n";
        break;
      case '\r':
        output << "\\r";
        break;
      case '\t':
        output << "\\t";
        break;
      default:
        output << character;
        break;
    }
  }

  return output.str();
}

std::string depth_json(
    const std::vector<minimatch::MarketDataLevel>& levels
) {
  std::ostringstream output;
  output << '[';

  for (std::size_t index = 0;
       index < levels.size();
       ++index) {
    if (index > 0) {
      output << ',';
    }

    output
        << "{\"price\":"
        << levels[index].price
        << ",\"quantity\":"
        << levels[index].quantity
        << '}';
  }

  output << ']';
  return output.str();
}

class MarketState {
 public:
  MarketState() {
    apply_initial_snapshot(
        "COINBASE",
        100,
        62640.00,
        62640.10
    );

    apply_initial_snapshot(
        "KRAKEN",
        200,
        62639.95,
        62640.15
    );

    apply_initial_snapshot(
        "BINANCE",
        300,
        62640.02,
        62640.12
    );
  }

  void update() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::normal_distribution<double> movement(
        0.0,
        0.025
    );

    for (auto& venue : venues_) {
      venue.mid += movement(random_);

      const double half_spread =
          venue.spread / 2.0;

      const double bid =
          std::round(
              (venue.mid - half_spread) * 100.0
          ) /
          100.0;

      const double ask =
          std::round(
              (venue.mid + half_spread) * 100.0
          ) /
          100.0;

      ++venue.sequence;

      market_.apply_snapshot(
          minimatch::MarketDataSnapshot{
              .venue = venue.name,
              .symbol = std::string(symbol),
              .sequence = venue.sequence,
              .timestamp_ns = now_ns(),
              .bids = {
                  {bid, venue.bid_quantity},
                  {bid - 0.05,
                   venue.bid_quantity * 1.5},
                  {bid - 0.10,
                   venue.bid_quantity * 2.0}
              },
              .asks = {
                  {ask, venue.ask_quantity},
                  {ask + 0.05,
                   venue.ask_quantity * 1.5},
                  {ask + 0.10,
                   venue.ask_quantity * 2.0}
              }
          }
      );
    }
  }

  std::string snapshot_json() const {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto bbo =
        market_.consolidated_bbo(
            std::string(symbol)
        );

    minimatch::RouteRequest buy_request{};
    buy_request.side =
        minimatch::RouteSide::Buy;
    buy_request.quantity = 1.0;

    minimatch::RouteRequest sell_request{};
    sell_request.side =
        minimatch::RouteSide::Sell;
    sell_request.quantity = 1.0;

    const auto buy_plan =
        minimatch::build_route_plan(
            buy_request,
            market_,
            std::string(symbol),
            1.0,
            1.0,
            0.01
        );

    const auto sell_plan =
        minimatch::build_route_plan(
            sell_request,
            market_,
            std::string(symbol),
            1.0,
            1.0,
            0.01
        );

    std::ostringstream output;

    output
        << std::fixed
        << std::setprecision(8)
        << '{'
        << "\"type\":\"l2_snapshot\","
        << "\"symbol\":\""
        << symbol
        << "\","
        << "\"timestampNs\":"
        << now_ns()
        << ','
        << "\"bbo\":{"
        << "\"valid\":"
        << (bbo.valid ? "true" : "false")
        << ",\"bidPrice\":"
        << bbo.bid_price
        << ",\"bidQuantity\":"
        << bbo.bid_quantity
        << ",\"bidVenue\":\""
        << escape_json(bbo.bid_venue)
        << "\",\"askPrice\":"
        << bbo.ask_price
        << ",\"askQuantity\":"
        << bbo.ask_quantity
        << ",\"askVenue\":\""
        << escape_json(bbo.ask_venue)
        << "\",\"midpoint\":"
        << bbo.midpoint
        << ",\"spread\":"
        << bbo.spread
        << "},\"venues\":[";

    const auto venue_names =
        market_.venues_for_symbol(
            std::string(symbol)
        );

    for (std::size_t index = 0;
         index < venue_names.size();
         ++index) {
      if (index > 0) {
        output << ',';
      }

      const auto book =
          market_.find_book(
              venue_names[index],
              std::string(symbol)
          );

      if (!book.has_value()) {
        continue;
      }

      output
          << '{'
          << "\"venue\":\""
          << escape_json(venue_names[index])
          << "\",\"sequence\":"
          << book->sequence()
          << ",\"synchronized\":"
          << (
                 book->synchronized()
                     ? "true"
                     : "false"
             )
          << ",\"bids\":"
          << depth_json(book->bids(5))
          << ",\"asks\":"
          << depth_json(book->asks(5))
          << '}';
    }

    output
        << "],\"routing\":{"
        << "\"buy\":"
        << route_json(buy_plan)
        << ",\"sell\":"
        << route_json(sell_plan)
        << "}}";

    return output.str();
  }

 private:
  struct VenueState {
    std::string name;
    std::uint64_t sequence{0};

    double mid{0.0};
    double spread{0.0};
    double bid_quantity{0.0};
    double ask_quantity{0.0};
  };

  mutable std::mutex mutex_;

  minimatch::ConsolidatedMarketData market_;

  std::vector<VenueState> venues_;

  std::mt19937_64 random_{42};

  static std::uint64_t now_ns() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<
            std::chrono::nanoseconds
        >(
            std::chrono::system_clock::now()
                .time_since_epoch()
        ).count()
    );
  }

  static std::string route_json(
      const minimatch::RoutePlan& plan
  ) {
    std::ostringstream output;

    output
        << '{'
        << "\"complete\":"
        << (plan.complete ? "true" : "false")
        << ",\"requestedQuantity\":"
        << plan.requested_quantity
        << ",\"routedQuantity\":"
        << plan.routed_quantity
        << ",\"averagePrice\":"
        << plan.average_price
        << ",\"estimatedFees\":"
        << plan.estimated_fees
        << ",\"legs\":[";

    for (std::size_t index = 0;
         index < plan.legs.size();
         ++index) {
      if (index > 0) {
        output << ',';
      }

      const auto& leg = plan.legs[index];

      output
          << '{'
          << "\"venue\":\""
          << escape_json(leg.venue)
          << "\",\"price\":"
          << leg.price
          << ",\"quantity\":"
          << leg.quantity
          << ",\"effectivePrice\":"
          << leg.effective_price
          << '}';
    }

    output << "]}";
    return output.str();
  }

  void apply_initial_snapshot(
      std::string venue,
      std::uint64_t sequence,
      double bid,
      double ask
  ) {
    venues_.push_back(
        VenueState{
            .name = venue,
            .sequence = sequence,
            .mid = (bid + ask) / 2.0,
            .spread = ask - bid,
            .bid_quantity = 2.0,
            .ask_quantity = 2.0
        }
    );

    market_.apply_snapshot(
        minimatch::MarketDataSnapshot{
            .venue = std::move(venue),
            .symbol = std::string(symbol),
            .sequence = sequence,
            .timestamp_ns = now_ns(),
            .bids = {
                {bid, 2.0},
                {bid - 0.05, 3.0},
                {bid - 0.10, 4.0}
            },
            .asks = {
                {ask, 2.0},
                {ask + 0.05, 3.0},
                {ask + 0.10, 4.0}
            }
        }
    );
  }
};

void handle_connection(
    tcp::socket socket,
    std::shared_ptr<MarketState> market,
    std::atomic<bool>& running
) {
  try {
    websocket::stream<tcp::socket> stream(
        std::move(socket)
    );

    stream.set_option(
        websocket::stream_base::timeout::suggested(
            beast::role_type::server
        )
    );

    stream.accept();

    std::cout
        << "Level-2 WebSocket client connected\n";

    while (running.load()) {
      const std::string message =
          market->snapshot_json();

      stream.write(
          asio::buffer(message)
      );

      std::this_thread::sleep_for(
          std::chrono::milliseconds(250)
      );
    }

    stream.close(
        websocket::close_code::normal
    );
  } catch (
      const boost::system::system_error& error
  ) {
    if (error.code() ==
            boost::asio::error::eof ||
        error.code() ==
            boost::asio::error::connection_reset ||
        error.code() ==
            boost::asio::error::broken_pipe ||
        error.code() ==
            boost::beast::websocket::error::closed) {
      std::cout
          << "Level-2 WebSocket client disconnected\n";
      return;
    }

    std::cerr
        << "WebSocket client failed: "
        << error.what()
        << '\n';
  } catch (const std::exception& error) {
    std::cerr
        << "WebSocket client failed: "
        << error.what()
        << '\n';
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const unsigned short port =
        argc > 1
            ? static_cast<unsigned short>(
                  std::stoul(argv[1])
              )
            : 9090;

    asio::io_context io_context;

    tcp::acceptor acceptor(
        io_context,
        tcp::endpoint(
            tcp::v4(),
            port
        )
    );

    auto market =
        std::make_shared<MarketState>();

    std::atomic<bool> running{true};

    std::thread updater(
        [market, &running]() {
          while (running.load()) {
            market->update();

            std::this_thread::sleep_for(
                std::chrono::milliseconds(100)
            );
          }
        }
    );

    std::cout
        << "MiniMatch Level-2 WebSocket server listening on "
        << "ws://0.0.0.0:"
        << port
        << '\n';

    while (running.load()) {
      tcp::socket socket(io_context);
      acceptor.accept(socket);

      std::thread(
          handle_connection,
          std::move(socket),
          market,
          std::ref(running)
      ).detach();
    }

    updater.join();
  } catch (const std::exception& error) {
    std::cerr
        << "Market-data WebSocket server failed: "
        << error.what()
        << '\n';

    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
