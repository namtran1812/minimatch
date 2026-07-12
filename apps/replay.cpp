#include "minimatch/event_log.hpp"
#include "minimatch/exchange.hpp"
#include <iostream>
#include <type_traits>

using namespace minimatch;
int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: minimatch_replay event-log [snapshot-out]\n";
    return 2;
  }
  try {
    Exchange exchange;
    EventLogReader reader(argv[1]);
    InputEvent event;
    std::uint64_t count = 0;
    while (reader.next(event)) {
      std::visit([&](const auto& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, OrderRequest>) exchange.submit(value);
        else if constexpr (std::is_same_v<T, CancelRequest>) exchange.cancel(value);
        else exchange.replace(value);
      }, event);
      ++count;
    }
    if (argc > 2) exchange.write_snapshot(argv[2]);
    std::cout << "events=" << count << " input_sequence=" << reader.last_sequence()
              << " symbols=" << exchange.symbols().size()
              << " active=" << exchange.active_orders()
              << " state_hash=" << exchange.state_hash() << '\n';
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
