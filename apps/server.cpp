#include "minimatch/event_log.hpp"
#include "minimatch/exchange.hpp"
#include "minimatch/protocol.hpp"
#include <boost/asio.hpp>
#include <array>
#include <cstring>
#include <iostream>
#include <memory>

using boost::asio::ip::tcp;
using namespace minimatch;

class Session : public std::enable_shared_from_this<Session> {
 public:
  Session(tcp::socket socket, Exchange& exchange, EventLogWriter& log)
      : socket_(std::move(socket)), exchange_(exchange), log_(log) {}
  void start() { read_header(); }
 private:
  void read_header() {
    auto self = shared_from_this();
    boost::asio::async_read(socket_, boost::asio::buffer(&header_, sizeof(header_)),
      [this, self](boost::system::error_code error, std::size_t) {
        if (error) return;
        if (header_.magic != 0x4D4D4154 || header_.version != 2 ||
            header_.length > payload_.size()) return;
        read_payload();
      });
  }
  void read_payload() {
    auto self = shared_from_this();
    boost::asio::async_read(socket_, boost::asio::buffer(payload_.data(), header_.length),
      [this, self](boost::system::error_code error, std::size_t) {
        if (error) return;
        if (header_.type == MessageType::NewOrder && header_.length == sizeof(WireOrder)) {
          WireOrder wire{};
          std::memcpy(&wire, payload_.data(), sizeof(wire));
          const auto request = decode_order(wire);
          log_.append(request);
          log_.flush();
          exchange_.submit(request);
        } else if (header_.type == MessageType::Cancel && header_.length == sizeof(WireCancel)) {
          WireCancel wire{};
          std::memcpy(&wire, payload_.data(), sizeof(wire));
          const auto request = decode_cancel(wire);
          log_.append(request);
          log_.flush();
          exchange_.cancel(request);
        } else if (header_.type == MessageType::Replace && header_.length == sizeof(WireReplace)) {
          WireReplace wire{};
          std::memcpy(&wire, payload_.data(), sizeof(wire));
          const auto request = decode_replace(wire);
          log_.append(request);
          log_.flush();
          exchange_.replace(request);
        }
        read_header();
      });
  }
  tcp::socket socket_;
  Exchange& exchange_;
  EventLogWriter& log_;
  WireHeader header_{};
  std::array<std::byte, 128> payload_{};
};

int main(int argc, char** argv) {
  try {
    const auto port = argc > 1 ? static_cast<unsigned short>(std::stoi(argv[1])) : 9001;
    const std::string log_path = argc > 2 ? argv[2] : "minimatch.events";
    boost::asio::io_context io;
    Exchange exchange;
    EventLogWriter log(log_path);
    exchange.on_trade([](const Trade& trade) {
      std::cout << "TRADE symbol=" << trade.symbol << " seq=" << trade.sequence
                << " maker=" << trade.maker_order_id << " taker=" << trade.taker_order_id
                << " px=" << trade.price << " qty=" << trade.quantity << '\n';
    });
    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), port));
    std::function<void()> accept;
    accept = [&] {
      acceptor.async_accept([&](boost::system::error_code error, tcp::socket socket) {
        if (!error) std::make_shared<Session>(std::move(socket), exchange, log)->start();
        accept();
      });
    };
    accept();
    std::cout << "MiniMatch v2 listening on " << port << '\n';
    io.run();
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
