// Read a sensor on a steady schedule, even when individual reads take time.
//
// DelayUntil waits for an Uptime deadline. Advancing that deadline by the same
// Duration each time preserves the cadence; waiting for a period after each
// read would add the work time to every cycle and gradually drift.
//
// This example deliberately makes every second read slow. Its policy is to
// skip missed readings and resume at the next future deadline, rather than
// perform a burst of catch-up work. The simulated reads need no hardware and
// run in the emulator. Serial at 115200 baud shows samples and skipped slots.

#include <Arduino.h>

#include "roo_time.h"

using namespace roo_time;

// Express the sampling interval with units. The deadline is a separate value
// because it describes when to run, rather than how long to wait.
const Duration kPeriod = Millis(100);
Uptime next_sample;
unsigned sample_count = 0;

// Advances to the first deadline strictly after now, preserving the cadence.
int64_t SkipMissedSamples(Uptime now, Uptime& deadline) {
  if (now < deadline) return 0;

  // Subtracting uptimes gives a Duration; dividing by another Duration gives
  // a count of whole periods. Include the slot at deadline itself, since the
  // next read must start in the future, even if now equals that deadline.
  const int64_t skipped = (now - deadline) / kPeriod + 1;
  deadline += kPeriod * skipped;
  return skipped;
}

void setup() {
  Serial.begin(115200);

  // Choose the first deadline relative to the current clock. Later deadlines
  // will advance from this schedule, rather than from each read's finish time.
  next_sample = Uptime::Now() + kPeriod;
  sample_count = 0;
}

void loop() {
  // Wait for the scheduled instant. A deadline that has already passed makes
  // DelayUntil return immediately. Scheduling can also make a wakeup late.
  DelayUntil(next_sample);

  // Reserve the following slot before doing work, keeping the original cadence.
  next_sample += kPeriod;
  ++sample_count;
  Serial.printf("sample %u\n", sample_count);

  // Every second read overruns a period. Replace with your sensor operation.
  Delay(sample_count % 2 == 0 ? Millis(175) : Millis(25));

  // If work used up the next slot, move past it. A real sensor application
  // should choose its own overrun policy; this one deliberately drops samples.
  const int64_t skipped = SkipMissedSamples(Uptime::Now(), next_sample);
  if (skipped != 0) {
    Serial.printf("skipped %lld scheduled sample(s)\n",
                  static_cast<long long>(skipped));
  }
}
