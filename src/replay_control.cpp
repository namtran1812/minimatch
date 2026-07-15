#include "minimatch/replay_control.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace minimatch {

namespace {

void validate_speed(double speed) {
  if (!std::isfinite(speed) ||
      speed < 0.0) {
    throw std::invalid_argument(
        "replay speed must be non-negative"
    );
  }
}

}  // namespace

std::string to_string(
    ReplayStatus status
) {
  switch (status) {
    case ReplayStatus::Ready:
      return "ready";

    case ReplayStatus::Running:
      return "running";

    case ReplayStatus::Paused:
      return "paused";

    case ReplayStatus::Complete:
      return "complete";

    case ReplayStatus::Stopped:
      return "stopped";
  }

  return "unknown";
}

ReplayController::ReplayController(
    double initial_speed
)
    : speed_(initial_speed) {
  validate_speed(initial_speed);
}

void ReplayController::apply(
    const ReplayCommand& command
) {
  switch (command.type) {
    case ReplayCommandType::Play:
      play();
      return;

    case ReplayCommandType::Pause:
      pause();
      return;

    case ReplayCommandType::SetSpeed:
      set_speed(command.speed);
      return;

    case ReplayCommandType::Restart:
      request_restart();
      return;

    case ReplayCommandType::SeekRecord:
      seek_record(
          command.record_index
      );
      return;

    case ReplayCommandType::Stop:
      stop();
      return;
  }

  throw std::invalid_argument(
      "unknown replay command"
  );
}

void ReplayController::play() {
  {
    std::lock_guard<std::mutex> lock(
        mutex_
    );

    if (status_ ==
        ReplayStatus::Stopped) {
      return;
    }

    status_ = ReplayStatus::Running;
    ++generation_;
  }

  condition_.notify_all();
}

void ReplayController::pause() {
  std::lock_guard<std::mutex> lock(
      mutex_
  );

  if (status_ ==
          ReplayStatus::Stopped ||
      status_ ==
          ReplayStatus::Complete) {
    return;
  }

  status_ = ReplayStatus::Paused;
  ++generation_;
}

void ReplayController::set_speed(
    double speed
) {
  validate_speed(speed);

  {
    std::lock_guard<std::mutex> lock(
        mutex_
    );

    speed_ = speed;
    ++generation_;
  }

  condition_.notify_all();
}

void ReplayController::request_restart() {
  {
    std::lock_guard<std::mutex> lock(
        mutex_
    );

    if (status_ ==
        ReplayStatus::Stopped) {
      return;
    }

    restart_requested_ = true;
    seek_record_.reset();

    current_record_ = 0;
    status_ = ReplayStatus::Running;

    ++generation_;
  }

  condition_.notify_all();
}

void ReplayController::seek_record(
    std::size_t record_index
) {
  {
    std::lock_guard<std::mutex> lock(
        mutex_
    );

    if (status_ ==
        ReplayStatus::Stopped) {
      return;
    }

    seek_record_ = record_index;
    restart_requested_ = false;

    current_record_ = record_index;

    ++generation_;
  }

  condition_.notify_all();
}

void ReplayController::stop() {
  {
    std::lock_guard<std::mutex> lock(
        mutex_
    );

    status_ = ReplayStatus::Stopped;
    ++generation_;
  }

  condition_.notify_all();
}

void ReplayController::mark_running() {
  {
    std::lock_guard<std::mutex> lock(
        mutex_
    );

    if (status_ !=
        ReplayStatus::Stopped) {
      status_ =
          ReplayStatus::Running;
      ++generation_;
    }
  }

  condition_.notify_all();
}

void ReplayController::mark_complete() {
  {
    std::lock_guard<std::mutex> lock(
        mutex_
    );

    if (status_ !=
        ReplayStatus::Stopped) {
      status_ =
          ReplayStatus::Complete;
      ++generation_;
    }
  }

  condition_.notify_all();
}

void ReplayController::update_progress(
    std::size_t current_record,
    std::size_t total_records
) {
  std::lock_guard<std::mutex> lock(
      mutex_
  );

  current_record_ = current_record;
  total_records_ = total_records;
}

void ReplayController::wait_until_runnable() {
  std::unique_lock<std::mutex> lock(
      mutex_
  );

  condition_.wait(
      lock,
      [this]() {
        return status_ !=
            ReplayStatus::Paused;
      }
  );
}

ReplayControlSnapshot
ReplayController::snapshot() const {
  std::lock_guard<std::mutex> lock(
      mutex_
  );

  return ReplayControlSnapshot{
      .status = status_,
      .speed = speed_,
      .current_record =
          current_record_,
      .total_records =
          total_records_,
      .generation = generation_,
      .restart_requested =
          restart_requested_,
      .seek_record = seek_record_
  };
}

bool ReplayController::
consume_restart_request() {
  std::lock_guard<std::mutex> lock(
      mutex_
  );

  const bool requested =
      restart_requested_;

  restart_requested_ = false;

  return requested;
}

std::optional<std::size_t>
ReplayController::consume_seek_request() {
  std::lock_guard<std::mutex> lock(
      mutex_
  );

  auto requested =
      seek_record_;

  seek_record_.reset();

  return requested;
}

bool ReplayController::stopped() const {
  std::lock_guard<std::mutex> lock(
      mutex_
  );

  return status_ ==
      ReplayStatus::Stopped;
}

}  // namespace minimatch
