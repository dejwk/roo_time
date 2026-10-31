#include <Arduino.h>

#include "roo_time.h"
#include "roo_time/format.h"

using namespace roo_time;

void setup() { Serial.begin(115200); }

void loop() {
  const Uptime started = Uptime::Now();
  Delay(Millis(25));  // Stand-in for reading a sensor or processing a message.
  const Uptime finished = Uptime::Now();
  const Duration elapsed = finished - started;
  char timestamp[40];
  char duration[40];
  if (FormatUptime(finished, timestamp, sizeof(timestamp)).status ==
          TextStatus::kOk &&
      FormatDuration(elapsed, duration, sizeof(duration)).status ==
          TextStatus::kOk) {
    Serial.printf("%s sensor read took %s\n", timestamp, duration);
  }
  Delay(Millis(100));
}
