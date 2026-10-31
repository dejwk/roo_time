// Measure how long an operation takes, and print when it finished.
//
// Uptime identifies an instant on the device's timing clock. Subtracting two
// Uptime values produces a Duration: the amount of time between those instants.
// Use this clock for measurements and timeouts because setting the wall clock
// does not change it. No NTP setup or calendar date is needed here.
//
// A short delay stands in for a sensor read, so this sketch runs unchanged on
// a board or in the emulator. Open Serial at 115200 baud. Each line contains
// an S+ uptime timestamp followed by the elapsed duration of one operation.

#include <Arduino.h>

#include "roo_time.h"
#include "roo_time/format.h"

using namespace roo_time;

void setup() { Serial.begin(115200); }

void loop() {
  // Read the clock immediately before the operation, then again afterward.
  // Replace the 25 ms delay with the work you want to measure.
  const Uptime started = Uptime::Now();
  Delay(Millis(25));  // Stand-in for reading a sensor or processing a message.
  const Uptime finished = Uptime::Now();

  // Keep the result as a Duration: it carries its units through arithmetic,
  // comparisons, and formatting, without extracting a raw microsecond count.
  const Duration elapsed = finished - started;

  // These convenience functions return Arduino Strings for Serial output.
  // Uptime prints as S+HH:MM:SS.ffffff; a duration has no S+ marker and omits
  // trailing fractional zeros by default. Hours can grow beyond 23.
  Serial.print(FormatUptimeArduino(finished));
  Serial.print(" sensor read took ");
  Serial.println(FormatDurationArduino(elapsed));

  // Leave a short pause between measurements; it is outside the measured work.
  Delay(Millis(100));
}
