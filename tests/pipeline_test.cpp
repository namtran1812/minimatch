#include "minimatch/pipeline.hpp"
#include "minimatch/spsc_ring.hpp"
#include <gtest/gtest.h>
#include <thread>
using namespace minimatch;
TEST(SpscRing, PreservesFifoOrder) {
  SpscRing<int, 8> q; EXPECT_TRUE(q.try_push(1)); EXPECT_TRUE(q.try_push(2)); int x{};
  EXPECT_TRUE(q.try_pop(x)); EXPECT_EQ(x, 1); EXPECT_TRUE(q.try_pop(x)); EXPECT_EQ(x, 2); EXPECT_TRUE(q.empty());
}
TEST(Pipeline, ProcessesCommandsDeterministically) {
  MatchingPipeline p; p.start();
  PipelineCommand a; a.order = {1,1,Side::Sell,OrderType::Limit,100,10,1};
  PipelineCommand b; b.order = {2,2,Side::Buy,OrderType::Limit,100,4,1};
  EXPECT_TRUE(p.submit(a)); EXPECT_TRUE(p.submit(b));
  while (p.metrics().processed < 2) std::this_thread::yield();
  p.stop(); EXPECT_EQ(p.active_orders(), 1u); EXPECT_EQ(p.metrics().trades, 1u); EXPECT_NE(p.state_hash(), 0u);
}
