Can you help me create a Task Manager library for arduino c++?
It should have 2 classes
1. Task
```c++
  typedef std::function<void(void* params)> TaskFunction;
  typedef std::function<bool()> TaskPredicate;

  Task(const char* name, TaskFunction fn);
  Task(const char* name, bool runOnce, TaskFunction fn);
  const char* getName() const;
  bool getRunOnce*() const;
  bool isEnabled() const;
  // check is the task is temporary paused
  bool isPaused() const;
  bool isRunning() const;
        // change task type.
  // ONCE will start paused, run once, and be paused again.
  // FOREVER will start active and will run at the specified interval.
  // In both cases the enable predicate will be checked to see of the task is enabled when not paused.
  void setRunOnce(bool runOnce);
        // change the enabled state
  void setEnabled(bool enabled);

  // enable the task if the predicate returns true
  void setEnabledWhen(TaskPredicate predicate);

  // change the interval of execution
  void setInterval(uint32_t intervalMillis);
        // callback when the task is done
  void setCallback(TaskDoneCallback doneCallback);

  // pass some data to the task
  void setData(void* params);
  void* getData() const;

  // pause a task
  void pause();

  // resume a paused task
  void resume(uint32_t delayMillis = 0);

  // try to run the task if it should run
  bool tryRun();

  // force the task to run
  void forceRun();

  // request an early run of the task and do not wait for the interval to be reached
  void requestEarlyRun();
```

2. TaskManager
```c++
  explicit TaskManager(const char* name);
  const char* getName() const;
      // Must be called from main loop and will loop over all registered tasks.
      // When using async mode, do not call loop: the async task will call it.
      // Returns the number of executed tasks
      size_t loop();

      void addTask(Task* task);
      void removeTask(Task* task);
      // call pause() on all tasks
      void pause();

      // call resume() on all tasks
      void resume();```

An example on how it should work:

```c++
#include <Arduino.h>
#include <TaskManager.h>

TaskManager taskManager1("tm-1");
TaskManager taskManager2("tm-2");

Task slowdown1("slowdown1", [](void* params) {
  long d = random(1000, 6000);
  Serial.printf("[slowdown1] Delaying for %ld ms\n", d);
  delay(d);
});

Task slowdown2("slowdown2", [](void* params) {
  long d = random(1000, 6000);
  Serial.printf("[slowdown2] Delaying for %ld ms\n", d);
  delay(d);
});

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  slowdown1.setRunOne(false);
  slowdown1.setInterval(3 * TaskDuration::SECONDS);
  taskManager1.addTask(&slowdown1);

  slowdown2.setRunOnce(true);
  taskManager2.addTask(&slowdown2);


}

void loop() {
  taskManager1.loop();
  taskManager2.loop();
}
```