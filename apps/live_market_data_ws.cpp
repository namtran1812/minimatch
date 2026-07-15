#include "minimatch/coinbase_l2.hpp"
#include "minimatch/market_data.hpp"
#include "minimatch/router_market_data.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace ssl = asio::ssl;

using tcp = asio::ip::tcp;

constexpr const char* coinbase_host =
    "advanced-trade-ws.coinbase.com";

constexpr const char* coinbase_port = "443";
constexpr const char* coinbase_target = "/";

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

std::string escape_json(
    const std::string& value
) {
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
    const std::vector<
        minimatch::MarketDataLevel
    >& levels
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

std::string route_json(
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
        << ",\"estimatedFee\":"
        << leg.estimated_fee
        << '}';
  }

  output << "]}";

  return output.str();
}

class LiveMarketState {
 public:
  explicit LiveMarketState(
      std::string product_id
  )
      : product_id_(std::move(product_id)) {}

  void apply_coinbase_snapshot(
      minimatch::MarketDataSnapshot snapshot
  ) {
    std::lock_guard<std::mutex> lock(
        mutex_
    );

    normalized_coinbase_sequence_ = 0;

    snapshot.sequence =
        normalized_coinbase_sequence_;

    const auto decision =
        market_.apply_snapshot(snapshot);

    if (!decision.accepted) {
      throw std::runtime_error(
          "Coinbase snapshot rejected: " +
          decision.message
      );
    }

    coinbase_ready_ = true;
    ++coinbase_snapshot_count_;
  }

  void apply_coinbase_updates(
      std::vector<
          minimatch::MarketDataUpdate
      > updates
  ) {
    std::lock_guard<std::mutex> lock(
        mutex_
    );

    if (!coinbase_ready_ ||
        updates.empty()) {
      return;
    }

    ++normalized_coinbase_sequence_;

    for (auto& update : updates) {
      update.sequence =
          normalized_coinbase_sequence_;
    }

    const auto decision =
        market_.apply_batch(updates);

    if (!decision.accepted) {
      coinbase_ready_ = false;

      throw std::runtime_error(
          "Coinbase update rejected: " +
          decision.message
      );
    }

    ++coinbase_update_count_;
  }

  [[nodiscard]] std::optional<double>
  coinbase_midpoint() const {
    std::lock_guard<std::mutex> lock(
        mutex_
    );

    if (!coinbase_ready_) {
      return std::nullopt;
    }

    const auto book =
        market_.find_book(
            "COINBASE",
            product_id_
        );

    if (!book.has_value() ||
        !book->synchronized()) {
      return std::nullopt;
    }

    const auto bid = book->best_bid();
    const auto ask = book->best_ask();

    if (!bid.has_value() ||
        !ask.has_value() ||
        bid->price >= ask->price) {
      return std::nullopt;
    }

    return (
        bid->price +
        ask->price
    ) / 2.0;
  }

  void apply_simulated_snapshot(
      const std::string& venue,
      std::uint64_t sequence,
      double bid,
      double ask,
      double bid_quantity,
      double ask_quantity
  ) {
    std::lock_guard<std::mutex> lock(
        mutex_
    );

    const auto decision =
        market_.apply_snapshot(
            minimatch::MarketDataSnapshot{
                .venue = venue,
                .symbol = product_id_,
                .sequence = sequence,
                .timestamp_ns = now_ns(),
                .bids = {
                    {bid, bid_quantity},
                    {bid - 0.05,
                     bid_quantity * 1.5},
                    {bid - 0.10,
                     bid_quantity * 2.0}
                },
                .asks = {
                    {ask, ask_quantity},
                    {ask + 0.05,
                     ask_quantity * 1.5},
                    {ask + 0.10,
                     ask_quantity * 2.0}
                }
            }
        );

    if (!decision.accepted) {
      throw std::runtime_error(
          venue +
          " snapshot rejected: " +
          decision.message
      );
    }
  }

  std::string json() const {
    std::lock_guard<std::mutex> lock(
        mutex_
    );

    const auto bbo =
        market_.consolidated_bbo(
            product_id_
        );

    minimatch::RouteRequest buy_request{};
    buy_request.side =
        minimatch::RouteSide::Buy;
    buy_request.quantity = 0.10;

    minimatch::RouteRequest sell_request{};
    sell_request.side =
        minimatch::RouteSide::Sell;
    sell_request.quantity = 0.10;

    const auto buy_plan =
        minimatch::build_route_plan(
            buy_request,
            market_,
            product_id_,
            1.0,
            1.0,
            0.01
        );

    const auto sell_plan =
        minimatch::build_route_plan(
            sell_request,
            market_,
            product_id_,
            1.0,
            1.0,
            0.01
        );

    std::ostringstream output;

    output
        << std::fixed
        << std::setprecision(8)
        << '{'
        << "\"type\":\"live_l2_snapshot\","
        << "\"symbol\":\""
        << escape_json(product_id_)
        << "\","
        << "\"timestampNs\":"
        << now_ns()
        << ','
        << "\"coinbaseReady\":"
        << (
               coinbase_ready_
                   ? "true"
                   : "false"
           )
        << ",\"coinbaseSnapshots\":"
        << coinbase_snapshot_count_
        << ",\"coinbaseUpdates\":"
        << coinbase_update_count_
        << ",\"bbo\":{"
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

    const auto venues =
        market_.venues_for_symbol(
            product_id_
        );

    bool first_venue = true;

    for (const auto& venue : venues) {
      const auto book =
          market_.find_book(
              venue,
              product_id_
          );

      if (!book.has_value()) {
        continue;
      }

      if (!first_venue) {
        output << ',';
      }

      first_venue = false;

      output
          << '{'
          << "\"venue\":\""
          << escape_json(venue)
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
  std::string product_id_;

  mutable std::mutex mutex_;

  minimatch::ConsolidatedMarketData market_;

  std::uint64_t
      normalized_coinbase_sequence_{0};

  bool coinbase_ready_{false};

  std::uint64_t
      coinbase_snapshot_count_{0};

  std::uint64_t
      coinbase_update_count_{0};
};

void run_simulated_venues(
    const std::shared_ptr<
        LiveMarketState
    >& state,
    std::atomic<bool>& running
) {
  std::mt19937_64 random(42);

  std::normal_distribution<double>
      movement(0.0, 0.03);

  std::uint64_t kraken_sequence = 1;
  std::uint64_t binance_sequence = 1;

  double fallback_midpoint = 64610.0;

  while (running.load()) {
    const auto live_midpoint =
        state->coinbase_midpoint();

    if (live_midpoint.has_value()) {
      fallback_midpoint =
          *live_midpoint;
    }

    const double kraken_mid =
        fallback_midpoint +
        movement(random);

    const double binance_mid =
        fallback_midpoint +
        movement(random);

    state->apply_simulated_snapshot(
        "KRAKEN",
        kraken_sequence++,
        kraken_mid - 0.08,
        kraken_mid + 0.08,
        1.5,
        1.8
    );

    state->apply_simulated_snapshot(
        "BINANCE",
        binance_sequence++,
        binance_mid - 0.05,
        binance_mid + 0.05,
        2.2,
        2.0
    );

    std::this_thread::sleep_for(
        std::chrono::milliseconds(200)
    );
  }
}

void run_coinbase_feed(
    const std::shared_ptr<
        LiveMarketState
    >& state,
    const std::string& product_id,
    std::atomic<bool>& running
) {
  while (running.load()) {
    try {
      asio::io_context io_context;

      ssl::context ssl_context(
          ssl::context::tls_client
      );

      ssl_context.set_default_verify_paths();

      ssl_context.set_verify_mode(
          ssl::verify_peer
      );

      tcp::resolver resolver(io_context);

      websocket::stream<
          beast::ssl_stream<
              beast::tcp_stream
          >
      > stream(
          io_context,
          ssl_context
      );

      if (!SSL_set_tlsext_host_name(
              stream.next_layer()
                  .native_handle(),
              coinbase_host
          )) {
        const auto error_code =
            static_cast<int>(
                ::ERR_get_error()
            );

        throw beast::system_error(
            beast::error_code(
                error_code,
                asio::error::
                    get_ssl_category()
            )
        );
      }

      const auto endpoints =
          resolver.resolve(
              coinbase_host,
              coinbase_port
          );

      beast::get_lowest_layer(stream)
          .connect(endpoints);

      stream.next_layer().handshake(
          ssl::stream_base::client
      );

      stream.set_option(
          websocket::stream_base::timeout::
              suggested(
                  beast::role_type::client
              )
      );

      stream.handshake(
          coinbase_host,
          coinbase_target
      );

      const std::string level2 =
          minimatch::
              coinbase_level2_subscription(
                  product_id
              );

      const std::string heartbeat =
          minimatch::
              coinbase_heartbeat_subscription();

      stream.write(
          asio::buffer(level2)
      );

      stream.write(
          asio::buffer(heartbeat)
      );

      std::cout
          << "Connected live Coinbase feed for "
          << product_id
          << '\n';

      while (running.load()) {
        beast::flat_buffer buffer;

        stream.read(buffer);

        const std::string payload =
            beast::buffers_to_string(
                buffer.data()
            );

        const auto parsed =
            minimatch::
                parse_coinbase_level2_message(
                    payload,
                    now_ns()
                );

        if (!parsed.valid) {
          std::cerr
              << "Coinbase parse rejected: "
              << parsed.error
              << '\n';

          continue;
        }

        if (parsed.heartbeat ||
            parsed.ignored) {
          continue;
        }

        if (parsed.snapshot) {
          state->apply_coinbase_snapshot(
              parsed.book_snapshot
          );
        } else {
          state->apply_coinbase_updates(
              parsed.updates
          );
        }
      }
    } catch (const std::exception& error) {
      std::cerr
          << "Coinbase feed disconnected: "
          << error.what()
          << '\n';

      if (running.load()) {
        std::this_thread::sleep_for(
            std::chrono::seconds(3)
        );
      }
    }
  }
}

void handle_dashboard_client(
    tcp::socket socket,
    const std::shared_ptr<
        LiveMarketState
    >& state
) {
  try {
    websocket::stream<tcp::socket> stream(
        std::move(socket)
    );

    stream.set_option(
        websocket::stream_base::timeout::
            suggested(
                beast::role_type::server
            )
    );

    stream.accept();

    std::cout
        << "Dashboard WebSocket client connected\n";

    for (;;) {
      const std::string message =
          state->json();

      stream.write(
          asio::buffer(message)
      );

      std::this_thread::sleep_for(
          std::chrono::milliseconds(250)
      );
    }
  } catch (
      const boost::system::system_error&
          error
  ) {
    if (
        error.code() ==
            websocket::error::closed ||
        error.code() ==
            asio::error::eof ||
        error.code() ==
            asio::error::connection_reset ||
        error.code() ==
            asio::error::broken_pipe
    ) {
      std::cout
          << "Dashboard WebSocket client disconnected\n";

      return;
    }

    std::cerr
        << "Dashboard client failed: "
        << error.what()
        << '\n';
  } catch (const std::exception& error) {
    std::cerr
        << "Dashboard client failed: "
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
            : 8089;

    const std::string product_id =
        argc > 2
            ? argv[2]
            : "BTC-USD";

    asio::io_context io_context;

    tcp::acceptor acceptor(
        io_context,
        tcp::endpoint(
            tcp::v4(),
            port
        )
    );

    auto state =
        std::make_shared<
            LiveMarketState
        >(product_id);

    std::atomic<bool> running{true};

    std::thread coinbase_thread(
        run_coinbase_feed,
        state,
        product_id,
        std::ref(running)
    );

    std::thread simulated_thread(
        run_simulated_venues,
        state,
        std::ref(running)
    );

    std::cout
        << "MiniMatch live market gateway listening on "
        << "ws://0.0.0.0:"
        << port
        << " product="
        << product_id
        << '\n';

    for (;;) {
      tcp::socket socket(io_context);

      acceptor.accept(socket);

      std::thread(
          handle_dashboard_client,
          std::move(socket),
          state
      ).detach();
    }

    running.store(false);

    coinbase_thread.join();
    simulated_thread.join();

    return EXIT_SUCCESS;
  } catch (const std::exception& error) {
    std::cerr
        << "Live market gateway failed: "
        << error.what()
        << '\n';

    return EXIT_FAILURE;
  }
}
