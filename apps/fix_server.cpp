#include "minimatch/fix.hpp"
#include "minimatch/fix_gateway.hpp"
#include "minimatch/fix_session.hpp"
#include "minimatch/fix_store.hpp"
#include "minimatch/oms.hpp"

#include <boost/asio.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

namespace {

using boost::asio::ip::tcp;

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


int fix_sequence_number(
    const minimatch::FixMessage& message
) {
  const auto value = message.get(34);

  if (!value.has_value()) {
    throw std::runtime_error(
        "FIX message is missing MsgSeqNum"
    );
  }

  return std::stoi(*value);
}

std::string fix_message_type_code(
    const minimatch::FixMessage& message
) {
  return minimatch::fix_message_code(
      message.message_type
  );
}

std::string printable_fix(
    std::string wire
) {
  for (char& character : wire) {
    if (character == minimatch::FIX_SOH) {
      character = '|';
    }
  }

  return wire;
}

bool extract_fix_message(
    std::string& buffer,
    std::string& message
) {
  const std::size_t begin =
      buffer.find("8=FIX.");

  if (begin == std::string::npos) {
    if (buffer.size() > 8192) {
      buffer.clear();
    }

    return false;
  }

  if (begin > 0) {
    buffer.erase(0, begin);
  }

  const std::string checksum_marker =
      std::string(1, minimatch::FIX_SOH) +
      "10=";

  const std::size_t checksum =
      buffer.find(checksum_marker);

  if (checksum == std::string::npos) {
    return false;
  }

  const std::size_t checksum_value =
      checksum +
      checksum_marker.size();

  const std::size_t message_end =
      buffer.find(
          minimatch::FIX_SOH,
          checksum_value
      );

  if (message_end == std::string::npos) {
    return false;
  }

  message =
      buffer.substr(
          0,
          message_end + 1
      );

  buffer.erase(
      0,
      message_end + 1
  );

  return true;
}

class SharedTradingState {
 public:
  SharedTradingState()
      : gateway_(oms_) {}

  minimatch::FixGatewayResult handle(
      const minimatch::FixMessage& message,
      std::uint64_t timestamp_ns
  ) {
    const std::lock_guard<std::mutex> lock(
        mutex_
    );

    return gateway_.handle(
        message,
        timestamp_ns
    );
  }

 private:
  std::mutex mutex_;
  minimatch::OrderManagementSystem oms_;
  minimatch::FixOrderGateway gateway_;
};

void send_fix(
    tcp::socket& socket,
    minimatch::FixStore& store,
    const std::string& session_id,
    const minimatch::FixMessage& message,
    std::uint64_t timestamp_ns
) {
  const std::string wire =
      minimatch::encode_fix_message(message);

  store.append_message(
      minimatch::StoredFixMessage{
          .session_id = session_id,
          .direction = "OUT",
          .sequence_number =
              fix_sequence_number(message),
          .message_type =
              fix_message_type_code(message),
          .wire_message = wire,
          .timestamp_ns = timestamp_ns
      }
  );

  boost::asio::write(
      socket,
      boost::asio::buffer(wire)
  );

  std::cout
      << "FIX OUT "
      << printable_fix(wire)
      << '\n';
}

void handle_client(
    tcp::socket socket,
    std::shared_ptr<SharedTradingState> trading_state,
    std::shared_ptr<minimatch::FixStore> fix_store,
    std::string sender_comp_id,
    std::string target_comp_id,
    int heartbeat_seconds
) {
  try {
    const std::string session_id =
        sender_comp_id +
        "->" +
        target_comp_id;

    minimatch::FixSession session(
        minimatch::FixSessionConfig{
            .sender_comp_id =
                std::move(sender_comp_id),
            .target_comp_id =
                std::move(target_comp_id),
            .heartbeat_interval_seconds =
                heartbeat_seconds
        }
    );

    const auto snapshot =
        fix_store->load_session(
            session_id
        );

    session.restore_sequence_state(
        snapshot.next_incoming_sequence,
        snapshot.next_outgoing_sequence,
        snapshot.last_received_time_ns,
        snapshot.last_sent_time_ns
    );

    std::cout
        << "Restored FIX session "
        << session_id
        << " incoming="
        << session.expected_incoming_sequence()
        << " outgoing="
        << session.next_outgoing_sequence()
        << '\n';

    std::array<char, 8192> read_buffer{};
    std::string accumulated;

    for (;;) {
      boost::system::error_code error;

      const std::size_t bytes_read =
          socket.read_some(
              boost::asio::buffer(
                  read_buffer
              ),
              error
          );

      if (error ==
          boost::asio::error::eof) {
        break;
      }

      if (error) {
        throw boost::system::system_error(
            error
        );
      }

      accumulated.append(
          read_buffer.data(),
          bytes_read
      );

      std::string wire;

      while (extract_fix_message(
          accumulated,
          wire
      )) {
        std::cout
            << "FIX IN  "
            << printable_fix(wire)
            << '\n';

        const auto parsed =
            minimatch::parse_fix_message(
                wire
            );

        if (!parsed.valid) {
          std::cerr
              << "FIX parse rejected: "
              << parsed.error
              << '\n';

          continue;
        }

        const std::uint64_t timestamp =
            now_ns();

        fix_store->append_message(
            minimatch::StoredFixMessage{
                .session_id = session_id,
                .direction = "IN",
                .sequence_number =
                    fix_sequence_number(
                        parsed.message
                    ),
                .message_type =
                    fix_message_type_code(
                        parsed.message
                    ),
                .wire_message = wire,
                .timestamp_ns = timestamp
            }
        );

        const auto session_result =
            session.receive(
                parsed.message,
                timestamp
            );

        if (session_result.response.has_value()) {
          send_fix(
              socket,
              *fix_store,
              session_id,
              *session_result.response,
              timestamp
          );
        }

        fix_store->save_session(
            minimatch::FixSessionSnapshot{
                .session_id = session_id,
                .next_incoming_sequence =
                    session.expected_incoming_sequence(),
                .next_outgoing_sequence =
                    session.next_outgoing_sequence(),
                .last_received_time_ns =
                    session.last_received_time_ns(),
                .last_sent_time_ns =
                    session.last_sent_time_ns()
            }
        );

        if (!session_result.accepted) {
          std::cerr
              << "FIX session rejected: "
              << session_result.message
              << '\n';

          continue;
        }

        if (
            parsed.message.message_type ==
                minimatch::FixMessageType::ResendRequest &&
            session_result.resend_begin_sequence.has_value() &&
            session_result.resend_end_sequence.has_value()
        ) {
          const int begin_sequence =
              *session_result.resend_begin_sequence;

          const int end_sequence =
              *session_result.resend_end_sequence;

          const auto stored_messages =
              fix_store->outbound_messages_from_sequence(
                  session_id,
                  begin_sequence
              );

          int pending_gap_begin = 0;
          int pending_gap_end = 0;

          const auto send_gap_fill =
              [&](int gap_begin,
                  int gap_end) {
                if (gap_begin <= 0 ||
                    gap_end < gap_begin) {
                  return;
                }

                const auto gap_fill =
                    minimatch::create_fix_gap_fill(
                        gap_begin,
                        gap_end + 1,
                        sender_comp_id,
                        target_comp_id,
                        timestamp
                    );

                const std::string gap_wire =
                    minimatch::encode_fix_message(
                        gap_fill
                    );

                boost::asio::write(
                    socket,
                    boost::asio::buffer(
                        gap_wire
                    )
                );

                std::cout
                    << "FIX GAPFILL "
                    << printable_fix(
                        gap_wire
                    )
                    << '\n';
              };

          for (const auto& stored :
               stored_messages) {
            if (stored.sequence_number >
                end_sequence) {
              break;
            }

            const auto parsed_stored =
                minimatch::parse_fix_message(
                    stored.wire_message
                );

            if (!parsed_stored.valid) {
              std::cerr
                  << "Skipping invalid stored FIX message: "
                  << parsed_stored.error
                  << '\n';

              continue;
            }

            const bool administrative =
                parsed_stored.message.message_type ==
                    minimatch::FixMessageType::Logon ||
                parsed_stored.message.message_type ==
                    minimatch::FixMessageType::Logout ||
                parsed_stored.message.message_type ==
                    minimatch::FixMessageType::Heartbeat ||
                parsed_stored.message.message_type ==
                    minimatch::FixMessageType::TestRequest ||
                parsed_stored.message.message_type ==
                    minimatch::FixMessageType::ResendRequest ||
                parsed_stored.message.message_type ==
                    minimatch::FixMessageType::SequenceReset ||
                parsed_stored.message.message_type ==
                    minimatch::FixMessageType::Reject;

            if (administrative) {
              if (pending_gap_begin == 0) {
                pending_gap_begin =
                    stored.sequence_number;
              }

              pending_gap_end =
                  stored.sequence_number;

              continue;
            }

            if (pending_gap_begin != 0) {
              send_gap_fill(
                  pending_gap_begin,
                  pending_gap_end
              );

              pending_gap_begin = 0;
              pending_gap_end = 0;
            }

            const auto resent =
                minimatch::prepare_fix_resend(
                    parsed_stored.message,
                    timestamp
                );

            const std::string resend_wire =
                minimatch::encode_fix_message(
                    resent
                );

            boost::asio::write(
                socket,
                boost::asio::buffer(
                    resend_wire
                )
            );

            std::cout
                << "FIX RESEND "
                << printable_fix(
                    resend_wire
                )
                << '\n';
          }

          if (pending_gap_begin != 0) {
            send_gap_fill(
                pending_gap_begin,
                pending_gap_end
            );
          }

          continue;
        }

        const bool application_message =
            parsed.message.message_type ==
                minimatch::FixMessageType::
                    NewOrderSingle ||
            parsed.message.message_type ==
                minimatch::FixMessageType::
                    OrderCancelRequest;

        if (!application_message) {
          continue;
        }

        const auto gateway_result =
            trading_state->handle(
                parsed.message,
                timestamp
            );

        auto response =
            session.prepare_outbound(
                gateway_result.response,
                timestamp
            );

        send_fix(
            socket,
            *fix_store,
            session_id,
            response,
            timestamp
        );

        fix_store->save_session(
            minimatch::FixSessionSnapshot{
                .session_id = session_id,
                .next_incoming_sequence =
                    session.expected_incoming_sequence(),
                .next_outgoing_sequence =
                    session.next_outgoing_sequence(),
                .last_received_time_ns =
                    session.last_received_time_ns(),
                .last_sent_time_ns =
                    session.last_sent_time_ns()
            }
        );
      }
    }
  } catch (const std::exception& error) {
    std::cerr
        << "FIX client disconnected: "
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
            : 9876;

    const std::string sender_comp_id =
        argc > 2
            ? argv[2]
            : "MINIMATCH";

    const std::string target_comp_id =
        argc > 3
            ? argv[3]
            : "CLIENT";

    const int heartbeat_seconds =
        argc > 4
            ? std::stoi(argv[4])
            : 30;

    const std::string database_path =
        argc > 5
            ? argv[5]
            : "minimatch_fix.sqlite";

    auto fix_store =
        std::make_shared<
            minimatch::FixStore
        >(database_path);

    boost::asio::io_context io_context;

    tcp::acceptor acceptor(
        io_context,
        tcp::endpoint(
            tcp::v4(),
            port
        )
    );

    auto trading_state =
        std::make_shared<
            SharedTradingState
        >();

    std::cout
        << "MiniMatch FIX 4.4 server listening on 0.0.0.0:"
        << port
        << " sender="
        << sender_comp_id
        << " target="
        << target_comp_id
        << " database="
        << database_path
        << '\n';

    for (;;) {
      tcp::socket socket(io_context);
      acceptor.accept(socket);

      std::cout
          << "FIX client connected from "
          << socket.remote_endpoint()
          << '\n';

      std::thread(
          handle_client,
          std::move(socket),
          trading_state,
          fix_store,
          sender_comp_id,
          target_comp_id,
          heartbeat_seconds
      ).detach();
    }
  } catch (const std::exception& error) {
    std::cerr
        << "FIX server failed: "
        << error.what()
        << '\n';

    return EXIT_FAILURE;
  }
}
