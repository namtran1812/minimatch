#include "minimatch/coinbase_l2.hpp"
#include "minimatch/market_data.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace ssl = asio::ssl;

using tcp = asio::ip::tcp;

constexpr const char* host =
    "advanced-trade-ws.coinbase.com";

constexpr const char* port = "443";
constexpr const char* target = "/";

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

void print_bbo(
    const minimatch::ConsolidatedMarketData& market,
    const std::string& product_id
) {
  const auto bbo =
      market.consolidated_bbo(product_id);

  if (!bbo.valid) {
    std::cout
        << "BBO unavailable for "
        << product_id
        << '\n';

    return;
  }

  std::cout
      << std::fixed
      << std::setprecision(2)
      << product_id
      << " bid="
      << bbo.bid_price
      << " x "
      << std::setprecision(8)
      << bbo.bid_quantity
      << std::setprecision(2)
      << " venue="
      << bbo.bid_venue
      << " ask="
      << bbo.ask_price
      << " x "
      << bbo.ask_quantity
      << " venue="
      << bbo.ask_venue
      << " spread="
      << bbo.spread
      << " midpoint="
      << bbo.midpoint
      << '\n';
}

void print_depth(
    const minimatch::ConsolidatedMarketData& market,
    const std::string& product_id,
    std::size_t depth
) {
  const auto book =
      market.find_book(
          "COINBASE",
          product_id
      );

  if (!book.has_value()) {
    return;
  }

  const auto bids = book->bids(depth);
  const auto asks = book->asks(depth);

  std::cout
      << "sequence="
      << book->sequence()
      << " synchronized="
      << (
             book->synchronized()
                 ? "true"
                 : "false"
         )
      << '\n';

  const std::size_t rows =
      std::max(
          bids.size(),
          asks.size()
      );

  for (std::size_t index = 0;
       index < rows;
       ++index) {
    std::cout << "  ";

    if (index < bids.size()) {
      std::cout
          << "BID "
          << bids[index].price
          << " x "
          << bids[index].quantity;
    } else {
      std::cout << "                    ";
    }

    std::cout << " | ";

    if (index < asks.size()) {
      std::cout
          << "ASK "
          << asks[index].price
          << " x "
          << asks[index].quantity;
    }

    std::cout << '\n';
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const std::string product_id =
        argc > 1
            ? argv[1]
            : "BTC-USD";

    const std::size_t maximum_messages =
        argc > 2
            ? static_cast<std::size_t>(
                  std::stoull(argv[2])
              )
            : 100;

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
            host
        )) {
      const auto error_code =
          static_cast<int>(
              ::ERR_get_error()
          );

      throw beast::system_error(
          beast::error_code(
              error_code,
              asio::error::get_ssl_category()
          )
      );
    }

    const auto endpoints =
        resolver.resolve(
            host,
            port
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

    stream.set_option(
        websocket::stream_base::decorator(
            [](websocket::request_type& request) {
              request.set(
                  beast::http::field::user_agent,
                  "MiniMatch/1.0"
              );
            }
        )
    );

    stream.handshake(host, target);

    const std::string level2_subscription =
        minimatch::coinbase_level2_subscription(
            product_id
        );

    const std::string heartbeat_subscription =
        minimatch::coinbase_heartbeat_subscription();

    stream.write(
        asio::buffer(
            level2_subscription
        )
    );

    stream.write(
        asio::buffer(
            heartbeat_subscription
        )
    );

    std::cout
        << "Connected to Coinbase Level-2 feed for "
        << product_id
        << '\n';

    minimatch::ConsolidatedMarketData market;

    std::size_t received_messages = 0;
    std::size_t level2_messages = 0;
    std::size_t heartbeat_messages = 0;
    std::size_t ignored_messages = 0;
    std::size_t rejected_messages = 0;

    std::uint64_t normalized_book_sequence = 0;
    bool snapshot_received = false;

    while (
        maximum_messages == 0 ||
        received_messages < maximum_messages
    ) {
      beast::flat_buffer buffer;

      stream.read(buffer);

      const std::string payload =
          beast::buffers_to_string(
              buffer.data()
          );

      ++received_messages;

      const auto parsed =
          minimatch::parse_coinbase_level2_message(
              payload,
              now_ns()
          );

      if (!parsed.valid) {
        ++rejected_messages;

        std::cerr
            << "Rejected Coinbase message: "
            << parsed.error
            << '\n';

        continue;
      }

      if (parsed.heartbeat) {
        ++heartbeat_messages;

        if (
            heartbeat_messages % 10 == 0
        ) {
          std::cout
              << "heartbeats="
              << heartbeat_messages
              << " level2="
              << level2_messages
              << " ignored="
              << ignored_messages
              << " rejected="
              << rejected_messages
              << '\n';
        }

        continue;
      }

      if (parsed.ignored) {
        ++ignored_messages;
        continue;
      }

      ++level2_messages;

      if (parsed.snapshot) {
        normalized_book_sequence = 0;

        auto snapshot =
            parsed.book_snapshot;

        snapshot.sequence =
            normalized_book_sequence;

        const auto decision =
            market.apply_snapshot(
                snapshot
            );

        if (!decision.accepted) {
          std::cerr
              << "Snapshot rejected: "
              << decision.message
              << '\n';

          continue;
        }

        snapshot_received = true;
      } else {
        if (!snapshot_received) {
          std::cerr
              << "Ignoring update before initial snapshot\n";
          continue;
        }

        ++normalized_book_sequence;

        auto updates = parsed.updates;

        for (auto& update : updates) {
          update.sequence =
              normalized_book_sequence;
        }

        const auto decision =
            market.apply_batch(
                updates
            );

        if (!decision.accepted) {
          std::cerr
              << "Incremental rejected: "
              << decision.message
              << " expected="
              << decision.expected_sequence
              << " received="
              << decision.received_sequence
              << '\n';

          continue;
        }
      }

      print_bbo(
          market,
          product_id
      );

      print_depth(
          market,
          product_id,
          5
      );
    }

    beast::error_code close_error;

    stream.close(
        websocket::close_code::normal,
        close_error
    );

    if (
        close_error &&
        close_error !=
            websocket::error::closed
    ) {
      std::cerr
          << "WebSocket close warning: "
          << close_error.message()
          << '\n';
    }

    std::cout
        << "messages="
        << received_messages
        << " level2="
        << level2_messages
        << " heartbeats="
        << heartbeat_messages
        << " ignored="
        << ignored_messages
        << " rejected="
        << rejected_messages
        << '\n';

    return EXIT_SUCCESS;
  } catch (const std::exception& error) {
    std::cerr
        << "Coinbase Level-2 client failed: "
        << error.what()
        << '\n';

    return EXIT_FAILURE;
  }
}
