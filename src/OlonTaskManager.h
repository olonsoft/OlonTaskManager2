#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <Arduino.h>
#include <string>
#include <functional>
#include <vector>
#include <algorithm>

namespace Olon {

namespace TaskDuration {
  constexpr uint32_t MILLISECONDS = 1U;
  constexpr uint32_t SECONDS = 1000U;
  constexpr uint32_t MINUTES = 60 * SECONDS;
  constexpr uint32_t HOURS = 60 * MINUTES;
}  // namespace TaskDuration

class Task {
 public:
  using TaskCallback = std::function<void(void* params)>;
  using TaskRunIfCallback = std::function<bool()>;
  using TaskDoneCallback = std::function<void(const Task& me, const uint32_t elapsed)>;
  // Default value for runOnce
  static constexpr bool RunOnce = true;

  // Constructor
  Task(const char* name, TaskCallback callback)
      : _name(name),
        _taskCallback(std::move(callback)) {}

  Task(const char* name, bool runOnce, TaskCallback callback)
      : Task(name, std::move(callback)) {
    setRunOnce(runOnce);
  }

  // Non-copyable
  Task(const Task&) = delete;
  Task& operator=(const Task&) = delete;

  // Moveable
  Task(Task&&) = default;
  Task& operator=(Task&&) = default;

  // Virtual destructor for proper cleanup
  virtual ~Task() = default;

  [[nodiscard]] const char* getName() const { return _name; }
  [[nodiscard]] bool getRunOnce() const { return _runOnce; }
  [[nodiscard]] bool isEnabled() const { return _enabled; }
  [[nodiscard]] bool isPaused() const { return _paused; }
  [[nodiscard]] bool isRunning() const { return _running; }
  [[nodiscard]] void* getData() const { return _data; }
  [[nodiscard]] bool isReadyToRun() const {
    return _enabled && !_paused && (!_taskRunIfCallback || _taskRunIfCallback());
  }

  void setRunOnce(bool runOnce) {
    _runOnce = runOnce;
    _paused = _runOnce;
  }

  void setEnabled(bool enabled) { _enabled = enabled; }

  void pause() { _paused = true; }

  void resume(uint32_t delayMillis = 0) {
    _paused = false;
    if (delayMillis) {
      setInterval(delayMillis);
      _lastRun = millis();
    }
  }

  void setRunIf(TaskRunIfCallback condition) {
    _taskRunIfCallback = std::move(condition);
  }

  void setInterval(uint32_t intervalMillis) {
    _intervalMillis = intervalMillis;
    _runImmediately = false;
  }

  void setImmediateInterval(uint32_t intervalMillis) {
    _intervalMillis = intervalMillis;
    _runImmediately = true;
  }

  void setDoneCallback(TaskDoneCallback callback) {
    _taskDoneCallback = std::move(callback);
  }

  void setData(void* params) { _data = params; }

  void requestEarlyRun() { _lastRun = 0; }

  bool tryRun() {
    if (_running || !isReadyToRun()) return false;

    const uint32_t now = safeMillis();
    if (_runImmediately || _intervalMillis == 0 ||
        timeDifference(now, _lastRun) >= _intervalMillis) {
      _runImmediately = false;
      forceRun();
      return true;
    }
    return false;
  }

  void forceRun() {
    uint32_t start = safeMillis();
    uint32_t start_us = micros();
    _running = true;
    _taskCallback(_data);
    _lastRun = start;
    _running = false;

    uint32_t elapsed = micros() - start_us;

    if (_runOnce) _paused = true;

    if (_taskDoneCallback) {
      _taskDoneCallback(*this, elapsed);
    }
  }

 private:
  // Handle millis() overflow safely
  [[nodiscard]] static uint32_t safeMillis() { return millis(); }

  // Safe time difference calculation accounting for overflow
  [[nodiscard]] static uint32_t timeDifference(uint32_t current,
                                               uint32_t previous) {
    return (current >= previous) ? (current - previous)
                                 : (UINT32_MAX - previous + current + 1);
  }

  // follow the constructots order to avoid warnings
  const char* _name;
  TaskCallback _taskCallback = nullptr;
  TaskRunIfCallback _taskRunIfCallback = nullptr;
  TaskDoneCallback _taskDoneCallback = nullptr;

  void* _data = nullptr;

  bool _runOnce = false;
  bool _enabled = true;
  bool _paused = false;
  bool _running = false;
  bool _runImmediately = false;

  uint32_t _intervalMillis = 0;
  mutable uint32_t _lastRun = 0;

};

// ----------------

class TaskManager {
 public:
  explicit TaskManager(std::string name) : _name(std::move(name)) {}

  // you should not delete the tasks in the destructor. Instead, whoever adds
  // the tasks should manage their lifetime for (auto& task : _tasks) {
  //     delete task;

  // Delete copy constructor and assignment
  TaskManager(const TaskManager&) = delete;
  TaskManager& operator=(const TaskManager&) = delete;

  // Allow move operations
  TaskManager(TaskManager&&) = default;
  TaskManager& operator=(TaskManager&&) = default;

  ~TaskManager() = default;

  /**
   * Adds a task to the manager. The task must remain valid for the lifetime of the TaskManager.
   * @param task Pointer to the task to add. Must not be null.
   * @note The TaskManager does not take ownership of the task.
   */
  void addTask(Task* task) {
    if (task && !containsTask(task)) {
      _tasks.push_back(task);
    }
  }

  void removeTask(Task* task) {
    _tasks.erase(std::remove(_tasks.begin(), _tasks.end(), task), _tasks.end());
  }

  [[nodiscard]] const std::string& getName() const { return _name; }

  [[nodiscard]] size_t getTaskCount() const { return _tasks.size(); }

  size_t loop() {
    size_t count = 0;
    for (auto& task : _tasks) {
      if (task && task->tryRun()) count++;
    }
    return count;
  }

  void pause() {
    for (auto& task : _tasks) {
      if (task) task->pause();
    }
  }

  void resume() {
    for (auto& task : _tasks) {
      if (task) task->resume();
    }
  }

 private:
  [[nodiscard]] bool containsTask(const Task* task) const {
    return std::find(_tasks.begin(), _tasks.end(), task) != _tasks.end();
  }

  std::string _name;
  std::vector<Task*> _tasks;
};

}  // namespace Olon

#endif  // TASK_MANAGER_H
