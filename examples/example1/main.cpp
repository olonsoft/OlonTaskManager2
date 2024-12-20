#include <Arduino.h>
#include "OlonTaskManager.h"

// Olon::TaskManager taskManager1("tm-1");

// Olon::Task slowdown1("slowdown1", [](void* params) {
//   long d = random(1000, 6000);
//   Serial.printf("[slowdown1] Delaying for %ld ms\n", d);
//   delay(d);
// });

// void setup() {
//   Serial.begin(115200);
//   while (!Serial) continue;

//   slowdown1.setRunOnce(false);
//   // slowdown1.setInterval(1000);  // 3 seconds in milliseconds

//   slowdown1.setCallback([](const Olon::Task& me, const uint32_t elapsed) {
//     Serial.printf("Task '%s' executed in %u ms\n", me.getName(), elapsed);
//   });
//   taskManager1.addTask(&slowdown1);

// }

// void loop() {
//   taskManager1.loop();
// }

// =================

Olon::TaskManager loopTaskManager("loop()");

Olon::Task sayHello("sayHello", [](void* params) { Serial.printf("%lu Hello\t", millis()); });
Olon::Task sayGoodbye("sayGoodbye", [](void* params) { Serial.printf("%lu Goodbye\t", millis()); });
Olon::Task ping("ping", Olon::Task::RunOnce, [](void* params) { Serial.printf("%lu ping\t ", millis()); });
Olon::Task output("output", [](void* params) { Serial.printf("%lu output\n", millis()); });
Olon::Task delayed("delayed", Olon::Task::RunOnce, [](void* params) { Serial.printf("%lu Delayed!\n", millis()); });

char* params = "Pong";

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;
  Serial.println();

  // sayHello.setRunOnce(false); // default
  sayHello.setInterval(1 * Olon::TaskDuration::SECONDS);
  sayHello.setDoneCallback([](const Olon::Task& me, const uint32_t elapsed) {
    Serial.printf("Task '%s' executed in %" PRIu32 " us\n", me.getName(), elapsed);
  });
  loopTaskManager.addTask(&sayHello);

  // sayGoodbye.setRunOnce(false); // default
  sayGoodbye.setInterval(3 * Olon::TaskDuration::SECONDS);
  sayGoodbye.setDoneCallback([](const Olon::Task& me, const uint32_t elapsed) {
    Serial.printf("Task '%s' executed in %" PRIu32 " us\n", me.getName(), elapsed);
    ping.setData(params);
    ping.resume();
  });
  loopTaskManager.addTask(&sayGoodbye);

  // ping.setRunOnce(true);
  // ping.pause(); // do not auto start. Need to call resume.
  // or auto start after 1500 ms
  // ping.setInterval(1500 * Olon::TaskDuration::MILLISECONDS);
  ping.setDoneCallback([](const Olon::Task& me, const uint32_t elapsed) {
    Serial.printf("Task '%s' executed in %" PRIu32 " us\n", me.getName(), elapsed);
  });
  loopTaskManager.addTask(&ping);

  // output.setRunOnce(false); // deeault
  output.setInterval(5 * Olon::TaskDuration::SECONDS);
  loopTaskManager.addTask(&output);

  // delayed.setRunOnce(true);
  delayed.resume(10 * Olon::TaskDuration::SECONDS);
  loopTaskManager.addTask(&delayed);
}

void loop() {
  loopTaskManager.loop();
}