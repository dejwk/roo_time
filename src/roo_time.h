#pragma once

/// Umbrella header for the roo_time module.
///
/// Provides duration, uptime, and wall-time abstractions.

#include <inttypes.h>
#if defined(ESP_PLATFORM) || defined(__linux__)
#define CTIME_HDR_DEFINED
#include <sys/time.h>

#include <ctime>
#endif

/// Convenience classes for handling delays and elapsed time measurement.
///
/// Helps avoid common mistakes such as mixing time units or confusing
/// timestamps with durations.
namespace roo_time {

/// Represents an amount of time (e.g. 5s, 10min).
///
/// Stored with microsecond precision and 64-bit range. Pass by value.
/// For rounding semantics, see README section "Rounding semantics".
class Duration {
 public:
  /// Calendar-like decomposition of a duration value.
  struct Components {
    bool negative : 1;
    uint64_t days : 26;
    uint8_t hours : 5;
    uint8_t minutes : 6;
    uint8_t seconds : 6;
    uint32_t micros : 20;
  };

  /// Constructs zero duration.
  constexpr Duration() : micros_(0) {}

  /// Returns the maximum representable duration.
  static const Duration Max() { return Duration(0x7FFFFFFFFFFFFFFF); }

  /// Returns duration in microseconds.
  [[nodiscard]] constexpr int64_t inMicros() const { return micros_; }

  /// Returns duration in milliseconds, rounded toward zero.
  [[nodiscard]] constexpr int64_t inMillis() const {
    return inMillisRoundedDown();
  }

  /// Returns duration in seconds, rounded toward zero.
  [[nodiscard]] constexpr int64_t inSeconds() const {
    return inSecondsRoundedDown();
  }

  /// Returns duration in minutes, rounded toward zero.
  [[nodiscard]] constexpr int64_t inMinutes() const {
    return inMinutesRoundedDown();
  }

  /// Returns duration in hours, rounded toward zero.
  [[nodiscard]] constexpr int64_t inHours() const { return inHoursRoundedDown(); }

  /// Returns duration in milliseconds, rounded toward zero.
  [[nodiscard]] constexpr int64_t inMillisRoundedDown() const {
    return micros_ / 1000LL;
  }

  /// Returns duration in seconds, rounded toward zero.
  [[nodiscard]] constexpr int64_t inSecondsRoundedDown() const {
    return micros_ / 1000000LL;
  }

  /// Returns duration in minutes, rounded toward zero.
  [[nodiscard]] constexpr int64_t inMinutesRoundedDown() const {
    return micros_ / 60000000LL;
  }

  /// Returns duration in hours, rounded toward zero.
  [[nodiscard]] constexpr int64_t inHoursRoundedDown() const {
    return micros_ / 3600000000LL;
  }

  /// Returns duration in milliseconds, rounded away from zero.
  [[nodiscard]] constexpr int64_t inMillisRoundedUp() const {
    int64_t q = micros_ / 1000LL;
    int64_t r = micros_ % 1000LL;
    if (r == 0) return q;
    return micros_ > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in seconds, rounded away from zero.
  [[nodiscard]] constexpr int64_t inSecondsRoundedUp() const {
    int64_t q = micros_ / 1000000LL;
    int64_t r = micros_ % 1000000LL;
    if (r == 0) return q;
    return micros_ > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in minutes, rounded away from zero.
  [[nodiscard]] constexpr int64_t inMinutesRoundedUp() const {
    int64_t q = micros_ / 60000000LL;
    int64_t r = micros_ % 60000000LL;
    if (r == 0) return q;
    return micros_ > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in hours, rounded away from zero.
  [[nodiscard]] constexpr int64_t inHoursRoundedUp() const {
    int64_t q = micros_ / 3600000000LL;
    int64_t r = micros_ % 3600000000LL;
    if (r == 0) return q;
    return micros_ > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in milliseconds, rounded to nearest (ties away from
  /// zero).
  [[nodiscard]] constexpr int64_t inMillisRoundedNearest() const {
    int64_t q = micros_ / 1000LL;
    int64_t r = micros_ % 1000LL;
    int64_t ar = r < 0 ? -r : r;
    if (ar * 2 < 1000LL) return q;
    return micros_ > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in seconds, rounded to nearest (ties away from zero).
  [[nodiscard]] constexpr int64_t inSecondsRoundedNearest() const {
    int64_t q = micros_ / 1000000LL;
    int64_t r = micros_ % 1000000LL;
    int64_t ar = r < 0 ? -r : r;
    if (ar * 2 < 1000000LL) return q;
    return micros_ > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in minutes, rounded to nearest (ties away from zero).
  [[nodiscard]] constexpr int64_t inMinutesRoundedNearest() const {
    int64_t q = micros_ / 60000000LL;
    int64_t r = micros_ % 60000000LL;
    int64_t ar = r < 0 ? -r : r;
    if (ar * 2 < 60000000LL) return q;
    return micros_ > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in hours, rounded to nearest (ties away from zero).
  [[nodiscard]] constexpr int64_t inHoursRoundedNearest() const {
    int64_t q = micros_ / 3600000000LL;
    int64_t r = micros_ % 3600000000LL;
    int64_t ar = r < 0 ? -r : r;
    if (ar * 2 < 3600000000LL) return q;
    return micros_ > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in milliseconds as floating-point value.
  [[nodiscard]] constexpr float inMillisFloat() const { return micros_ / 1000.0; }

  /// Returns duration in seconds as floating-point value.
  [[nodiscard]] constexpr float inSecondsFloat() const {
    return micros_ / 1000000.0;
  }

  /// Returns duration in minutes as floating-point value.
  [[nodiscard]] constexpr float inMinutesFloat() const {
    return micros_ / 60000000.0;
  }

  /// Returns duration in hours as floating-point value.
  [[nodiscard]] constexpr float inHoursFloat() const {
    return micros_ / 3600000000.0;
  }

  /// Adds another duration to this one.
  Duration& operator+=(const Duration& other) {
    micros_ += other.inMicros();
    return *this;
  }

  /// Subtracts another duration from this one.
  Duration& operator-=(const Duration& other) {
    micros_ -= other.inMicros();
    return *this;
  }

  /// Breaks duration into components (days, hours, minutes, ...).
  Components toComponents();

  /// Reconstructs duration from components.
  static Duration FromComponents(const Components& components);

 private:
  friend constexpr Duration Micros(long long micros);

  friend constexpr Duration Millis(long long millis);
  friend constexpr Duration Seconds(long long seconds);
  friend constexpr Duration Minutes(long long minutes);
  friend constexpr Duration Hours(long long hours);

  friend constexpr Duration Millis(float millis);
  friend constexpr Duration Seconds(float seconds);
  friend constexpr Duration Minutes(float minutes);
  friend constexpr Duration Hours(float hours);

  friend constexpr Duration Millis(double millis);
  friend constexpr Duration Seconds(double seconds);
  friend constexpr Duration Minutes(double minutes);
  friend constexpr Duration Hours(double hours);

  constexpr Duration(int64_t micros) : micros_(micros) {}

  int64_t micros_;
};

/// Backwards compatibility alias. Prefer `Duration` in new code.
using Interval = Duration;

/// Constructs a duration from microseconds.
inline constexpr Duration Micros(long long micros) { return Duration(micros); }

/// Constructs a duration from milliseconds.
///
/// Overloads accept integer and floating-point input types.
inline constexpr Duration Millis(long long millis) {
  return Micros(millis * 1000);
}

inline constexpr Duration Millis(unsigned long long millis) {
  return Millis((long long)millis);
}

inline constexpr Duration Millis(long millis) {
  return Millis((long long)millis);
}

inline constexpr Duration Millis(unsigned long millis) {
  return Millis((long long)millis);
}

inline constexpr Duration Millis(int millis) {
  return Millis((long long)millis);
}

inline constexpr Duration Millis(unsigned int millis) {
  return Millis((long long)millis);
}

inline constexpr Duration Millis(short millis) {
  return Millis((long long)millis);
}

inline constexpr Duration Millis(unsigned short millis) {
  return Millis((long long)millis);
}

inline constexpr Duration Millis(float millis) {
  return Duration((long long)(millis * 1000));
}

inline constexpr Duration Millis(double millis) {
  return Duration((long long)(millis * 1000));
}

/// Constructs a duration from seconds.
///
/// Overloads accept integer and floating-point input types.
inline constexpr Duration Seconds(long long seconds) {
  return Micros(seconds * 1000 * 1000);
}

inline constexpr Duration Seconds(unsigned long long seconds) {
  return Seconds((long long)seconds);
}

inline constexpr Duration Seconds(long seconds) {
  return Seconds((long long)seconds);
}

inline constexpr Duration Seconds(unsigned long seconds) {
  return Seconds((long long)seconds);
}

inline constexpr Duration Seconds(int seconds) {
  return Seconds((long long)seconds);
}

inline constexpr Duration Seconds(unsigned int seconds) {
  return Seconds((long long)seconds);
}

inline constexpr Duration Seconds(short seconds) {
  return Seconds((long long)seconds);
}

inline constexpr Duration Seconds(unsigned short seconds) {
  return Seconds((long long)seconds);
}

inline constexpr Duration Seconds(float seconds) {
  return Duration((int64_t)(seconds * 1000 * 1000));
}

inline constexpr Duration Seconds(double seconds) {
  return Duration((int64_t)(seconds * 1000 * 1000));
}

/// Constructs a duration from minutes.
///
/// Overloads accept integer and floating-point input types.
inline constexpr Duration Minutes(long long minutes) {
  return Duration(minutes * 1000 * 1000 * 60);
}

inline constexpr Duration Minutes(unsigned long long minutes) {
  return Minutes((long long)minutes);
}

inline constexpr Duration Minutes(long minutes) {
  return Minutes((long long)minutes);
}

inline constexpr Duration Minutes(unsigned long minutes) {
  return Minutes((long long)minutes);
}

inline constexpr Duration Minutes(int minutes) {
  return Minutes((long long)minutes);
}

inline constexpr Duration Minutes(unsigned int minutes) {
  return Minutes((long long)minutes);
}

inline constexpr Duration Minutes(short minutes) {
  return Minutes((long long)minutes);
}

inline constexpr Duration Minutes(unsigned short minutes) {
  return Minutes((long long)minutes);
}

inline constexpr Duration Minutes(float minutes) {
  return Duration((int64_t)(minutes * 1000 * 1000 * 60));
}

inline constexpr Duration Minutes(double minutes) {
  return Duration((int64_t)(minutes * 1000 * 1000 * 60));
}

/// Constructs a duration from hours.
///
/// Overloads accept integer and floating-point input types.
inline constexpr Duration Hours(long long hours) {
  return Duration(hours * 1000 * 1000 * 60 * 60);
}

inline constexpr Duration Hours(unsigned long long minutes) {
  return Hours((long long)minutes);
}

inline constexpr Duration Hours(long minutes) {
  return Hours((long long)minutes);
}

inline constexpr Duration Hours(unsigned long minutes) {
  return Hours((long long)minutes);
}

inline constexpr Duration Hours(int minutes) {
  return Hours((long long)minutes);
}

inline constexpr Duration Hours(unsigned int minutes) {
  return Hours((long long)minutes);
}

inline constexpr Duration Hours(short minutes) {
  return Hours((long long)minutes);
}

inline constexpr Duration Hours(unsigned short minutes) {
  return Hours((long long)minutes);
}

inline constexpr Duration Hours(float hours) {
  return Duration((int64_t)(hours * 1000 * 1000 * 60 * 60));
}

inline constexpr Duration Hours(double hours) {
  return Duration((int64_t)(hours * 1000 * 1000 * 60 * 60));
}

/// Returns true if both durations are equal.
inline bool operator==(const Duration& a, const Duration& b) {
  return a.inMicros() == b.inMicros();
}

/// Returns true if durations differ.
inline bool operator!=(const Duration& a, const Duration& b) {
  return a.inMicros() != b.inMicros();
}

/// Returns true if `a` is shorter than `b`.
inline bool operator<(const Duration& a, const Duration& b) {
  return a.inMicros() < b.inMicros();
}

/// Returns true if `a` is longer than `b`.
inline bool operator>(const Duration& a, const Duration& b) {
  return a.inMicros() > b.inMicros();
}

/// Returns true if `a` is not longer than `b`.
inline bool operator<=(const Duration& a, const Duration& b) {
  return a.inMicros() <= b.inMicros();
}

/// Returns true if `a` is not shorter than `b`.
inline bool operator>=(const Duration& a, const Duration& b) {
  return a.inMicros() >= b.inMicros();
}

/// Returns the sum of two durations.
inline Duration operator+(const Duration& a, const Duration& b) {
  return Micros(a.inMicros() + b.inMicros());
}

/// Returns the difference between two durations.
inline Duration operator-(const Duration& a, const Duration& b) {
  return Micros(a.inMicros() - b.inMicros());
}

/// Multiplies duration by an integer factor.
inline Duration operator*(const Duration& a, int b) {
  return Micros(a.inMicros() * b);
}

/// Multiplies duration by an integer factor.
inline Duration operator*(int a, const Duration& b) {
  return Micros(a * b.inMicros());
}

/// Represents an instant relative to process/boot start time.
///
/// Stored with microsecond precision and 64-bit range. May not include sleep
/// time on some platforms.
class Uptime {
 public:
  /// Returns current monotonic process uptime.
  static const Uptime Now();

  /// Returns uptime value at process start.
  static const Uptime Start() { return Uptime(0); }

  /// Returns the maximum representable uptime value.
  static const Uptime Max() { return Uptime(0x7FFFFFFFFFFFFFFF); }

  /// Constructs zero uptime value.
  Uptime() : micros_(0) {}

  /// Copy constructor.
  Uptime(const Uptime& other) : micros_(other.micros_) {}

  /// Copy constructor for volatile sources.
  Uptime(const volatile Uptime& other) : micros_(other.micros_) {}

  /// Assignment operator.
  Uptime& operator=(const Uptime& other) {
    micros_ = other.micros_;
    return *this;
  }

  /// Assignment operator for volatile sources.
  Uptime& operator=(const volatile Uptime& other) {
    micros_ = other.micros_;
    return *this;
  }

  /// Returns uptime in microseconds.
  [[nodiscard]] int64_t inMicros() const { return micros_; }

  /// Returns uptime in milliseconds.
  [[nodiscard]] int64_t inMillis() const { return micros_ / 1000LL; }

  /// Returns uptime in seconds.
  [[nodiscard]] int64_t inSeconds() const { return micros_ / 1000000LL; }

  /// Returns uptime in minutes.
  [[nodiscard]] int64_t inMinutes() const { return micros_ / 60000000LL; }

  /// Returns uptime in hours.
  [[nodiscard]] int64_t inHours() const { return micros_ / 3600000000LL; }

  // Duration HowLongAgo() const {
  //   return Duration(Now().ToMicros() - this->ToMicros);
  // }

  /// Adds duration to this uptime.
  Uptime& operator+=(const Duration& i) {
    micros_ += i.inMicros();
    return *this;
  }

  /// Subtracts duration from this uptime.
  Uptime& operator-=(const Duration& i) {
    micros_ -= i.inMicros();
    return *this;
  }

 private:
  friend Uptime operator+(const Uptime& u, const Duration& i);
  friend Uptime operator-(const Uptime& u, const Duration& i);
  friend Uptime operator+(const Duration& i, const Uptime& u);

  Uptime(int64_t micros) : micros_(micros) {}

  int64_t micros_;
};

/// Returns true if uptimes are equal.
inline bool operator==(const Uptime& a, const Uptime& b) {
  return a.inMicros() == b.inMicros();
}

/// Returns true if uptimes differ.
inline bool operator!=(const Uptime& a, const Uptime& b) {
  return a.inMicros() != b.inMicros();
}

/// Returns true if `a` is earlier than `b`.
inline bool operator<(const Uptime& a, const Uptime& b) {
  return a.inMicros() < b.inMicros();
}

/// Returns true if `a` is later than `b`.
inline bool operator>(const Uptime& a, const Uptime& b) {
  return a.inMicros() > b.inMicros();
}

/// Returns true if `a` is not later than `b`.
inline bool operator<=(const Uptime& a, const Uptime& b) {
  return a.inMicros() <= b.inMicros();
}

/// Returns true if `a` is not earlier than `b`.
inline bool operator>=(const Uptime& a, const Uptime& b) {
  return a.inMicros() >= b.inMicros();
}

/// Returns elapsed duration between two uptime instants.
inline Duration operator-(const Uptime& a, const Uptime& b) {
  return Micros(a.inMicros() - b.inMicros());
}

/// Returns uptime shifted by duration.
inline Uptime operator+(const Uptime& u, const Duration& i) {
  return Uptime(u.inMicros() + i.inMicros());
}

/// Returns uptime shifted backwards by duration.
inline Uptime operator-(const Uptime& u, const Duration& i) {
  return Uptime(u.inMicros() - i.inMicros());
}

/// Returns uptime shifted by duration.
inline Uptime operator+(const Duration& i, const Uptime& u) {
  return Uptime(u.inMicros() + i.inMicros());
}

/// Delays execution for `duration`.
///
/// Negative durations are treated as no-op.
void Delay(Duration duration);

/// Delays execution until `deadline`.
///
/// If deadline is in the past, returns immediately.
void DelayUntil(Uptime deadline);

/// Represents absolute wall time since Unix epoch.
///
/// Stored with microsecond precision and 64-bit range. Does not account for
/// leap seconds.
class WallTime {
 public:
  /// Constructs epoch wall time.
  WallTime() {}

  /// Constructs wall time from offset since Unix epoch.
  explicit WallTime(Duration since_epoch) : since_epoch_(since_epoch) {}

  /// Returns elapsed duration since Unix epoch.
  [[nodiscard]] Duration sinceEpoch() const { return since_epoch_; }

  /// Adds duration to this wall time.
  WallTime& operator+=(const Duration& i) {
    since_epoch_ += i;
    return *this;
  }

  /// Subtracts duration from this wall time.
  WallTime& operator-=(const Duration& i) {
    since_epoch_ -= i;
    return *this;
  }

 private:
  friend WallTime operator+(const WallTime&, const Duration&);
  friend WallTime operator-(const WallTime&, const Duration&);
  friend WallTime operator+(const Duration&, const WallTime&);

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
  return a.sinceEpoch() - b.sinceEpoch();
}

/// Returns wall time shifted by duration.
inline WallTime operator+(const WallTime& t, const Duration& i) {
  return WallTime(t.sinceEpoch() + i);
}

/// Returns wall time shifted backwards by duration.
inline WallTime operator-(const WallTime& t, const Duration& i) {
  return WallTime(t.sinceEpoch() - i);
}

/// Returns wall time shifted by duration.
inline WallTime operator+(const Duration& i, const WallTime& t) {
  return WallTime(t.sinceEpoch() + i);
}

/// Abstract interface for obtaining current wall time.
class WallTimeClock {
 public:
  /// Virtual destructor.
  virtual ~WallTimeClock() = default;

  /// Returns current wall time.
  virtual WallTime now() const = 0;
};

#ifdef CTIME_HDR_DEFINED
/// Wall-time clock backed by `gettimeofday`.
class SystemClock : public WallTimeClock {
 public:
  /// Returns current system wall time.
  WallTime now() const override {
    struct timeval tv;
    if (gettimeofday(&tv, nullptr)) return WallTime();
    return WallTime(Micros(tv.tv_sec * 1000000LL + tv.tv_usec));
  }
};
#endif

class TimeZone {
 public:
  /// Constructs UTC timezone.
  TimeZone() : offset_minutes_(0) {}

  /// Creates time zone with specified UTC offset.
  constexpr explicit TimeZone(Duration offset)
      : offset_minutes_(offset.inMinutes()) {}

  /// Returns UTC offset of this time zone.
  [[nodiscard]] constexpr Duration offset() const {
    return Minutes(offset_minutes_);
  }

 private:
  int16_t offset_minutes_;
};

namespace timezone {
constexpr TimeZone UTC = TimeZone(Micros(0));
}

enum DayOfWeek {
  kSunday = 0,
  kMonday = 1,
  kTuesday = 2,
  kWednesday = 3,
  kThursday = 4,
  kFriday = 5,
  kSaturday = 6
};

enum Month {
  kJanuary = 1,
  kFebruary = 2,
  kMarch = 3,
  kApril = 4,
  kMay = 5,
  kJune = 6,
  kJuly = 7,
  kAugust = 8,
  kSeptember = 9,
  kOctober = 10,
  kNovember = 11,
  kDecember = 12
};

/// Represents wall time decomposed into date/time in a specific time zone.
///
/// Does not account for leap seconds.
class DateTime {
 public:
  /// Constructs `DateTime` representing current time in UTC.
  DateTime() : DateTime(WallTime(), timezone::UTC) {}

  /// Constructs `DateTime` at midnight of a date in the specified time zone.
  ///
  /// @param year Four-digit year.
  /// @param month Month in [1, 12].
  /// @param day Day in [1, max_day_of_month].
  DateTime(uint16_t year, uint8_t month, uint8_t day, TimeZone tz);

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
           uint8_t minute, uint8_t second, uint32_t micros, TimeZone tz);

  /// Constructs `DateTime` for `wallTime` in time zone `tz`.
  DateTime(WallTime wallTime, TimeZone tz);

  /// Returns `WallTime` corresponding to this `DateTime`.
  [[nodiscard]] WallTime wallTime() const { return walltime_; }

  /// Returns time zone of this `DateTime`.
  [[nodiscard]] TimeZone timeZone() const { return tz_; }

  /// Returns four-digit year.
  [[nodiscard]] int16_t year() const { return year_; }

  /// Returns month in [1, 12].
  [[nodiscard]] Month month() const { return (Month)month_; }

  /// Returns day of month in valid range.
  [[nodiscard]] uint8_t day() const { return day_; }

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
  /// Constructs `DateTime` from C `tm` structure.
  DateTime(struct tm t, TimeZone tz = timezone::UTC)
      : DateTime(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min,
                 t.tm_sec, 0, tz) {}

  /// Returns this value as a C `tm` structure.
  struct tm tmStruct() const {
    return tm{.tm_sec = second_,
              .tm_min = minute_,
              .tm_hour = hour_,
              .tm_mday = day_,
              .tm_mon = month_ - 1,
              .tm_year = year_ - 1900,
              .tm_wday = day_of_week_,
              .tm_yday = day_of_year_,
              .tm_isdst = -1};
  }
#endif

 private:
  WallTime walltime_;
  TimeZone tz_;
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
  return a.wallTime() == b.wallTime() &&
         a.timeZone().offset() == b.timeZone().offset();
}

/// Returns true if date-times differ in instant or time-zone offset.
inline bool operator!=(const DateTime& a, const DateTime& b) {
  return a.wallTime() != b.wallTime() ||
         a.timeZone().offset() != b.timeZone().offset();
}

}  // namespace roo_time

#if defined(__linux__)

/// Convenience printers to aid testing.
#include <iomanip>
#include <ostream>

/// Streams textual `Duration` representation for tests.
inline std::ostream& operator<<(std::ostream& os,
                                const roo_time::Duration& duration) {
  os << duration.inMicros() << " us";
  return os;
}

/// Streams textual `Uptime` representation for tests.
inline std::ostream& operator<<(std::ostream& os, const roo_time::Uptime& t) {
  os << (t - roo_time::Uptime::Start()) << " uptime";
  return os;
}

/// Streams textual `WallTime` representation for tests.
inline std::ostream& operator<<(std::ostream& os, const roo_time::WallTime& t) {
  os << t.sinceEpoch() << " since Epoch";
  return os;
}

/// Streams textual `DateTime` representation for tests.
inline std::ostream& operator<<(std::ostream& os,
                                const roo_time::DateTime& dt) {
  os << std::setfill('0') << std::setw(4) << (int)dt.year() << "-";
  os << std::setfill('0') << std::setw(2) << (int)dt.month() << "-";
  os << std::setfill('0') << std::setw(2) << (int)dt.day() << " ";
  os << std::setfill('0') << std::setw(2) << (int)dt.hour() << ":";
  os << std::setfill('0') << std::setw(2) << (int)dt.minute() << ":";
  os << std::setfill('0') << std::setw(2) << (int)dt.second() << ".";
  os << std::setfill('0') << std::setw(6) << (int64_t)dt.micros();
  if (dt.timeZone().offset().inMicros() > 0) {
    os << "+";
  }
  os << dt.timeZone().offset().inMinutes() << "min";
  return os;
}

#endif
