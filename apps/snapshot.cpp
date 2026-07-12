#include "minimatch/exchange.hpp"
#include <iostream>
using namespace minimatch;
int main(int argc, char** argv) {
  const std::string path = argc > 1 ? argv[1] : "minimatch.snapshot";
  try {
    Exchange before(1024);
    before.submit({1, 1, Side::Buy, OrderType::Limit, 100, 10, 1});
    before.submit({2, 2, Side::Sell, OrderType::Limit, 102, 7, 1});
    before.submit({3, 1, Side::Buy, OrderType::Limit, 200, 4, 2});
    before.write_snapshot(path);
    Exchange after(1024);
    after.load_snapshot(path);
    std::cout << "before_hash=" << before.state_hash() << " after_hash=" << after.state_hash()
              << " active=" << after.active_orders() << '\n';
    return before.state_hash() == after.state_hash() ? 0 : 1;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
