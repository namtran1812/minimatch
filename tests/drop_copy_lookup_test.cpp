#include "minimatch/drop_copy.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace {

using namespace minimatch;

TEST(
    DropCopyLookup,
    ReturnsOnlyEventsForRequestedOrder
) {
  const std::string path =
      "/tmp/minimatch_drop_copy_lookup_test.sqlite";

  std::filesystem::remove(
      path
  );

  {
    DropCopyStore store(
        path,
        100
    );

    store.publish(
        DropCopyEvent{
            .timestamp_ns = 100,
            .order_id = 11,
            .symbol = 1,
            .event_type = "OMS_FILL",
            .status = "FILLED",
            .remaining = 0,
            .reject_reason =
                RejectReason::None
        }
    );

    store.publish(
        DropCopyEvent{
            .timestamp_ns = 200,
            .order_id = 22,
            .symbol = 1,
            .event_type = "EXECUTION_REPORT",
            .status = "ACCEPTED",
            .remaining = 5,
            .reject_reason =
                RejectReason::None
        }
    );

    store.publish(
        DropCopyEvent{
            .timestamp_ns = 300,
            .order_id = 11,
            .symbol = 1,
            .event_type = "OMS_FILL",
            .status = "FILLED",
            .remaining = 0,
            .reject_reason =
                RejectReason::None
        }
    );

    const auto events =
        store.events_for_order(
            11
        );

    ASSERT_EQ(
        events.size(),
        2U
    );

    EXPECT_EQ(
        events[0].order_id,
        11U
    );

    EXPECT_EQ(
        events[1].order_id,
        11U
    );

    EXPECT_EQ(
        events[0].timestamp_ns,
        100U
    );

    EXPECT_EQ(
        events[1].timestamp_ns,
        300U
    );

    EXPECT_EQ(
        events[0].event_type,
        "OMS_FILL"
    );

    EXPECT_EQ(
        events[1].event_type,
        "OMS_FILL"
    );
  }

  std::filesystem::remove(
      path
  );
}

TEST(
    DropCopyLookup,
    PersistsLookupAcrossRestart
) {
  const std::string path =
      "/tmp/minimatch_drop_copy_restart_test.sqlite";

  std::filesystem::remove(
      path
  );

  {
    DropCopyStore store(
        path,
        10
    );

    store.publish(
        DropCopyEvent{
            .timestamp_ns = 500,
            .order_id = 77,
            .symbol = 1,
            .event_type = "OMS_FILL",
            .status = "FILLED",
            .remaining = 0,
            .reject_reason =
                RejectReason::None
        }
    );
  }

  {
    DropCopyStore restored(
        path,
        10
    );

    const auto events =
        restored.events_for_order(
            77
        );

    ASSERT_EQ(
        events.size(),
        1U
    );

    EXPECT_EQ(
        events[0].order_id,
        77U
    );

    EXPECT_EQ(
        events[0].timestamp_ns,
        500U
    );
  }

  std::filesystem::remove(
      path
  );
}

}  // namespace
