#include "minimatch/drop_copy.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>

using namespace minimatch;

int main(
    int argc,
    char** argv
) {
  std::size_t event_count =
      100000;

  bool single_match =
      false;

  if (argc > 1) {
    event_count =
        static_cast<std::size_t>(
            std::stoull(
                argv[1]
            )
        );
  }

  if (
      argc > 2 &&
      std::string(argv[2]) ==
          "single"
  ) {
    single_match =
        true;
  }

  const std::string path =
      "/tmp/minimatch_drop_copy_benchmark.sqlite";

  std::filesystem::remove(
      path
  );

  const OrderId target_order =
      424242;

  DropCopyStore store(
      path,
      event_count + 10
  );

  for (
      std::size_t i = 0;
      i < event_count;
      ++i
  ) {
    OrderId order_id =
        static_cast<OrderId>(
            i + 1
        );

    if (single_match) {
      if (
          i ==
          event_count / 2
      ) {
        order_id =
            target_order;
      }
    } else if (
        i % 100 == 0
    ) {
      order_id =
          target_order;
    }

    store.publish(
        DropCopyEvent{
            .timestamp_ns =
                static_cast<std::uint64_t>(
                    i + 1
                ),
            .order_id =
                order_id,
            .symbol =
                1,
            .event_type =
                "OMS_FILL",
            .status =
                "FILLED",
            .remaining =
                0,
            .reject_reason =
                RejectReason::None
        }
    );
  }

  constexpr std::size_t iterations =
      1000;

  const auto start =
      std::chrono::
          steady_clock::now();

  std::size_t total_events =
      0;

  for (
      std::size_t i = 0;
      i < iterations;
      ++i
  ) {
    const auto events =
        store.events_for_order(
            target_order
        );

    total_events +=
        events.size();
  }

  const auto end =
      std::chrono::
          steady_clock::now();

  const auto elapsed_ns =
      std::chrono::
          duration_cast<
              std::chrono::nanoseconds
          >(
              end - start
          )
          .count();

  const double avg_lookup_ns =
      static_cast<double>(
          elapsed_ns
      ) /
      static_cast<double>(
          iterations
      );

  std::cout
      << "Drop-copy lookup benchmark\n"
      << "events=" << event_count << "\n"
      << "mode="
      << (
          single_match
              ? "single"
              : "multi"
      )
      << "\n"
      << "iterations="
      << iterations
      << "\n"
      << "matches_per_lookup="
      << (
          total_events /
          iterations
      )
      << "\n"
      << "avg_lookup_ns="
      << avg_lookup_ns
      << "\n";

  std::filesystem::remove(
      path
  );

  return 0;
}
