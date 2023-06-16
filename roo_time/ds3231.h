#pragma once

#include <Arduino.h>

#include "DS3231.h"
#include "roo_time.h"

namespace roo_time {

// Clock implementation that uses a DS3231 device as a time source.
class Ds3231Clock : public roo_time::WallTimeClock {
 public:
  Ds3231Clock(TimeZone tz = timezone::UTC,
              Interval max_uptime_trusted = Seconds(10))
      : rtc_(),
        tz_(tz),
        max_uptime_trusted_(max_uptime_trusted),
        last_reading_time_(Uptime::Now() - Hours(1)) {}

  void begin() { rtc_.begin(); }

  // Returns the current time. Reads from the underlying hardware, and caches
  // the result for max_uptime_trusted_, using uptime reading to interpolate.
  // This way, the method can be called very frequently, and the overhead is low
  // - it communicates over I2C only sporadically, to re-sync the clock.
  WallTime now() const override {
    Uptime now = Uptime::Now();
    Interval delta = now - last_reading_time_;
    if (delta < max_uptime_trusted_) {
      // Use delta for approximation, but round to seconds, since DS3231 only
      // has second accuracy.
      return last_reading_ + roo_time::Seconds(delta.inSeconds());
    }
    // NOTE: DS3231 lib returns a Unix timestamp, but it is not actually in UTC
    // (it's 1 hour off). Hence, we don't rely on it.
    auto ds = rtc_.getDateTime();
    last_reading_ = (roo_time::DateTime(ds.year, ds.month, ds.day, ds.hour,
                                        ds.minute, ds.second, 0, tz_))
                        .wallTime();
    last_reading_time_ = now;
    return last_reading_;
  }

  // Sets the clock to the specified wall time. The time will be stored in the
  // clock's timezone (specified during construction).
  void set(WallTime time) {
    DateTime dt(time, tz_);
    rtc_.setDateTime(dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(),
                     dt.second());
    last_reading_ = time;
    last_reading_time_ = Uptime::Now();
  }

 private:
  mutable DS3231 rtc_;
  TimeZone tz_;
  Interval max_uptime_trusted_;
  mutable WallTime last_reading_;
  mutable Uptime last_reading_time_;
};

}  // namespace roo_time