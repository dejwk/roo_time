#pragma once

#include "roo_time/civil_day.h"
#include "roo_time/duration.h"

#if defined(ESP_PLATFORM) || defined(__linux__)
#define CTIME_HDR_DEFINED
#include <sys/time.h>

#include <ctime>
#endif

namespace roo_time {
/// Represents absolute wall time since Unix epoch.
///
/// Stored with microsecond precision; INT64_MIN is reserved for invalid time.
/// Arithmetic requires valid operands and representable, valid results.
/// Does not account for leap seconds.
class WallTime {
 public:
  /// Constructs an 'unset' wall time.
  static constexpr WallTime Unset() {
    return WallTime(Duration::Min(), InitTag{});
  }

  /// Constructs the WallTime corresponding to the Unix Epoch (1970-01-01 00:00
  /// UTC).
  static constexpr WallTime Epoch() { return WallTime(Duration(), InitTag{}); }

  /// Constructs the WallTime corresponding to the specified duration since Unix
  /// Epoch (1970-01-01 00:00 UTC).
  static constexpr WallTime SinceEpoch(Duration since_epoch) {
    return WallTime(since_epoch, InitTag{});
  }

  /// Constructs wall time at the Unix epoch.
  [[deprecated("Use WallTime::Epoch() instead")]]
  constexpr WallTime()
      : since_epoch_(Duration()) {}

  /// Constructs wall time from offset since Unix epoch.
  [[deprecated("Use WallTime::SinceEpoch() instead")]]
  explicit constexpr WallTime(Duration since_epoch)
      : since_epoch_(since_epoch) {}

  /// Returns true if this wall time is valid (not the unset value).
  [[nodiscard]] constexpr bool isSet() const {
    return since_epoch_ != Duration::Min();
  }

  /// Returns elapsed duration since Unix epoch.
  [[nodiscard]] constexpr Duration sinceEpoch() const { return since_epoch_; }

  /// Adds duration to this wall time. The wall time must not be unset.
  WallTime& operator+=(const Duration& i) {
    assert(isSet());
    since_epoch_ += i;
    return *this;
  }

  /// Subtracts duration from this wall time. The wall time must not be unset.
  WallTime& operator-=(const Duration& i) {
    assert(isSet());
    since_epoch_ -= i;
    return *this;
  }

 private:
  friend WallTime operator+(const WallTime&, const Duration&);
  friend WallTime operator-(const WallTime&, const Duration&);
  friend WallTime operator+(const Duration&, const WallTime&);

  struct InitTag {};
  constexpr WallTime(Duration since_epoch, InitTag)
      : since_epoch_(since_epoch) {}

  Duration since_epoch_;
};

/// Returns true if both wall times are equal.
inline bool operator==(const WallTime& a, const WallTime& b) {
  return a.sinceEpoch() == b.sinceEpoch();
}

/// Returns true if wall times differ.
inline bool operator!=(const WallTime& a, const WallTime& b) {
  return a.sinceEpoch() != b.sinceEpoch();
}

/// Returns true if `a` is earlier than `b`.
inline bool operator<(const WallTime& a, const WallTime& b) {
  return a.sinceEpoch() < b.sinceEpoch();
}

/// Returns true if `a` is later than `b`.
inline bool operator>(const WallTime& a, const WallTime& b) {
  return a.sinceEpoch() > b.sinceEpoch();
}

/// Returns true if `a` is not later than `b`.
inline bool operator<=(const WallTime& a, const WallTime& b) {
  return a.sinceEpoch() <= b.sinceEpoch();
}

/// Returns true if `a` is not earlier than `b`.
inline bool operator>=(const WallTime& a, const WallTime& b) {
  return a.sinceEpoch() >= b.sinceEpoch();
}

/// Returns elapsed duration between two wall times.
inline Duration operator-(const WallTime& a, const WallTime& b) {
  assert(a.isSet() && b.isSet());
  return a.sinceEpoch() - b.sinceEpoch();
}

/// Returns wall time shifted by duration.
inline WallTime operator+(const WallTime& t, const Duration& i) {
  assert(t.isSet());
  return WallTime::SinceEpoch(t.sinceEpoch() + i);
}

/// Returns wall time shifted backwards by duration.
inline WallTime operator-(const WallTime& t, const Duration& i) {
  assert(t.isSet());
  return WallTime::SinceEpoch(t.sinceEpoch() - i);
}

/// Returns wall time shifted by duration.
inline WallTime operator+(const Duration& i, const WallTime& t) {
  assert(t.isSet());
  return WallTime::SinceEpoch(t.sinceEpoch() + i);
}

/// Abstract interface for obtaining current wall time.
/// Check isSet() on returned values; synchronization status is tracked by the
/// caller. Wall time may jump; use Uptime for elapsed measurement and
/// deadlines.
class WallTimeClock {
 public:
  /// Virtual destructor.
  virtual ~WallTimeClock() = default;

  /// Returns current wall time. Returns an unset wall time on read error.
  virtual WallTime now() const = 0;
};

#ifdef CTIME_HDR_DEFINED
/// Wall-time clock backed by `gettimeofday`; does not initiate synchronization.
/// Returns unset wall time when gettimeofday fails.
class SystemClock : public WallTimeClock {
 public:
  /// Returns current system wall time.
  WallTime now() const override {
    struct timeval tv;
    if (gettimeofday(&tv, nullptr)) return WallTime::Unset();
    return WallTime::SinceEpoch(Micros(tv.tv_sec * 1000000LL + tv.tv_usec));
  }
};
#endif

/// Fixed UTC offset in whole signed 16-bit minutes, without DST or zone rules.
/// Supply a representable whole-minute offset; construction is unchecked and
/// truncates sub-minute input toward zero.
class UtcOffset {
 public:
  /// Constructs a zero UTC offset.
  constexpr UtcOffset() : offset_minutes_(0) {}

  /// Constructs a fixed UTC offset.
  constexpr explicit UtcOffset(Duration offset)
      : offset_minutes_(offset.inMinutes()) {}

  /// Returns this fixed offset as a duration.
  [[nodiscard]] [[deprecated("Use asDuration() instead")]]
  constexpr Duration offset() const {
    return asDuration();
  }

  /// Returns this fixed offset as a duration.
  [[nodiscard]] constexpr Duration asDuration() const {
    return Minutes(offset_minutes_);
  }

  /// Returns the offset value in microseconds.
  [[nodiscard]] constexpr int64_t inMicros() const {
    return offset_minutes_ * 60000000LL;
  }

  /// Returns the offset value in milliseconds.
  [[nodiscard]] constexpr int32_t inMillis() const {
    return static_cast<int32_t>(offset_minutes_) * 60000L;
  }

  /// Returns the offset value in seconds.
  [[nodiscard]] constexpr int32_t inSeconds() const {
    return static_cast<int32_t>(offset_minutes_) * 60;
  }

  /// Returns the offset value in minutes.
  [[nodiscard]] constexpr int16_t inMinutes() const { return offset_minutes_; }

 private:
  int16_t offset_minutes_;
};

/// Returns true if both offsets are equal.
inline constexpr bool operator==(UtcOffset a, UtcOffset b) {
  return a.inMinutes() == b.inMinutes();
}

/// Returns true if the offsets differ.
inline constexpr bool operator!=(UtcOffset a, UtcOffset b) {
  return a.inMinutes() != b.inMinutes();
}

/// Returns true if the first offset is smaller.
inline constexpr bool operator<(UtcOffset a, UtcOffset b) {
  return a.inMinutes() < b.inMinutes();
}

/// Returns true if the first offset is smaller or equal.
inline constexpr bool operator<=(UtcOffset a, UtcOffset b) {
  return a.inMinutes() <= b.inMinutes();
}

/// Returns true if the first offset is larger.
inline constexpr bool operator>(UtcOffset a, UtcOffset b) {
  return a.inMinutes() > b.inMinutes();
}

/// Returns true if the first offset is larger or equal.
inline constexpr bool operator>=(UtcOffset a, UtcOffset b) {
  return a.inMinutes() >= b.inMinutes();
}

namespace timezone {
constexpr UtcOffset UTC = UtcOffset(Micros(0));
}

/// Represents wall time decomposed into date/time in a specific time zone.
///
/// Supports valid Gregorian dates in years 1-9999; inputs are not validated or
/// normalized. Wall-time conversion must also yield a local date in that range.
/// Does not account for leap seconds.
class DateTime {
 public:
  /// Constructs `DateTime` representing the Unix epoch in UTC.
  DateTime() : DateTime(WallTime::Epoch(), timezone::UTC) {}

  /// Constructs `DateTime` at midnight of a date in the specified time zone.
  ///
  /// @param year Four-digit year.
  /// @param month Month in [1, 12].
  /// @param day Day in [1, max_day_of_month].
  DateTime(uint16_t year, uint8_t month, uint8_t day, UtcOffset tz);

  /// Constructs date/time in the specified time zone.
  ///
  /// @param year Four-digit year.
  /// @param month Month in [1, 12].
  /// @param day Day in [1, max_day_of_month].
  /// @param hour Hour in [0, 23].
  /// @param minute Minute in [0, 59].
  /// @param second Second in [0, 59].
  /// @param micros Microsecond fraction in [0, 999999].
  /// @param tz Time zone to interpret the components in.
  DateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour,
           uint8_t minute, uint8_t second, uint32_t micros, UtcOffset tz);

  /// Constructs `DateTime` for `wallTime` in time zone `tz`. The `wallTime`
  /// must be set.
  DateTime(WallTime wallTime, UtcOffset tz);

  /// Returns `WallTime` corresponding to this `DateTime`.
  [[nodiscard]] WallTime wallTime() const { return walltime_; }

  /// Returns the fixed UTC offset of this `DateTime`.
  [[nodiscard]] [[deprecated("Use utcOffset() instead")]] UtcOffset timeZone()
      const {
    return offset_;
  }

  /// Returns the fixed UTC offset of this `DateTime`.
  [[nodiscard]] UtcOffset utcOffset() const { return offset_; }

  /// Returns four-digit year.
  [[nodiscard]] int16_t year() const { return year_; }

  /// Returns month in [1, 12].
  [[nodiscard]] Month month() const { return (Month)month_; }

  /// Returns day of month in valid range.
  [[nodiscard]] uint8_t day() const { return day_; }

  /// Returns the local civil date, discarding time of day and UTC offset.
  [[nodiscard]] CivilDay civilDay() const {
    return CivilDay::FromYmd(year_, month_, day_);
  }

  /// Returns hour in [0, 23].
  [[nodiscard]] uint8_t hour() const { return hour_; }

  /// Returns minute in [0, 59].
  [[nodiscard]] uint8_t minute() const { return minute_; }

  /// Returns second in [0, 59].
  [[nodiscard]] uint8_t second() const { return second_; }

  /// Returns microsecond fraction in [0, 999999].
  [[nodiscard]] uint32_t micros() const { return micros_; }

  /// Returns day of week in this time zone.
  [[nodiscard]] DayOfWeek dayOfWeek() const { return day_of_week_; }

  /// Returns day of year in [1, 366].
  [[nodiscard]] uint16_t dayOfYear() const { return day_of_year_; }

#ifdef CTIME_HDR_DEFINED
  /// Constructs from valid C `tm` calendar fields in the explicit fixed offset.
  /// Does not interpret tm_isdst, tm_wday, or tm_yday.
  DateTime(struct tm t, UtcOffset tz = timezone::UTC)
      : DateTime(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min,
                 t.tm_sec, 0, tz) {}

  /// Returns local calendar fields with zero-based tm_yday and tm_isdst = -1.
  /// The fixed UTC offset is not carried into the C structure.
  struct tm tmStruct() const {
    return tm{.tm_sec = second_,
              .tm_min = minute_,
              .tm_hour = hour_,
              .tm_mday = day_,
              .tm_mon = month_ - 1,
              .tm_year = year_ - 1900,
              .tm_wday = day_of_week_,
              .tm_yday = day_of_year_ - 1,
              .tm_isdst = -1};
  }
#endif

 private:
  WallTime walltime_ = WallTime::Unset();
  UtcOffset offset_;
  int16_t year_;
  uint8_t month_;
  uint8_t day_;
  uint8_t hour_;
  uint8_t minute_;
  uint8_t second_;
  DayOfWeek day_of_week_;
  uint16_t day_of_year_;
  uint32_t micros_;
};

/// Returns true if both date-times represent the same instant and offset.
inline bool operator==(const DateTime& a, const DateTime& b) {
  return a.wallTime() == b.wallTime() && a.utcOffset() == b.utcOffset();
}

/// Returns true if date-times differ in instant or time-zone offset.
inline bool operator!=(const DateTime& a, const DateTime& b) {
  return a.wallTime() != b.wallTime() || a.utcOffset() != b.utcOffset();
}

}  // namespace roo_time
