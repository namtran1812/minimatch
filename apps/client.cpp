#include "minimatch/protocol.hpp"
#include <boost/asio.hpp>
#include <iostream>
#include <string_view>

using boost::asio::ip::tcp;
using namespace minimatch;

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "usage:\n"
              << "  minimatch_client host port new id client symbol buy|sell limit|market|ioc|fok|post price qty\n"
              << "  minimatch_client host port cancel id client symbol\n"
              << "  minimatch_client host port replace id client symbol new_price new_qty\n";
    return 2;
  }
  try {
    boost::asio::io_context io;
    tcp::socket socket(io);
    socket.connect({boost::asio::ip::make_address(argv[1]),
                    static_cast<unsigned short>(std::stoi(argv[2]))});
    const std::string_view command = argv[3];
    if (command == "new" && argc == 11) {
      const std::string_view type = argv[8];
      OrderType order_type = OrderType::Limit;
      if (type == "market") order_type = OrderType::Market;
      else if (type == "ioc") order_type = OrderType::IOC;
      else if (type == "fok") order_type = OrderType::FOK;
      else if (type == "post") order_type = OrderType::PostOnly;
      OrderRequest request{std::stoull(argv[4]), static_cast<ClientId>(std::stoul(argv[5])),
                           std::string_view(argv[7]) == "buy" ? Side::Buy : Side::Sell,
                           order_type, std::stoll(argv[9]),
                           static_cast<Quantity>(std::stoul(argv[10])),
                           static_cast<SymbolId>(std::stoul(argv[6]))};
      const auto bytes = encode(request);
      boost::asio::write(socket, boost::asio::buffer(bytes));
    } else if (command == "cancel" && argc == 7) {
      CancelRequest request{std::stoull(argv[4]), static_cast<ClientId>(std::stoul(argv[5])),
                            static_cast<SymbolId>(std::stoul(argv[6]))};
      const auto bytes = encode(request);
      boost::asio::write(socket, boost::asio::buffer(bytes));
    } else if (command == "replace" && argc == 9) {
      ReplaceRequest request{std::stoull(argv[4]), static_cast<ClientId>(std::stoul(argv[5])),
                             std::stoll(argv[7]), static_cast<Quantity>(std::stoul(argv[8])),
                             static_cast<SymbolId>(std::stoul(argv[6]))};
      const auto bytes = encode(request);
      boost::asio::write(socket, boost::asio::buffer(bytes));
    } else {
      throw std::runtime_error("invalid client arguments");
    }
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
