#include "minimatch/binance_feed.hpp"

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
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace minimatch {

namespace {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace ssl = asio::ssl;
namespace websocket = beast::websocket;

using tcp = asio::ip::tcp;

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

std::string fetch_snapshot(
    asio::io_context& io_context,
    ssl::context& ssl_context,
    const BinanceFeedConfig& config
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
      config.rest_host
  );

  const auto endpoints =
      resolver.resolve(
          config.rest_host,
          config.rest_port
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
      uppercase(config.exchange_symbol) +
      "&limit=" +
      std::to_string(
          config.snapshot_depth
      );

  http::request<
      http::empty_body
  > request{
      http::verb::get,
      target,
      11
  };

  request.set(
      http::field::host,
      config.rest_host
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
        "Binance REST snapshot failed with HTTP " +
        std::to_string(
            response.result_int()
        ) +
        ": " +
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
    throw beast::system_error(
        shutdown_error
    );
  }

  return response.body();
}

MarketDataSnapshot normalize_snapshot(
    const BinanceDepthSnapshot& snapshot,
    const std::string& normalized_symbol
) {
  return MarketDataSnapshot{
      .venue = "BINANCE",
      .symbol = normalized_symbol,
      .sequence = 0,
      .timestamp_ns =
          snapshot.received_timestamp_ns,
      .bids = snapshot.bids,
      .asks = snapshot.asks
  };
}

void publish_status(
    const BinanceStatusHandler& handler,
    const BinanceFeedStats& stats
) {
  if (handler) {
    handler(stats);
  }
}

}  // namespace

BinanceFeed::BinanceFeed(
    BinanceFeedConfig config
)
    : config_(std::move(config)) {
  config_.exchange_symbol =
      uppercase(config_.exchange_symbol);

  if (config_.exchange_symbol.empty()) {
    throw std::invalid_argument(
        "Binance exchange symbol cannot be empty"
    );
  }

  if (config_.normalized_symbol.empty()) {
    throw std::invalid_argument(
        "Binance normalized symbol cannot be empty"
    );
  }

  if (
      config_.snapshot_depth != 100 &&
      config_.snapshot_depth != 500 &&
      config_.snapshot_depth != 1000 &&
      config_.snapshot_depth != 5000
  ) {
    throw std::invalid_argument(
        "unsupported Binance snapshot depth"
    );
  }
}

void BinanceFeed::run(
    std::atomic<bool>& running,
    const BinanceSnapshotHandler&
        on_snapshot,
    const BinanceUpdateHandler&
        on_updates,
    const BinanceStatusHandler&
        on_status
) {
  if (!on_snapshot || !on_updates) {
    throw std::invalid_argument(
        "Binance feed handlers are required"
    );
  }

  BinanceFeedStats stats;

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

      configure_tls_hostname(
          stream.next_layer(),
          config_.websocket_host
      );

      const auto endpoints =
          resolver.resolve(
              config_.websocket_host,
              config_.websocket_port
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
                    http::field::user_agent,
                    "MiniMatch/1.0"
                );
              }
          )
      );

      const std::string target =
          "/ws/" +
          lowercase(
              config_.exchange_symbol
          ) +
          "@depth@100ms";

      websocket::response_type response;

      stream.handshake(
          response,
          config_.websocket_host +
              ":" +
              config_.websocket_port,
          target
      );

      if (
          response.result() !=
          http::status::switching_protocols
      ) {
        throw std::runtime_error(
            "Binance WebSocket returned HTTP " +
            std::to_string(
                response.result_int()
            )
        );
      }

      ++stats.connections;
      publish_status(on_status, stats);

      const std::string snapshot_json =
          fetch_snapshot(
              io_context,
              ssl_context,
              config_
          );

      const auto snapshot =
          parse_binance_depth_snapshot(
              snapshot_json,
              config_.exchange_symbol,
              now_ns()
          );

      if (!snapshot.valid) {
        throw std::runtime_error(
            "Binance snapshot parse failed: " +
            snapshot.error
        );
      }

      BinanceBookSynchronizer
          synchronizer(
              config_.normalized_symbol
          );

      const auto snapshot_result =
          synchronizer.apply_snapshot(
              snapshot
          );

      if (!snapshot_result.accepted) {
        throw std::runtime_error(
            snapshot_result.message
        );
      }

      on_snapshot(
          normalize_snapshot(
              snapshot,
              config_.normalized_symbol
          )
      );

      ++stats.snapshots;
      stats.synchronized = false;
      stats.last_update_id =
          snapshot.last_update_id;

      publish_status(on_status, stats);

      while (running.load()) {
        beast::flat_buffer buffer;

        stream.read(buffer);

        ++stats.received_messages;

        const auto parsed =
            parse_binance_depth_update(
                beast::buffers_to_string(
                    buffer.data()
                ),
                config_.exchange_symbol,
                now_ns()
            );

        if (!parsed.valid) {
          ++stats.rejected_updates;
          publish_status(on_status, stats);

          throw std::runtime_error(
              "Binance update parse failed: " +
              parsed.error
          );
        }

        const auto result =
            synchronizer.apply_update(
                parsed
            );

        stats.last_update_id =
            synchronizer.last_update_id();

        stats.synchronized =
            synchronizer.synchronized();

        if (result.ignored) {
          ++stats.ignored_updates;
          publish_status(on_status, stats);
          continue;
        }

        if (!result.accepted) {
          ++stats.rejected_updates;
          publish_status(on_status, stats);

          if (result.snapshot_required) {
            throw std::runtime_error(
                result.message
            );
          }

          continue;
        }

        if (result.updates.empty()) {
          ++stats.ignored_updates;
          publish_status(on_status, stats);
          continue;
        }

        on_updates(result.updates);

        ++stats.accepted_updates;
        publish_status(on_status, stats);
      }

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
        throw beast::system_error(
            close_error
        );
      }
    } catch (const std::exception&) {
      stats.synchronized = false;
      publish_status(on_status, stats);

      if (!running.load()) {
        break;
      }

      ++stats.reconnects;
      publish_status(on_status, stats);

      std::this_thread::sleep_for(
          std::chrono::seconds(3)
      );
    }
  }
}

const BinanceFeedConfig&
BinanceFeed::config() const {
  return config_;
}

}  // namespace minimatch
