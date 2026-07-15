#include "minimatch/latency_stats.hpp"

#include <gtest/gtest.h>

namespace {

using minimatch::LatencyStats;

TEST(LatencyStats, StartsEmpty) {
  LatencyStats stats;

  const auto snapshot =
      stats.snapshot("normalize");

  EXPECT_EQ(snapshot.count, 0U);
  EXPECT_EQ(snapshot.p50_ns, 0U);
  EXPECT_EQ(snapshot.p95_ns, 0U);
  EXPECT_EQ(snapshot.p99_ns, 0U);
}

TEST(LatencyStats, TracksSummaryValues) {
  LatencyStats stats;

  stats.record("normalize", 10);
  stats.record("normalize", 20);
  stats.record("normalize", 30);

  const auto snapshot =
      stats.snapshot("normalize");

  EXPECT_EQ(snapshot.count, 3U);
  EXPECT_EQ(snapshot.minimum_ns, 10U);
  EXPECT_EQ(snapshot.maximum_ns, 30U);
  EXPECT_DOUBLE_EQ(
      snapshot.average_ns,
      20.0
  );

  EXPECT_EQ(snapshot.p50_ns, 20U);
}

TEST(LatencyStats, TracksPercentiles) {
  LatencyStats stats;

  for (std::uint64_t value = 1;
       value <= 100;
       ++value) {
    stats.record(
        "book_update",
        value
    );
  }

  const auto snapshot =
      stats.snapshot("book_update");

  EXPECT_EQ(snapshot.p50_ns, 50U);
  EXPECT_EQ(snapshot.p95_ns, 95U);
  EXPECT_EQ(snapshot.p99_ns, 99U);
}

TEST(LatencyStats, UsesBoundedSampleWindow) {
  LatencyStats stats(3);

  stats.record("route", 10);
  stats.record("route", 20);
  stats.record("route", 30);
  stats.record("route", 100);

  const auto snapshot =
      stats.snapshot("route");

  EXPECT_EQ(snapshot.count, 4U);
  EXPECT_EQ(snapshot.maximum_ns, 100U);
  EXPECT_EQ(snapshot.p50_ns, 30U);
}

TEST(LatencyStats, ReturnsSortedSnapshots) {
  LatencyStats stats;

  stats.record("serialize", 5);
  stats.record("book_update", 4);
  stats.record("normalize", 3);

  const auto snapshots =
      stats.snapshots();

  ASSERT_EQ(snapshots.size(), 3U);

  EXPECT_EQ(
      snapshots[0].name,
      "book_update"
  );

  EXPECT_EQ(
      snapshots[1].name,
      "normalize"
  );

  EXPECT_EQ(
      snapshots[2].name,
      "serialize"
  );
}

TEST(LatencyStats, ResetsSeries) {
  LatencyStats stats;

  stats.record("normalize", 100);
  stats.reset("normalize");

  EXPECT_EQ(
      stats.snapshot("normalize").count,
      0U
  );
}

TEST(LatencyStats, RejectsEmptyName) {
  LatencyStats stats;

  EXPECT_THROW(
      stats.record("", 100),
      std::invalid_argument
  );
}

TEST(LatencyStats, RejectsZeroCapacity) {
  EXPECT_THROW(
      LatencyStats(0),
      std::invalid_argument
  );
}

}  // namespace
