#include "minimatch/queue_model.hpp"
#include <iostream>
int main() {
  minimatch::QueuePositionModel queue(120, 50);
  queue.on_cancel_ahead(20);
  queue.on_trade(70);
  queue.on_trade(45);
  const auto& s = queue.state();
  std::cout << "ahead=" << s.ahead << '\n'
            << "filled=" << s.filled << '\n'
            << "remaining=" << s.own_remaining << '\n'
            << "events=" << s.events << '\n';
}
