#include <Arduino.h>

#include "roo_time.h"
#include "roo_time/format.h"

using namespace roo_time;

// Script the clock becoming ready. In firmware, inject your synchronized
// SystemClock or RTC, with readiness tracked by the clock's provider.
class MeasurementClock : public WallTimeClock {
 public:
  /// Returns the scripted clock value, initially unavailable.
  WallTime now() const override { return value_; }

  /// Supplies a trusted reading to subsequent measurements.
  void set(WallTime value) { value_ = value; }

 private:
  WallTime value_ = WallTime::Unset();
};

// Produces a timestamp in a caller-owned buffer. A source label keeps uptime
// fallback records distinguishable from absolute UTC timestamps.
bool WriteMeasurement(const WallTimeClock& clock, Uptime uptime, int value) {
  const WallTime now = clock.now();
  char timestamp[40];
  const char* source = now.isSet() ? "utc" : "uptime";
  const FormatResult result =
      now.isSet() ? FormatIsoDateTime(DateTime(now, timezone::UTC), timestamp,
                                      sizeof(timestamp))
                  : FormatUptime(uptime, timestamp, sizeof(timestamp));
  if (result.status != TextStatus::kOk) return false;
  Serial.printf("source=%s timestamp=%s value=%d\n", source, timestamp, value);
  return true;
}

void setup() {
  Serial.begin(115200);
  MeasurementClock clock;
  const Uptime first = Uptime::Start() + Millis(250);
  WriteMeasurement(clock, first, 420);
  clock.set(DateTime(2026, 6, 15, 12, 30, 0, 123456, timezone::UTC).wallTime());
  WriteMeasurement(clock, first + Seconds(1), 421);
}

void loop() { Delay(Seconds(1)); }
