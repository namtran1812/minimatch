#include "minimatch/replay_control.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <stdexcept>
#include <thread>

namespace {

using minimatch::ReplayCommand;
using minimatch::ReplayCommandType;
using minimatch::ReplayController;
using minimatch::ReplayStatus;

TEST(ReplayControl, StartsReady) {
  ReplayController controller(10.0);

  const auto state =
      controller.snapshot();

  EXPECT_EQ(
      state.status,
      ReplayStatus::Ready
  );

  EXPECT_DOUBLE_EQ(
      state.speed,
      10.0
  );

  EXPECT_EQ(
      state.current_record,
      0U
  );
}

TEST(ReplayControl, PlaysAndPauses) {
  ReplayController controller;

  controller.play();

  EXPECT_EQ(
      controller.snapshot().status,
      ReplayStatus::Running
  );

  controller.pause();

  EXPECT_EQ(
      controller.snapshot().status,
      ReplayStatus::Paused
  );

  controller.play();

  EXPECT_EQ(
      controller.snapshot().status,
      ReplayStatus::Running
  );
}

TEST(ReplayControl, ChangesSpeed) {
  ReplayController controller;

  controller.set_speed(100.0);

  EXPECT_DOUBLE_EQ(
      controller.snapshot().speed,
      100.0
  );

  controller.set_speed(0.0);

  EXPECT_DOUBLE_EQ(
      controller.snapshot().speed,
      0.0
  );
}

TEST(ReplayControl, RejectsInvalidSpeed) {
  EXPECT_THROW(
      ReplayController(-1.0),
      std::invalid_argument
  );

  ReplayController controller;

  EXPECT_THROW(
      controller.set_speed(-1.0),
      std::invalid_argument
  );
}

TEST(ReplayControl, RequestsRestart) {
  ReplayController controller;

  controller.update_progress(
      400,
      600
  );

  controller.request_restart();

  const auto state =
      controller.snapshot();

  EXPECT_TRUE(
      state.restart_requested
  );

  EXPECT_EQ(
      state.current_record,
      0U
  );

  EXPECT_TRUE(
      controller.consume_restart_request()
  );

  EXPECT_FALSE(
      controller.consume_restart_request()
  );
}

TEST(ReplayControl, RequestsSeek) {
  ReplayController controller;

  controller.seek_record(350);

  const auto state =
      controller.snapshot();

  ASSERT_TRUE(
      state.seek_record.has_value()
  );

  EXPECT_EQ(
      *state.seek_record,
      350U
  );

  const auto request =
      controller.consume_seek_request();

  ASSERT_TRUE(request.has_value());
  EXPECT_EQ(*request, 350U);

  EXPECT_FALSE(
      controller
          .consume_seek_request()
          .has_value()
  );
}

TEST(ReplayControl, TracksProgress) {
  ReplayController controller;

  controller.update_progress(
      250,
      601
  );

  const auto state =
      controller.snapshot();

  EXPECT_EQ(
      state.current_record,
      250U
  );

  EXPECT_EQ(
      state.total_records,
      601U
  );
}

TEST(ReplayControl, WaitsWhilePaused) {
  ReplayController controller;

  controller.pause();

  auto waiter =
      std::async(
          std::launch::async,
          [&controller]() {
            controller
                .wait_until_runnable();

            return true;
          }
      );

  EXPECT_EQ(
      waiter.wait_for(
          std::chrono::milliseconds(50)
      ),
      std::future_status::timeout
  );

  controller.play();

  EXPECT_EQ(
      waiter.wait_for(
          std::chrono::seconds(1)
      ),
      std::future_status::ready
  );

  EXPECT_TRUE(waiter.get());
}

TEST(ReplayControl, StopReleasesWaiter) {
  ReplayController controller;

  controller.pause();

  auto waiter =
      std::async(
          std::launch::async,
          [&controller]() {
            controller
                .wait_until_runnable();

            return true;
          }
      );

  controller.stop();

  EXPECT_EQ(
      waiter.wait_for(
          std::chrono::seconds(1)
      ),
      std::future_status::ready
  );

  EXPECT_TRUE(controller.stopped());
}

TEST(ReplayControl, AppliesCommands) {
  ReplayController controller;

  controller.apply(
      ReplayCommand{
          .type =
              ReplayCommandType::SetSpeed,
          .speed = 25.0
      }
  );

  EXPECT_DOUBLE_EQ(
      controller.snapshot().speed,
      25.0
  );

  controller.apply(
      ReplayCommand{
          .type =
              ReplayCommandType::SeekRecord,
          .record_index = 125
      }
  );

  EXPECT_EQ(
      controller.snapshot()
          .current_record,
      125U
  );

  controller.apply(
      ReplayCommand{
          .type =
              ReplayCommandType::Stop
      }
  );

  EXPECT_TRUE(controller.stopped());
}


TEST(ReplayControl, RequestsTimestampSeek) {
  ReplayController controller;

  controller.seek_timestamp(
      1'784'099'700'000'000'000ULL
  );

  const auto state =
      controller.snapshot();

  ASSERT_TRUE(
      state.seek_timestamp_ns.has_value()
  );

  EXPECT_EQ(
      *state.seek_timestamp_ns,
      1'784'099'700'000'000'000ULL
  );

  const auto request =
      controller
          .consume_timestamp_seek_request();

  ASSERT_TRUE(request.has_value());

  EXPECT_EQ(
      *request,
      1'784'099'700'000'000'000ULL
  );

  EXPECT_FALSE(
      controller
          .consume_timestamp_seek_request()
          .has_value()
  );
}

TEST(ReplayControl, TimestampSeekClearsRecordSeek) {
  ReplayController controller;

  controller.seek_record(200);
  controller.seek_timestamp(5000);

  const auto state =
      controller.snapshot();

  EXPECT_FALSE(
      state.seek_record.has_value()
  );

  ASSERT_TRUE(
      state.seek_timestamp_ns.has_value()
  );

  EXPECT_EQ(
      *state.seek_timestamp_ns,
      5000U
  );
}


}  // namespace
