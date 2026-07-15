#include "minimatch/market_recorder.hpp"
#include "minimatch/router_market_data.hpp"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;

using tcp = asio::ip::tcp;

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
      << (
             plan.complete
                 ? "true"
                 : "false"
         )
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

std::string market_json(
    const minimatch::ConsolidatedMarketData& market,
    const std::string& symbol,
    std::uint64_t timestamp_ns,
    std::size_t record_index,
    std::size_t applied_records,
    std::size_t rejected_records,
    double speed,
    bool replay_complete
) {
  const auto bbo =
      market.consolidated_bbo(symbol);

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
          market,
          symbol,
          1.0,
          1.0,
          0.01
      );

  const auto sell_plan =
      minimatch::build_route_plan(
          sell_request,
          market,
          symbol,
          1.0,
          1.0,
          0.01
      );

  std::ostringstream output;

  output
      << std::fixed
      << std::setprecision(8)
      << '{'
      << "\"type\":\"replay_l2_snapshot\","
      << "\"mode\":\"replay\","
      << "\"symbol\":\""
      << escape_json(symbol)
      << "\","
      << "\"timestampNs\":"
      << timestamp_ns
      << ",\"coinbaseReady\":"
      << (
             bbo.valid
                 ? "true"
                 : "false"
         )
      << ",\"coinbaseSnapshots\":1"
      << ",\"coinbaseUpdates\":"
      << applied_records
      << ",\"recordingEnabled\":false"
      << ",\"recordedRecords\":0"
      << ",\"recordedBytes\":0"
      << ",\"replay\":{"
      << "\"recordIndex\":"
      << record_index
      << ",\"appliedRecords\":"
      << applied_records
      << ",\"rejectedRecords\":"
      << rejected_records
      << ",\"speed\":"
      << speed
      << ",\"complete\":"
      << (
             replay_complete
                 ? "true"
                 : "false"
         )
      << "},\"bbo\":{"
      << "\"valid\":"
      << (
             bbo.valid
                 ? "true"
                 : "false"
         )
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
      market.venues_for_symbol(symbol);

  bool first_venue = true;

  for (const auto& venue : venues) {
    const auto book =
        market.find_book(
            venue,
            symbol
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

void sleep_for_replay_delta(
    std::uint64_t previous_timestamp_ns,
    std::uint64_t current_timestamp_ns,
    double speed
) {
  if (speed <= 0.0 ||
      previous_timestamp_ns == 0 ||
      current_timestamp_ns <=
          previous_timestamp_ns) {
    return;
  }

  const auto recorded_delta =
      current_timestamp_ns -
      previous_timestamp_ns;

  const auto scaled_delta =
      static_cast<std::uint64_t>(
          static_cast<double>(
              recorded_delta
          ) /
          speed
      );

  std::this_thread::sleep_for(
      std::chrono::nanoseconds(
          scaled_delta
      )
  );
}

void replay_to_client(
    tcp::socket socket,
    const std::string& recording_path,
    const std::string& symbol,
    double speed
) {
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
      << "Replay dashboard client connected"
      << " speed="
      << speed
      << "x\n";

  minimatch::MarketRecordReader reader(
      recording_path
  );

  minimatch::ConsolidatedMarketData market;

  minimatch::MarketRecord record;

  std::size_t record_index = 0;
  std::size_t applied_records = 0;
  std::size_t rejected_records = 0;

  std::uint64_t previous_timestamp_ns = 0;
  std::uint64_t latest_timestamp_ns = 0;

  while (reader.next(record)) {
    ++record_index;

    sleep_for_replay_delta(
        previous_timestamp_ns,
        record.recorded_timestamp_ns,
        speed
    );

    minimatch::MarketDataDecision decision;

    if (
        record.type ==
        minimatch::MarketRecordType::Snapshot
    ) {
      decision =
          market.apply_snapshot(
              record.snapshot
          );
    } else {
      decision =
          market.apply_batch(
              record.updates
          );
    }

    if (decision.accepted) {
      ++applied_records;
    } else {
      ++rejected_records;

      std::cerr
          << "Replay record rejected"
          << " index="
          << record_index
          << " message="
          << decision.message
          << '\n';
    }

    latest_timestamp_ns =
        record.recorded_timestamp_ns;

    const std::string message =
        market_json(
            market,
            symbol,
            latest_timestamp_ns,
            record_index,
            applied_records,
            rejected_records,
            speed,
            false
        );

    stream.write(
        asio::buffer(message)
    );

    previous_timestamp_ns =
        record.recorded_timestamp_ns;
  }

  const std::string final_message =
      market_json(
          market,
          symbol,
          latest_timestamp_ns,
          record_index,
          applied_records,
          rejected_records,
          speed,
          true
      );

  stream.write(
      asio::buffer(final_message)
  );

  std::cout
      << "Replay complete"
      << " records="
      << record_index
      << " applied="
      << applied_records
      << " rejected="
      << rejected_records
      << '\n';

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
        << "Replay socket close warning: "
        << close_error.message()
        << '\n';
  }
}

void print_usage(
    const char* program
) {
  std::cerr
      << "Usage:\n  "
      << program
      << " <port>"
      << " <recording.bin>"
      << " [symbol]"
      << " [speed]\n\n"
      << "Examples:\n"
      << "  "
      << program
      << " 8090 session.bin BTC-USD 1\n"
      << "  "
      << program
      << " 8090 session.bin BTC-USD 10\n"
      << "  "
      << program
      << " 8090 session.bin BTC-USD 100\n"
      << "  "
      << program
      << " 8090 session.bin BTC-USD 0\n\n"
      << "speed=0 means maximum speed\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc < 3) {
      print_usage(argv[0]);
      return EXIT_FAILURE;
    }

    const auto port =
        static_cast<unsigned short>(
            std::stoul(argv[1])
        );

    const std::string recording_path =
        argv[2];

    const std::string symbol =
        argc > 3
            ? argv[3]
            : "BTC-USD";

    const double speed =
        argc > 4
            ? std::stod(argv[4])
            : 1.0;

    if (!std::isfinite(speed) ||
        speed < 0.0) {
      throw std::invalid_argument(
          "replay speed must be non-negative"
      );
    }

    // Validate the recording before opening the port.
    minimatch::MarketRecordReader validator(
        recording_path
    );

    static_cast<void>(validator);

    asio::io_context io_context;

    tcp::acceptor acceptor(
        io_context,
        tcp::endpoint(
            tcp::v4(),
            port
        )
    );

    std::cout
        << "MiniMatch replay WebSocket server listening on "
        << "ws://0.0.0.0:"
        << port
        << " recording="
        << recording_path
        << " symbol="
        << symbol
        << " speed="
        << speed
        << "x\n";

    for (;;) {
      tcp::socket socket(io_context);

      acceptor.accept(socket);

      try {
        replay_to_client(
            std::move(socket),
            recording_path,
            symbol,
            speed
        );
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
              << "Replay dashboard client disconnected\n";

          continue;
        }

        std::cerr
            << "Replay client failed: "
            << error.what()
            << '\n';
      } catch (const std::exception& error) {
        std::cerr
            << "Replay client failed: "
            << error.what()
            << '\n';
      }
    }
  } catch (const std::exception& error) {
    std::cerr
        << "Replay WebSocket server failed: "
        << error.what()
        << '\n';

    return EXIT_FAILURE;
  }
}
