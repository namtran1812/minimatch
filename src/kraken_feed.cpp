#include "minimatch/kraken_feed.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <chrono>
#include <cstdint>
#include <iostream>
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

void publish_status(
    const KrakenStatusHandler& handler,
    const KrakenFeedStats& stats
) {
  if (handler) {
    handler(stats);
  }
}

}  // namespace

KrakenFeed::KrakenFeed(
    KrakenFeedConfig config
)
    : config_(std::move(config)) {
  if (config_.exchange_symbol.empty()) {
    throw std::invalid_argument(
        "Kraken exchange symbol cannot be empty"
    );
  }

  if (config_.normalized_symbol.empty()) {
    throw std::invalid_argument(
        "Kraken normalized symbol cannot be empty"
    );
  }

  if (
      config_.depth != 10 &&
      config_.depth != 25 &&
      config_.depth != 100 &&
      config_.depth != 500 &&
      config_.depth != 1000
  ) {
    throw std::invalid_argument(
        "unsupported Kraken book depth"
    );
  }

  if (config_.websocket_host.empty() ||
      config_.websocket_port.empty() ||
      config_.websocket_target.empty()) {
    throw std::invalid_argument(
        "Kraken WebSocket configuration is incomplete"
    );
  }
}

void KrakenFeed::run(
    std::atomic<bool>& running,
    const KrakenSnapshotHandler&
        on_snapshot,
    const KrakenUpdateHandler&
        on_updates,
    const KrakenStatusHandler&
        on_status
) {
  if (!on_snapshot || !on_updates) {
    throw std::invalid_argument(
        "Kraken feed handlers are required"
    );
  }

  KrakenFeedStats stats;

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

      websocket::response_type response;

      stream.handshake(
          response,
          config_.websocket_host +
              ":" +
              config_.websocket_port,
          config_.websocket_target
      );

      if (
          response.result() !=
          http::status::switching_protocols
      ) {
        throw std::runtime_error(
            "Kraken WebSocket returned HTTP " +
            std::to_string(
                response.result_int()
            )
        );
      }

      ++stats.connections;
      stats.synchronized = false;

      publish_status(on_status, stats);

      const std::string subscription =
          build_kraken_subscription(
              config_.exchange_symbol,
              config_.depth
          );

      stream.write(
          asio::buffer(subscription)
      );

      KrakenBookNormalizer normalizer(
          config_.normalized_symbol,
          config_.depth
      );

      while (running.load()) {
        beast::flat_buffer buffer;

        stream.read(buffer);

        ++stats.received_messages;

        const std::string payload =
            beast::buffers_to_string(
                buffer.data()
            );

        const auto parsed =
            parse_kraken_book_message(
                payload,
                config_.exchange_symbol,
                now_ns()
            );

        if (!parsed.valid) {
          ++stats.rejected_messages;
          stats.synchronized = false;

          publish_status(
              on_status,
              stats
          );

          throw std::runtime_error(
              "Kraken message parse failed: " +
              parsed.error
          );
        }

        if (parsed.ignored) {
          ++stats.ignored_messages;

          publish_status(
              on_status,
              stats
          );

          continue;
        }

        stats.latest_checksum =
            parsed.checksum;

        if (parsed.snapshot) {
          const auto snapshot =
              normalizer.normalize_snapshot(
                  parsed
              );

          on_snapshot(snapshot);

          ++stats.snapshots;

          stats.synchronized =
              normalizer.synchronized();

          stats.local_sequence =
              normalizer.sequence();

          publish_status(
              on_status,
              stats
          );

          continue;
        }

        const auto updates =
            normalizer.normalize_update(
                parsed
            );

        if (updates.empty()) {
          ++stats.ignored_messages;

          publish_status(
              on_status,
              stats
          );

          continue;
        }

        on_updates(updates);

        ++stats.accepted_updates;

        stats.synchronized =
            normalizer.synchronized();

        stats.local_sequence =
            normalizer.sequence();

        publish_status(
            on_status,
            stats
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
              websocket::error::closed &&
          close_error !=
              ssl::error::stream_truncated
      ) {
        throw beast::system_error(
            close_error
        );
      }
    } catch (const std::exception& error) {
      std::cerr
          << "Kraken feed connection failed: "
          << error.what()
          << '\n';

      stats.synchronized = false;

      publish_status(
          on_status,
          stats
      );

      if (!running.load()) {
        break;
      }

      ++stats.reconnects;

      publish_status(
          on_status,
          stats
      );

      std::this_thread::sleep_for(
          config_.reconnect_delay
      );
    }
  }
}

const KrakenFeedConfig&
KrakenFeed::config() const {
  return config_;
}

}  // namespace minimatch
