#pragma once

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>

namespace minimatch {

enum class ReplayStatus {
  Ready,
  Running,
  Paused,
  Complete,
  Stopped
};

enum class ReplayCommandType {
  Play,
  Pause,
  SetSpeed,
  Restart,
  SeekRecord,
  Stop
};

struct ReplayCommand {
  ReplayCommandType type{
      ReplayCommandType::Play
  };

  double speed{1.0};
  std::size_t record_index{0};
};

struct ReplayControlSnapshot {
  ReplayStatus status{
      ReplayStatus::Ready
  };

  double speed{1.0};

  std::size_t current_record{0};
  std::size_t total_records{0};

  std::uint64_t generation{0};

  bool restart_requested{false};

  std::optional<std::size_t>
      seek_record;
};

class ReplayController {
 public:
  explicit ReplayController(
      double initial_speed = 1.0
  );

  void apply(
      const ReplayCommand& command
  );

  void play();
  void pause();

  void set_speed(double speed);

  void request_restart();

  void seek_record(
      std::size_t record_index
  );

  void stop();

  void mark_running();

  void mark_complete();

  void update_progress(
      std::size_t current_record,
      std::size_t total_records
  );

  void wait_until_runnable();

  [[nodiscard]]
  ReplayControlSnapshot snapshot() const;

  [[nodiscard]]
  bool consume_restart_request();

  [[nodiscard]]
  std::optional<std::size_t>
  consume_seek_request();

  [[nodiscard]]
  bool stopped() const;

 private:
  mutable std::mutex mutex_;
  std::condition_variable condition_;

  ReplayStatus status_{
      ReplayStatus::Ready
  };

  double speed_{1.0};

  std::size_t current_record_{0};
  std::size_t total_records_{0};

  std::uint64_t generation_{0};

  bool restart_requested_{false};

  std::optional<std::size_t>
      seek_record_;
};

[[nodiscard]]
std::string to_string(
    ReplayStatus status
);

}  // namespace minimatch
