#include "minimatch/market_recorder.hpp"
#include "minimatch/replay_control.hpp"
#include "minimatch/router_market_data.hpp"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

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

struct LoadedRecording {
  std::vector<minimatch::MarketRecord> records;
};

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

LoadedRecording load_recording(
    const std::string& path
) {
  minimatch::MarketRecordReader reader(path);

  LoadedRecording loaded;
  minimatch::MarketRecord record;

  while (reader.next(record)) {
    loaded.records.push_back(
        std::move(record)
    );

    record = minimatch::MarketRecord{};
  }

  if (loaded.records.empty()) {
    throw std::runtime_error(
        "market recording contains no records"
    );
  }

  return loaded;
}

std::size_t find_record_for_timestamp(
    const LoadedRecording& recording,
    std::uint64_t timestamp_ns
) {
  const auto iterator =
      std::lower_bound(
          recording.records.begin(),
          recording.records.end(),
          timestamp_ns,
          [](
              const minimatch::MarketRecord& record,
              std::uint64_t value
          ) {
            return
                record.recorded_timestamp_ns <
                value;
          }
      );

  return static_cast<std::size_t>(
      std::distance(
          recording.records.begin(),
          iterator
      )
  );
}

bool apply_record(
    const minimatch::MarketRecord& record,
    minimatch::ConsolidatedMarketData& market
) {
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

  return decision.accepted;
}

void rebuild_to_record(
    const LoadedRecording& recording,
    std::size_t target_record,
    minimatch::ConsolidatedMarketData& market,
    std::size_t& rejected
) {
  market =
      minimatch::ConsolidatedMarketData{};

  rejected = 0;

  const std::size_t end =
      std::min(
          target_record,
          recording.records.size()
      );

  for (std::size_t index = 0;
       index < end;
       ++index) {
    if (!apply_record(
            recording.records[index],
            market
        )) {
      ++rejected;
    }
  }
}

void sleep_interruptibly(
    minimatch::ReplayController& controller,
    std::uint64_t recorded_delta_ns
) {
  const auto state =
      controller.snapshot();

  if (state.speed <= 0.0 ||
      recorded_delta_ns == 0) {
    return;
  }

  const auto scaled =
      static_cast<std::uint64_t>(
          static_cast<double>(
              recorded_delta_ns
          ) /
          state.speed
      );

  constexpr std::uint64_t chunk_ns =
      10'000'000;

  std::uint64_t remaining = scaled;

  while (remaining > 0) {
    const auto current =
        controller.snapshot();

    if (
        current.status ==
            minimatch::ReplayStatus::Paused ||
        current.restart_requested ||
        current.seek_record.has_value() ||
        current.seek_timestamp_ns.has_value() ||
        controller.stopped()
    ) {
      return;
    }

    const auto sleep_ns =
        std::min(
            remaining,
            chunk_ns
        );

    std::this_thread::sleep_for(
        std::chrono::nanoseconds(
            sleep_ns
        )
    );

    remaining -= sleep_ns;
  }
}

std::string market_json(
    const minimatch::ConsolidatedMarketData& market,
    const std::string& symbol,
    std::uint64_t timestamp_ns,
    const minimatch::ReplayControlSnapshot& control,
    std::size_t rejected_records,
    std::uint64_t checksum
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
      << "\",\"timestampNs\":"
      << timestamp_ns
      << ",\"coinbaseReady\":"
      << (bbo.valid ? "true" : "false")
      << ",\"coinbaseSnapshots\":1"
      << ",\"coinbaseUpdates\":"
      << control.current_record
      << ",\"recordingEnabled\":false"
      << ",\"recordedRecords\":0"
      << ",\"recordedBytes\":0"
      << ",\"replay\":{"
      << "\"status\":\""
      << minimatch::to_string(
             control.status
         )
      << "\",\"recordIndex\":"
      << control.current_record
      << ",\"totalRecords\":"
      << control.total_records
      << ",\"rejectedRecords\":"
      << rejected_records
      << ",\"speed\":"
      << control.speed
      << ",\"generation\":"
      << control.generation
      << ",\"complete\":"
      << (
             control.status ==
                     minimatch::ReplayStatus::
                         Complete
                 ? "true"
                 : "false"
         )
      << ",\"checksum\":"
      << checksum
      << ",\"firstTimestampNs\":"
      << (
             control.total_records > 0
                 ? timestamp_ns
                 : 0
         )
      << ",\"currentTimestampNs\":"
      << timestamp_ns
      << "},\"bbo\":{"
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

  bool first = true;

  for (const auto& venue :
       market.venues_for_symbol(symbol)) {
    const auto book =
        market.find_book(
            venue,
            symbol
        );

    if (!book.has_value()) {
      continue;
    }

    if (!first) {
      output << ',';
    }

    first = false;

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

minimatch::ReplayCommand parse_command(
    const std::string& json
) {
  boost::property_tree::ptree root;
  std::istringstream input(json);

  boost::property_tree::read_json(
      input,
      root
  );

  const std::string command =
      root.get<std::string>(
          "command"
      );

  if (command == "play") {
    return minimatch::ReplayCommand{
        .type =
            minimatch::ReplayCommandType::Play
    };
  }

  if (command == "pause") {
    return minimatch::ReplayCommand{
        .type =
            minimatch::ReplayCommandType::Pause
    };
  }

  if (command == "restart") {
    return minimatch::ReplayCommand{
        .type =
            minimatch::ReplayCommandType::Restart
    };
  }

  if (command == "stop") {
    return minimatch::ReplayCommand{
        .type =
            minimatch::ReplayCommandType::Stop
    };
  }

  if (command == "speed") {
    return minimatch::ReplayCommand{
        .type =
            minimatch::ReplayCommandType::
                SetSpeed,
        .speed =
            root.get<double>("speed")
    };
  }

  if (command == "seek") {
    return minimatch::ReplayCommand{
        .type =
            minimatch::ReplayCommandType::
                SeekRecord,
        .record_index =
            root.get<std::size_t>(
                "recordIndex"
            )
    };
  }

  if (command == "seekTimestamp") {
    return minimatch::ReplayCommand{
        .type =
            minimatch::ReplayCommandType::
                SeekTimestamp,
        .timestamp_ns =
            root.get<std::uint64_t>(
                "timestampNs"
            )
    };
  }

  throw std::invalid_argument(
      "unsupported replay command"
  );
}

void handle_client(
    tcp::socket socket,
    const LoadedRecording& recording,
    const std::string& symbol,
    double initial_speed
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

  auto controller =
      std::make_shared<
          minimatch::ReplayController
      >(initial_speed);

  controller->update_progress(
      0,
      recording.records.size()
  );

  controller->play();

  std::mutex write_mutex;
  std::atomic<bool> connected{true};

  std::thread command_thread(
      [&stream,
       &write_mutex,
       &connected,
       controller]() {
        try {
          while (connected.load()) {
            beast::flat_buffer buffer;

            stream.read(buffer);

            const auto command =
                parse_command(
                    beast::buffers_to_string(
                        buffer.data()
                    )
                );

            controller->apply(command);
          }
        } catch (...) {
          connected.store(false);
          controller->stop();
        }
      }
  );

  minimatch::ConsolidatedMarketData market;

  std::size_t current_record = 0;
  std::size_t rejected_records = 0;
  std::uint64_t previous_timestamp = 0;
  std::uint64_t latest_timestamp = 0;

  try {
    while (connected.load() &&
           !controller->stopped()) {
      controller->wait_until_runnable();

      if (controller->stopped()) {
        break;
      }

      if (
          controller
              ->consume_restart_request()
      ) {
        current_record = 0;
        previous_timestamp = 0;
        latest_timestamp = 0;

        rebuild_to_record(
            recording,
            0,
            market,
            rejected_records
        );

        controller->update_progress(
            0,
            recording.records.size()
        );
      }

      const auto seek =
          controller
              ->consume_seek_request();

      if (seek.has_value()) {
        current_record =
            std::min(
                *seek,
                recording.records.size()
            );

        rebuild_to_record(
            recording,
            current_record,
            market,
            rejected_records
        );

        previous_timestamp =
            current_record > 0
                ? recording
                      .records[
                          current_record - 1
                      ]
                      .recorded_timestamp_ns
                : 0;

        latest_timestamp =
            previous_timestamp;

        controller->update_progress(
            current_record,
            recording.records.size()
        );
      }

      const auto timestamp_seek =
          controller
              ->consume_timestamp_seek_request();

      if (timestamp_seek.has_value()) {
        current_record =
            find_record_for_timestamp(
                recording,
                *timestamp_seek
            );

        rebuild_to_record(
            recording,
            current_record,
            market,
            rejected_records
        );

        previous_timestamp =
            current_record > 0
                ? recording
                      .records[
                          current_record - 1
                      ]
                      .recorded_timestamp_ns
                : 0;

        latest_timestamp =
            previous_timestamp;

        controller->update_progress(
            current_record,
            recording.records.size()
        );
      }

      if (
          current_record >=
          recording.records.size()
      ) {
        controller->mark_complete();

        const auto state =
            controller->snapshot();

        const std::string message =
            market_json(
                market,
                symbol,
                latest_timestamp,
                state,
                rejected_records,
                minimatch::
                    market_state_checksum(
                        market,
                        symbol
                    )
            );

        {
          std::lock_guard<std::mutex> lock(
              write_mutex
          );

          stream.write(
              asio::buffer(message)
          );
        }

        controller->pause();
        continue;
      }

      const auto& record =
          recording.records[
              current_record
          ];

      if (
          previous_timestamp > 0 &&
          record.recorded_timestamp_ns >
              previous_timestamp
      ) {
        sleep_interruptibly(
            *controller,
            record.recorded_timestamp_ns -
                previous_timestamp
        );
      }

      const auto before_apply =
          controller->snapshot();

      if (
          before_apply.status ==
              minimatch::ReplayStatus::
                  Paused ||
          before_apply.restart_requested ||
          before_apply.seek_record
              .has_value() ||
          before_apply.seek_timestamp_ns
              .has_value()
      ) {
        continue;
      }

      if (!apply_record(record, market)) {
        ++rejected_records;
      }

      ++current_record;

      latest_timestamp =
          record.recorded_timestamp_ns;

      previous_timestamp =
          record.recorded_timestamp_ns;

      controller->update_progress(
          current_record,
          recording.records.size()
      );

      const auto state =
          controller->snapshot();

      const std::string message =
          market_json(
              market,
              symbol,
              latest_timestamp,
              state,
              rejected_records,
              minimatch::
                  market_state_checksum(
                      market,
                      symbol
                  )
          );

      {
        std::lock_guard<std::mutex> lock(
            write_mutex
        );

        stream.write(
            asio::buffer(message)
        );
      }
    }
  } catch (...) {
    connected.store(false);
    controller->stop();
  }

  connected.store(false);
  controller->stop();

  beast::error_code close_error;

  stream.close(
      websocket::close_code::normal,
      close_error
  );

  if (command_thread.joinable()) {
    command_thread.join();
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc < 3) {
      std::cerr
          << "Usage: "
          << argv[0]
          << " <port> <recording.bin>"
          << " [symbol] [speed]\n";

      return EXIT_FAILURE;
    }

    const auto port =
        static_cast<unsigned short>(
            std::stoul(argv[1])
        );

    const std::string path = argv[2];

    const std::string symbol =
        argc > 3
            ? argv[3]
            : "BTC-USD";

    const double speed =
        argc > 4
            ? std::stod(argv[4])
            : 1.0;

    const LoadedRecording recording =
        load_recording(path);

    asio::io_context io_context;

    tcp::acceptor acceptor(
        io_context,
        tcp::endpoint(
            tcp::v4(),
            port
        )
    );

    std::cout
        << "Interactive replay server listening on "
        << "ws://0.0.0.0:"
        << port
        << " records="
        << recording.records.size()
        << " speed="
        << speed
        << "x\n";

    for (;;) {
      tcp::socket socket(io_context);
      acceptor.accept(socket);

      handle_client(
          std::move(socket),
          recording,
          symbol,
          speed
      );
    }
  } catch (const std::exception& error) {
    std::cerr
        << "Interactive replay server failed: "
        << error.what()
        << '\n';

    return EXIT_FAILURE;
  }
}
