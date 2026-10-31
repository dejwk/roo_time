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
  Serial.print(FormatUptimeArduino(finished));
  Serial.print(" sensor read took ");
  Serial.println(FormatDurationArduino(elapsed));
  Delay(Millis(100));
}
