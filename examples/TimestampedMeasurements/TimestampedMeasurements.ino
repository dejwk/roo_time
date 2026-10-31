// Timestamp sensor records before and after a wall clock becomes ready.
//
// A WallTime is an absolute UTC instant; an Uptime belongs to this device's
// timing clock. Until a trusted calendar clock is available, record uptime as
// a clearly labeled fallback rather than pretending it is a date and time.
//
// This example also shows allocation-free formatting into a caller-owned
// buffer and checking its result before printing a record. A scripted clock
// makes both cases repeatable on a board or in the emulator. Serial at 115200
// baud shows value 420 with uptime, followed by value 421 with a UTC timestamp.

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
  // Unset is an explicit sentinel, distinct from a valid reading at the epoch.
  WallTime value_ = WallTime::Unset();
};

// Produces a timestamp in a caller-owned buffer. A source label keeps uptime
// fallback records distinguishable from absolute UTC timestamps.
bool WriteMeasurement(const WallTimeClock& clock, Uptime uptime, int value) {
  // Read once so every decision for this record uses the same snapshot.
  // This example's clock supplies trusted values within calendar years 1-9999.
  const WallTime now = clock.now();

  // Enough space for either an ISO timestamp or this example's uptime text,
  // including the terminating NUL. The formatter reports insufficient space.
  char timestamp[40];

  // isSet() detects the unset sentinel. A real clock provider must separately
  // establish readiness: a readable system clock can still be unsynchronized.
  const char* source = now.isSet() ? "utc" : "uptime";

  // Use ISO date/time text for UTC, or the S+ uptime layout for the fallback.
  // These buffer overloads write into timestamp without allocating a String.
  const FormatResult result =
      now.isSet() ? FormatIsoDateTime(DateTime(now, timezone::UTC), timestamp,
                                      sizeof(timestamp))
                  : FormatUptime(uptime, timestamp, sizeof(timestamp));

  // Never emit a partial timestamp as if it were a complete sensor record.
  if (result.status != TextStatus::kOk) return false;
  Serial.printf("source=%s timestamp=%s value=%d\n", source, timestamp, value);
  return true;
}

void setup() {
  Serial.begin(115200);
  MeasurementClock clock;
  const Uptime first = Uptime::Start() + Millis(250);

  // The clock starts unavailable, so this first reading uses uptime.
  WriteMeasurement(clock, first, 420);

  // Simulate receiving a trusted UTC reading from NTP or an initialized RTC.
  // DateTime holds the supplied calendar fields; wallTime() extracts the
  // instant.
  clock.set(DateTime(2026, 6, 15, 12, 30, 0, 123456, timezone::UTC).wallTime());

  // The next record uses calendar time. Its source label makes the change of
  // timestamp meaning explicit to anyone reading or processing these records.
  WriteMeasurement(clock, first + Seconds(1), 421);
}

void loop() {
  // Both clock states were demonstrated once in setup().
  Delay(Seconds(1));
}
