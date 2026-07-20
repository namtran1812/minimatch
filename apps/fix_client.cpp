#include "minimatch/fix.hpp"
#include "minimatch/fix_store.hpp"

#include <boost/asio.hpp>

#include <array>
#include <cstdint>
#include <iostream>
#include <string>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

namespace {

std::uint64_t now_ns() {
    using namespace std::chrono;

    return static_cast<std::uint64_t>(
        duration_cast<nanoseconds>(
            system_clock::now().time_since_epoch()
        ).count()
    );
}

void send_message(
    tcp::socket& socket,
    const minimatch::FixMessage& message
) {
    const std::string wire =
        minimatch::encode_fix_message(message);

    asio::write(
        socket,
        asio::buffer(wire)
    );

    std::cout
        << "SENT: "
        << wire
        << '\n';
}

std::string read_some(
    tcp::socket& socket
) {
    std::array<char, 8192> buffer{};

    const std::size_t size =
        socket.read_some(
            asio::buffer(buffer)
        );

    return std::string(
        buffer.data(),
        size
    );
}

} // namespace

int main(
    int argc,
    char** argv
) {
    try {
        const std::string host =
            argc > 1
                ? argv[1]
                : "127.0.0.1";

        const std::string port =
            argc > 2
                ? argv[2]
                : "9876";

        asio::io_context io;

        tcp::resolver resolver(io);

        tcp::socket socket(io);

        asio::connect(
            socket,
            resolver.resolve(
                host,
                port
            )
        );

        std::cout
            << "Connected to FIX server "
            << host
            << ":"
            << port
            << '\n';

        minimatch::FixStore store(
            "data/fix/minimatch_fix.sqlite"
        );

        const auto snapshot =
            store.load_session(
                "MINIMATCH->CLIENT"
            );

        int sequence =
            snapshot.next_incoming_sequence;

        minimatch::FixMessage logon;

        logon.message_type =
            minimatch::FixMessageType::Logon;

        logon.set(
            34,
            std::to_string(sequence++)
        );

        logon.set(
            49,
            "CLIENT"
        );

        logon.set(
            56,
            "MINIMATCH"
        );

        logon.set(
            52,
            std::to_string(
                now_ns()
            )
        );

        logon.set(
            98,
            "0"
        );

        logon.set(
            108,
            "30"
        );

        send_message(
            socket,
            logon
        );

        std::cout
            << "RECV: "
            << read_some(socket)
            << '\n';

        minimatch::FixMessage order;

        order.message_type =
            minimatch::
                FixMessageType::
                    NewOrderSingle;

        order.set(
            34,
            std::to_string(sequence++)
        );

        order.set(
            49,
            "CLIENT"
        );

        order.set(
            56,
            "MINIMATCH"
        );

        order.set(
            52,
            std::to_string(
                now_ns()
            )
        );

        order.set(
            11,
            "FIX-ORDER-1"
        );

        order.set(
            55,
            "BTCUSD"
        );

        order.set(
            54,
            "1"
        );

        order.set(
            38,
            "0.50"
        );

        order.set(
            40,
            "2"
        );

        order.set(
            44,
            "62640.10"
        );

        order.set(
            59,
            "0"
        );

        send_message(
            socket,
            order
        );

        std::cout
            << "RECV: "
            << read_some(socket)
            << '\n';

        minimatch::FixMessage cancel;

        cancel.message_type =
            minimatch::
                FixMessageType::
                    OrderCancelRequest;

        cancel.set(
            34,
            std::to_string(sequence++)
        );

        cancel.set(
            49,
            "CLIENT"
        );

        cancel.set(
            56,
            "MINIMATCH"
        );

        cancel.set(
            52,
            std::to_string(
                now_ns()
            )
        );

        cancel.set(
            11,
            "FIX-CANCEL-1"
        );

        cancel.set(
            41,
            "FIX-ORDER-1"
        );

        cancel.set(
            55,
            "BTCUSD"
        );

        cancel.set(
            54,
            "1"
        );

        send_message(
            socket,
            cancel
        );

        std::cout
            << "RECV: "
            << read_some(socket)
            << '\n';

        socket.close();

        return 0;

    } catch (
        const std::exception& ex
    ) {
        std::cerr
            << "FIX client failed: "
            << ex.what()
            << '\n';

        return 1;
    }
}
