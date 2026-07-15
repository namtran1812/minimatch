#include "minimatch/binance_l2.hpp"
#include "minimatch/market_data.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace ssl = asio::ssl;
namespace websocket = beast::websocket;

using tcp = asio::ip::tcp;

constexpr const char* websocket_host =
    "data-stream.binance.vision";

constexpr const char* websocket_port = "443";

constexpr const char* rest_host =
    "data-api.binance.vision";

constexpr const char* rest_port = "443";

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

std::string lowercase(
    std::string value
) {
  std::transform(
      value.begin(),
      value.end(),
      value.begin(),
      [](unsigned char character) {
        return static_cast<char>(
            std::tolower(character)
        );
      }
  );

  return value;
}

std::string uppercase(
    std::string value
) {
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

template <typename Stream>
void configure_tls_hostname(
    Stream& stream,
    const std::string& host
) {
  if (!SSL_set_tlsext_host_name(
          stream.native_handle(),
          host.c_str()
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

  stream.set_verify_callback(
      ssl::host_name_verification(host)
  );
}

std::string fetch_depth_snapshot(
    asio::io_context& io_context,
    ssl::context& ssl_context,
    const std::string& exchange_symbol,
    std::size_t limit
) {
  tcp::resolver resolver(io_context);

  beast::ssl_stream<
      beast::tcp_stream
  > stream(
      io_context,
      ssl_context
  );

  configure_tls_hostname(
      stream,
      rest_host
  );

  const auto endpoints =
      resolver.resolve(
          rest_host,
          rest_port
      );

  beast::get_lowest_layer(stream)
      .expires_after(
          std::chrono::seconds(10)
      );

  beast::get_lowest_layer(stream)
      .connect(endpoints);

  stream.handshake(
      ssl::stream_base::client
  );

  const std::string target =
      "/api/v3/depth?symbol=" +
      exchange_symbol +
      "&limit=" +
      std::to_string(limit);

  http::request<
      http::empty_body
  > request{
      http::verb::get,
      target,
      11
  };

  request.set(
      http::field::host,
      rest_host
  );

  request.set(
      http::field::user_agent,
      "MiniMatch/1.0"
  );

  request.set(
      http::field::accept,
      "application/json"
  );

  http::write(stream, request);

  beast::flat_buffer buffer;

  http::response<
      http::string_body
  > response;

  http::read(
      stream,
      buffer,
      response
  );

  if (
      response.result() !=
      http::status::ok
  ) {
    throw std::runtime_error(
        "Binance snapshot HTTP request failed: " +
        std::to_string(
            response.result_int()
        ) +
        " " +
        response.body()
    );
  }

  beast::error_code shutdown_error;

  stream.shutdown(shutdown_error);

  if (
      shutdown_error ==
      asio::error::eof ||
      shutdown_error ==
      ssl::error::stream_truncated
  ) {
    shutdown_error = {};
  }

  if (shutdown_error) {
    std::cerr
        << "Binance REST TLS shutdown warning: "
        << shutdown_error.message()
        << '\n';
  }

  return response.body();
}

minimatch::MarketDataSnapshot
normalized_snapshot(
    const minimatch::BinanceDepthSnapshot& snapshot,
    const std::string& normalized_symbol
) {
  return minimatch::MarketDataSnapshot{
      .venue = "BINANCE",
      .symbol = normalized_symbol,
      .sequence = 0,
      .timestamp_ns =
          snapshot.received_timestamp_ns,
      .bids = snapshot.bids,
      .asks = snapshot.asks
  };
}

void print_book(
    const minimatch::ConsolidatedMarketData& market,
    const std::string& symbol,
    std::uint64_t exchange_update_id,
    std::size_t accepted_events,
    std::size_t ignored_events
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
      << exchange_update_id
      << " accepted="
      << accepted_events
      << " ignored="
      << ignored_events
      << '\n';
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const std::string exchange_symbol =
        argc > 1
            ? uppercase(argv[1])
            : "BTCUSDT";

    const std::string normalized_symbol =
        argc > 2
            ? argv[2]
            : "BTC-USD";

    const std::size_t maximum_events =
        argc > 3
            ? static_cast<std::size_t>(
                  std::stoull(argv[3])
              )
            : 100;

    const std::string stream_name =
        lowercase(exchange_symbol) +
        "@depth@100ms";

    const std::string websocket_target =
        "/ws/" + stream_name;

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

    configure_tls_hostname(
        stream.next_layer(),
        websocket_host
    );

    const auto endpoints =
        resolver.resolve(
            websocket_host,
            websocket_port
        );

    beast::get_lowest_layer(stream)
        .expires_after(
            std::chrono::seconds(10)
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
            [](
                websocket::request_type&
                    request
            ) {
              request.set(
                  beast::http::field::
                      user_agent,
                  "MiniMatch/1.0"
              );
            }
        )
    );

    websocket::response_type
        handshake_response;

    stream.handshake(
        handshake_response,
        std::string(websocket_host) +
            ":" +
            websocket_port,
        websocket_target
    );

    if (
        handshake_response.result() !=
        beast::http::status::
            switching_protocols
    ) {
      throw std::runtime_error(
          "Binance WebSocket handshake returned HTTP " +
          std::to_string(
              handshake_response.result_int()
          ) +
          ": " +
          handshake_response.body()
      );
    }

    std::cout
        << "Connected to Binance diff-depth stream "
        << websocket_target
        << '\n';

    /*
     * The WebSocket is intentionally opened before
     * the REST snapshot is requested. Messages that
     * arrive during the HTTP request remain queued in
     * the socket receive buffer.
     */
    const std::string snapshot_json =
        fetch_depth_snapshot(
            io_context,
            ssl_context,
            exchange_symbol,
            5000
        );

    const auto parsed_snapshot =
        minimatch::
            parse_binance_depth_snapshot(
                snapshot_json,
                exchange_symbol,
                now_ns()
            );

    if (!parsed_snapshot.valid) {
      throw std::runtime_error(
          "Binance snapshot parse failed: " +
          parsed_snapshot.error
      );
    }

    minimatch::BinanceBookSynchronizer
        synchronizer(normalized_symbol);

    const auto sync_snapshot =
        synchronizer.apply_snapshot(
            parsed_snapshot
        );

    if (!sync_snapshot.accepted) {
      throw std::runtime_error(
          sync_snapshot.message
      );
    }

    minimatch::ConsolidatedMarketData market;

    const auto snapshot_decision =
        market.apply_snapshot(
            normalized_snapshot(
                parsed_snapshot,
                normalized_symbol
            )
        );

    if (!snapshot_decision.accepted) {
      throw std::runtime_error(
          "Normalized Binance snapshot rejected: " +
          snapshot_decision.message
      );
    }

    std::cout
        << "Applied Binance REST snapshot"
        << " lastUpdateId="
        << parsed_snapshot.last_update_id
        << " bids="
        << parsed_snapshot.bids.size()
        << " asks="
        << parsed_snapshot.asks.size()
        << '\n';

    std::size_t received_events = 0;
    std::size_t accepted_events = 0;
    std::size_t ignored_events = 0;

    while (
        maximum_events == 0 ||
        accepted_events < maximum_events
    ) {
      beast::flat_buffer buffer;

      stream.read(buffer);

      ++received_events;

      const std::string payload =
          beast::buffers_to_string(
              buffer.data()
          );

      const auto parsed_update =
          minimatch::
              parse_binance_depth_update(
                  payload,
                  exchange_symbol,
                  now_ns()
              );

      if (!parsed_update.valid) {
        throw std::runtime_error(
            "Binance update parse failed: " +
            parsed_update.error
        );
      }

      const auto sync_result =
          synchronizer.apply_update(
              parsed_update
          );

      if (sync_result.ignored) {
        ++ignored_events;
        continue;
      }

      if (!sync_result.accepted) {
        if (
            sync_result.snapshot_required
        ) {
          throw std::runtime_error(
              sync_result.message +
              "; reconnect and bootstrap a new snapshot"
          );
        }

        std::cerr
            << "Binance update rejected: "
            << sync_result.message
            << '\n';

        continue;
      }

      if (sync_result.updates.empty()) {
        ++ignored_events;
        continue;
      }

      const auto decision =
          market.apply_batch(
              sync_result.updates
          );

      if (!decision.accepted) {
        throw std::runtime_error(
            "Normalized Binance update rejected: " +
            decision.message
        );
      }

      ++accepted_events;

      print_book(
          market,
          normalized_symbol,
          synchronizer.last_update_id(),
          accepted_events,
          ignored_events
      );
    }

    std::cout
        << "Binance stream summary"
        << " received="
        << received_events
        << " accepted="
        << accepted_events
        << " ignored="
        << ignored_events
        << " synchronized="
        << (
               synchronizer.synchronized()
                   ? "true"
                   : "false"
           )
        << '\n';

    beast::error_code close_error;

    stream.close(
        websocket::close_code::normal,
        close_error
    );

    if (
        close_error &&
        close_error !=
            websocket::error::closed &&
        close_error !=
            ssl::error::stream_truncated
    ) {
      std::cerr
          << "Binance WebSocket close warning: "
          << close_error.message()
          << '\n';
    }

    return EXIT_SUCCESS;
  } catch (const std::exception& error) {
    std::cerr
        << "Binance Level-2 client failed: "
        << error.what()
        << '\n';

    return EXIT_FAILURE;
  }
}
