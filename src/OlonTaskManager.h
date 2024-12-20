#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <Arduino.h>

#include <vector>

namespace Olon {

namespace TaskDuration {
constexpr uint32_t MILLISECONDS = 1U;
constexpr uint32_t SECONDS = 1000U;
constexpr uint32_t MINUTES = 60 * SECONDS;
constexpr uint32_t HOURS = 60 * MINUTES;
}  // namespace TaskDuration

class Task {
 public:
  typedef std::function<void(void* params)> TaskFunction;
  typedef std::function<bool()> TaskRunIfFunction;
  typedef std::function<void(const Task& me, const uint32_t elapsed)> TaskDoneCallbackFunction;
  static const bool RunOnce = true;

  // Constructor
  Task(const char* name, TaskFunction fn)
      : _name(name),
        _fn(fn),
        _runOnce(false),
        _intervalMillis(0),
        _lastRun(0),
        _enabled(true),
        _paused(false) {}

  Task(const char* name, bool runOnce, TaskFunction fn) : Task(name, fn) {
    setRunOnce(runOnce);
  }

  // Destructor
  ~Task() {}

  const char* getName() const { return _name; }

  void setRunOnce(bool runOnce) {
    _runOnce = runOnce;
    _paused = _runOnce;
  }

  bool getRunOnce() const { return _runOnce; }

  void setEnabled(bool enabled) {
    _enabled = enabled;
    if (enabled && !_wasPending) {
      _lastRun = millis();  // Reset last run time when task becomes enabled
    }
    _wasPending = enabled;
  }

  bool isCapable() const {
    return !_taskRunIf || _taskRunIf();
  }

  bool isEnabled() const {
    return _enabled;
  }

  void pause() { _paused = true; }

  bool isPaused() const { return _paused; }

  void resume(uint32_t delayMillis = 0) {
    _paused = false;
    if (delayMillis) {
      setInterval(delayMillis);
      _lastRun = millis();
    }
  }

  bool isRunning() const { return _running; }

  void setRunIf(TaskRunIfFunction taskRunIf) {
    _taskRunIf = taskRunIf;
    // Check if enable state changed with new taskCanRun
    bool pending = isPending();
    if (pending && !_wasPending) {
      _lastRun = millis();  // Reset last run time when task becomes enabled
    }
    _wasPending = pending;
  }

  void setInterval(uint32_t intervalMillis) {
    _intervalMillis = intervalMillis;
    _runImmediately = false;
  }

  void setImmediateInterval(uint32_t intervalMillis) {
    _intervalMillis = intervalMillis;
	  _runImmediately = true;
  }

  void setDoneCallback(TaskDoneCallbackFunction doneCallback) {
    _doneCallback = doneCallback;
  }

  void setData(void* params) { _data = params; }

  void* getData() const { return _data; }

  bool tryRun() {
    if (_paused || _running || !isPending()) {
      return false;
    }

  if (_runImmediately || _intervalMillis == 0 || millis() - _lastRun >= _intervalMillis) {
      _runImmediately = false;
      forceRun();
      return true;
    }
    return false;
  }

  void forceRun() {
    uint32_t start = millis();
    uint32_t start_us = micros();
    _running = true;
    _fn(_data);
    _lastRun = start;
    _running = false;

    uint32_t elapsed = micros() - start_us;

    if (_runOnce) {
      _paused = true;
    }

    if (_doneCallback) {
      _doneCallback(*this, elapsed);
    }
  }

  void requestEarlyRun() { _lastRun = 0; }

 private:
  bool isPending() const {
    bool pending = isEnabled() && isCapable();

    // Update last run time when task transitions from disabled to enabled
    if (pending && !_wasPending) {
      _lastRun = millis();
    }
    _wasPending = pending;

    return pending;
  }
  const char* _name;
  TaskFunction _fn;
  bool _runOnce = false;
  TaskRunIfFunction _taskRunIf = nullptr;
  TaskDoneCallbackFunction _doneCallback = nullptr;
  void* _data = nullptr;
  uint32_t _intervalMillis;
  mutable uint32_t _lastRun;
  bool _enabled = true;
  bool _paused = false;
  bool _running = false;
  bool _runImmediately = false;
  mutable bool _wasPending = true;
};

// ----------------

class TaskManager {
 public:
  explicit TaskManager(const char* name) : _name(name) {}

  // Destructor to clean up tasks
  ~TaskManager() {
    // you should not delete the tasks in the destructor. Instead, whoever adds
    // the tasks should manage their lifetime for (auto& task : _tasks) {
    //     delete task;
    // }
    _tasks.clear();
  }

  void addTask(Task* task) { _tasks.push_back(task); }

  // Remove a specific task
  void removeTask(Task* task) {
    _tasks.erase(std::remove(_tasks.begin(), _tasks.end(), task), _tasks.end());
  }

  const char* getName() const { return _name; }

  // Loop through tasks and execute if they are ready to run
  size_t loop() {
    size_t executed = 0;
    for (auto& task : _tasks) {
      if (task->tryRun()) {
        executed++;
      }
    }
    return executed;
  }

  void pause() {
    for (auto& task : _tasks) {
      task->pause();
    }
  }

  void resume() {
    for (auto& task : _tasks) {
      task->resume();
    }
  }

 private:
  const char* _name;
  std::vector<Task*> _tasks;
};

}  // namespace Olon

#endif  // TASK_MANAGER_H
