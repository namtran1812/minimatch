#include "minimatch/venue_health.hpp"

#include <gtest/gtest.h>

namespace {

using minimatch::VenueHealthConfig;
using minimatch::VenueHealthMonitor;
using minimatch::VenueHealthStatus;

VenueHealthMonitor make_monitor() {
  return VenueHealthMonitor(
      VenueHealthConfig{
          .delayed_after_ns =
              100'000'000ULL,
          .stale_after_ns =
              200'000'000ULL,
          .disconnected_after_ns =
              500'000'000ULL,
          .rate_window_ns =
              1'000'000'000ULL
      }
  );
}

TEST(VenueHealth, StartsUnknown) {
  auto monitor = make_monitor();

  const auto state =
      monitor.snapshot(
          "COINBASE",
          1'000'000'000ULL
      );

  EXPECT_EQ(
      state.status,
      VenueHealthStatus::Unknown
  );

  EXPECT_FALSE(state.synchronized);
  EXPECT_FALSE(
      monitor.routable(
          "COINBASE",
          1'000'000'000ULL
      )
  );
}

TEST(VenueHealth, TracksHealthyVenue) {
  auto monitor = make_monitor();

  monitor.record_snapshot(
      "COINBASE",
      1'000'000'000ULL
  );

  monitor.set_synchronized(
      "COINBASE",
      true
  );

  const auto state =
      monitor.snapshot(
          "COINBASE",
          1'050'000'000ULL
      );

  EXPECT_EQ(
      state.status,
      VenueHealthStatus::Healthy
  );

  EXPECT_EQ(state.snapshot_count, 1U);
  EXPECT_EQ(state.message_count, 1U);

  EXPECT_TRUE(
      monitor.routable(
          "COINBASE",
          1'050'000'000ULL
      )
  );
}

TEST(VenueHealth, MarksDelayedVenue) {
  auto monitor = make_monitor();

  monitor.record_update(
      "KRAKEN",
      1'000'000'000ULL
  );

  monitor.set_synchronized(
      "KRAKEN",
      true
  );

  const auto state =
      monitor.snapshot(
          "KRAKEN",
          1'150'000'000ULL
      );

  EXPECT_EQ(
      state.status,
      VenueHealthStatus::Delayed
  );

  EXPECT_TRUE(
      monitor.routable(
          "KRAKEN",
          1'150'000'000ULL
      )
  );
}

TEST(VenueHealth, ExcludesStaleVenue) {
  auto monitor = make_monitor();

  monitor.record_update(
      "KRAKEN",
      1'000'000'000ULL
  );

  monitor.set_synchronized(
      "KRAKEN",
      true
  );

  const auto state =
      monitor.snapshot(
          "KRAKEN",
          1'250'000'000ULL
      );

  EXPECT_EQ(
      state.status,
      VenueHealthStatus::Stale
  );

  EXPECT_FALSE(
      monitor.routable(
          "KRAKEN",
          1'250'000'000ULL
      )
  );
}

TEST(VenueHealth, MarksDisconnectedVenue) {
  auto monitor = make_monitor();

  monitor.record_update(
      "BINANCE",
      1'000'000'000ULL
  );

  monitor.set_synchronized(
      "BINANCE",
      true
  );

  const auto state =
      monitor.snapshot(
          "BINANCE",
          1'600'000'000ULL
      );

  EXPECT_EQ(
      state.status,
      VenueHealthStatus::Disconnected
  );

  EXPECT_FALSE(
      monitor.routable(
          "BINANCE",
          1'600'000'000ULL
      )
  );
}

TEST(VenueHealth, UnsynchronizedVenueIsDisconnected) {
  auto monitor = make_monitor();

  monitor.record_snapshot(
      "KRAKEN",
      1'000'000'000ULL
  );

  const auto state =
      monitor.snapshot(
          "KRAKEN",
          1'010'000'000ULL
      );

  EXPECT_EQ(
      state.status,
      VenueHealthStatus::Disconnected
  );
}

TEST(VenueHealth, TracksOperationalCounters) {
  auto monitor = make_monitor();

  monitor.record_reconnect("KRAKEN");
  monitor.record_rejection("KRAKEN");
  monitor.record_sequence_gap("KRAKEN");
  monitor.record_checksum_error("KRAKEN");

  const auto state =
      monitor.snapshot(
          "KRAKEN",
          1'000'000'000ULL
      );

  EXPECT_EQ(state.reconnect_count, 1U);
  EXPECT_EQ(state.rejected_count, 1U);
  EXPECT_EQ(state.sequence_gap_count, 1U);
  EXPECT_EQ(state.checksum_error_count, 1U);
}

TEST(VenueHealth, TracksMessageRate) {
  auto monitor = make_monitor();

  monitor.record_snapshot(
      "COINBASE",
      1'000'000'000ULL
  );

  monitor.record_update(
      "COINBASE",
      1'100'000'000ULL
  );

  monitor.record_update(
      "COINBASE",
      1'200'000'000ULL
  );

  const auto state =
      monitor.snapshot(
          "COINBASE",
          1'500'000'000ULL
      );

  EXPECT_DOUBLE_EQ(
      state.messages_per_second,
      6.0
  );
}

TEST(VenueHealth, ReturnsSortedSnapshots) {
  auto monitor = make_monitor();

  monitor.record_update(
      "KRAKEN",
      1'000'000'000ULL
  );

  monitor.record_update(
      "BINANCE",
      1'000'000'000ULL
  );

  monitor.record_update(
      "COINBASE",
      1'000'000'000ULL
  );

  const auto states =
      monitor.snapshots(1'050'000'000ULL);

  ASSERT_EQ(states.size(), 3U);

  EXPECT_EQ(states[0].venue, "BINANCE");
  EXPECT_EQ(states[1].venue, "COINBASE");
  EXPECT_EQ(states[2].venue, "KRAKEN");
}

}  // namespace
