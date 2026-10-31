#include <Arduino.h>

#include "roo_time.h"

using namespace roo_time;

const Duration kPeriod = Millis(100);
Uptime next_sample;
unsigned sample_count = 0;

// Advances to the first deadline strictly after now, preserving the cadence.
int64_t SkipMissedSamples(Uptime now, Uptime& deadline) {
  if (now < deadline) return 0;
  const int64_t skipped = (now - deadline) / kPeriod + 1;
  deadline += kPeriod * skipped;
  return skipped;
}

void setup() {
  Serial.begin(115200);
  next_sample = Uptime::Now() + kPeriod;
  sample_count = 0;
}

void loop() {
  DelayUntil(next_sample);
  next_sample += kPeriod;
  ++sample_count;
  Serial.printf("sample %u\n", sample_count);
  // Every second read overruns a period. Replace with your sensor operation.
  Delay(sample_count % 2 == 0 ? Millis(175) : Millis(25));
  const int64_t skipped = SkipMissedSamples(Uptime::Now(), next_sample);
  if (skipped != 0) {
    Serial.printf("skipped %lld scheduled sample(s)\n",
                  static_cast<long long>(skipped));
  }
}
