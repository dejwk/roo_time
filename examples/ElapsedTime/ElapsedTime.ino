#include <Arduino.h>

#include "roo_time.h"

using namespace roo_time;

void setup() {
  Serial.begin(9600);
}

void loop() {
  // Uptime carries microseconds since program start.
  Uptime now = Uptime::Now();

  // Conveniently convert to various time units, as needed.
  Serial.println(now.inMillis());

  // Basic arithmetics and convenience construction.
  now += Hours(2);

  Serial.println(now.inMinutes());

  // Measuring elapsed time.
  Uptime start = Uptime::Now();
  delay(3500);
  Duration elapsed = Uptime::Now() - start;
  if (elapsed > Seconds(2)) {
    Serial.printf("Elapsed: %d ms\n", elapsed.inMillis());
  }

  delay(5000);
}
