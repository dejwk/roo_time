#pragma once

#include <inttypes.h>
#if defined(ESP_PLATFORM) || defined(__linux__)
#define CTIME_HDR_DEFINED
#include <sys/time.h>

#include <ctime>
#endif

// Convenience classes for handling delays, measure elapsed time, etc.,
// providing safety against common errors like confusing time units
// or confusing 'timestamps' with 'durations'.

namespace roo_time {

// Represents an 'amount of time', e.g. e.g. 5s, 10 min, etc.
// Internally represented with microsecond precision and 64-bit range.
// Should be passed by value.
class Duration {
 public:
  struct Components {
    bool negative : 1;
    uint64_t days : 26;
    uint8_t hours : 5;
    uint8_t minutes : 6;
    uint8_t seconds : 6;
    uint32_t micros : 20;
  };

  constexpr Duration() : micros_(0) {}

  static const Duration Max() { return Duration(0x7FFFFFFFFFFFFFFF); }

  constexpr int64_t inMicros() const { return micros_; }

  constexpr int64_t inMillis() const { return inMillisRoundedDown(); }
  constexpr int64_t inSeconds() const { return inSecondsRoundedDown(); }
  constexpr int64_t inMinutes() const { return inMinutesRoundedDown(); }
  constexpr int64_t inHours() const { return inHoursRoundedDown(); }

  constexpr int64_t inMillisRoundedDown() const { return micros_ / 1000LL; }

  constexpr int64_t inSecondsRoundedDown() const { return micros_ / 1000000LL; }

  constexpr int64_t inMinutesRoundedDown() const {
    return micros_ / 60000000LL;
  }

  constexpr int64_t inHoursRoundedDown() const {
    return micros_ / 3600000000LL;
  }

  constexpr int64_t inMillisRoundedUp() const {
    return (micros_ + 999LL) / 1000LL;
  }

  constexpr int64_t inSecondsRoundedUp() const {
    return (micros_ + 999999LL) / 1000000LL;
  }

  constexpr int64_t inMinutesRoundedUp() const {
    return (micros_ + 59999999LL) / 60000000LL;
  }

  constexpr int64_t inHoursRoundedUp() const {
    return (micros_ + 3599999999LL) / 3600000000LL;
  }

  constexpr int64_t inMillisRoundedNearest() const {
    return (micros_ + 499LL) / 1000LL;
  }

  constexpr int64_t inSecondsRoundedNearest() const {
    return (micros_ + 499999LL) / 1000000LL;
  }

  constexpr int64_t inMinutesRoundedNearest() const {
    return (micros_ + 29999999LL) / 60000000LL;
  }

  constexpr int64_t inHoursRoundedNearest() const {
    return (micros_ + 1799999999LL) / 3600000000LL;
  }

  constexpr float inMillisFloat() const { return micros_ / 1000.0; }
  constexpr float inSecondsFloat() const { return micros_ / 1000000.0; }
  constexpr float inMinutesFloat() const { return micros_ / 60000000.0; }
  constexpr float inHoursFloat() const { return micros_ / 3600000000.0; }

  Duration& operator+=(const Duration& other) {
    micros_ += other.inMicros();
    return *this;
  }

  Duration& operator-=(const Duration& other) {
    micros_ -= other.inMicros();
    return *this;
  }

  // Breaks the duration into components (days, hours, minutes, etc.)
  Components toComponents();

  // Reconstitutes the duration from the components.
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

// For backwards compatibiliity. Prefer 'Duration' in the new code.
using Interval = Duration;

inline constexpr Duration Micros(long long micros) { return Duration(micros); }

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

inline bool operator==(const Duration& a, const Duration& b) {
  return a.inMicros() == b.inMicros();
}

inline bool operator!=(const Duration& a, const Duration& b) {
  return a.inMicros() != b.inMicros();
}

inline bool operator<(const Duration& a, const Duration& b) {
  return a.inMicros() < b.inMicros();
}

inline bool operator>(const Duration& a, const Duration& b) {
  return a.inMicros() > b.inMicros();
}

inline bool operator<=(const Duration& a, const Duration& b) {
  return a.inMicros() <= b.inMicros();
}

inline bool operator>=(const Duration& a, const Duration& b) {
  return a.inMicros() >= b.inMicros();
}

inline Duration operator+(const Duration& a, const Duration& b) {
  return Micros(a.inMicros() + b.inMicros());
}

inline Duration operator-(const Duration& a, const Duration& b) {
  return Micros(a.inMicros() - b.inMicros());
}

inline Duration operator*(const Duration& a, int b) {
  return Micros(a.inMicros() * b);
}

inline Duration operator*(int a, const Duration& b) {
  return Micros(a * b.inMicros());
}

// Represents an 'instant in time', relatively to the start time of the
// process. Internally represented with microsecond precition and 64-bit range.
// May not count time while the board is in sleep mode. POD; pass it by value.
class Uptime {
 public:
  static const Uptime Now();

  static const Uptime Start() { return Uptime(0); }
  static const Uptime Max() { return Uptime(0x7FFFFFFFFFFFFFFF); }

  Uptime() : micros_(0) {}
  Uptime(const Uptime& other) : micros_(other.micros_) {}
  Uptime(const volatile Uptime& other) : micros_(other.micros_) {}

  Uptime& operator=(const Uptime& other) {
    micros_ = other.micros_;
    return *this;
  }

  Uptime& operator=(const volatile Uptime& other) {
    micros_ = other.micros_;
    return *this;
  }

  int64_t inMicros() const { return micros_; }
  int64_t inMillis() const { return micros_ / 1000LL; }
  int64_t inSeconds() const { return micros_ / 1000000LL; }
  int64_t inMinutes() const { return micros_ / 60000000LL; }
  int64_t inHours() const { return micros_ / 3600000000LL; }

  // Duration HowLongAgo() const {
  //   return Duration(Now().ToMicros() - this->ToMicros);
  // }

  Uptime& operator+=(const Duration& i) {
    micros_ += i.inMicros();
    return *this;
  }

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

inline bool operator==(const Uptime& a, const Uptime& b) {
  return a.inMicros() == b.inMicros();
}

inline bool operator!=(const Uptime& a, const Uptime& b) {
  return a.inMicros() != b.inMicros();
}

inline bool operator<(const Uptime& a, const Uptime& b) {
  return a.inMicros() < b.inMicros();
}

inline bool operator>(const Uptime& a, const Uptime& b) {
  return a.inMicros() > b.inMicros();
}

inline bool operator<=(const Uptime& a, const Uptime& b) {
  return a.inMicros() <= b.inMicros();
}

inline bool operator>=(const Uptime& a, const Uptime& b) {
  return a.inMicros() >= b.inMicros();
}

inline Duration operator-(const Uptime& a, const Uptime& b) {
  return Micros(a.inMicros() - b.inMicros());
}

inline Uptime operator+(const Uptime& u, const Duration& i) {
  return Uptime(u.inMicros() + i.inMicros());
}

inline Uptime operator-(const Uptime& u, const Duration& i) {
  return Uptime(u.inMicros() - i.inMicros());
}

inline Uptime operator+(const Duration& i, const Uptime& u) {
  return Uptime(u.inMicros() + i.inMicros());
}

// Delays execution for the specified duration. Does nothing if the duration is
// negative.
void Delay(Duration duration);

// Delays execution until the specified deadline. Does nothing if the
// deadline has already passed.
void DelayUntil(Uptime deadline);

// Represents an absolute 'instant in time'. Internally represented with
// microsecond precision and 64-bit range. Does not account for leap seconds.
// Lightweight (8 bytes); pass it by value.
class WallTime {
 public:
  WallTime() {}
  explicit WallTime(Duration since_epoch) : since_epoch_(since_epoch) {}

  Duration sinceEpoch() const { return since_epoch_; }

  WallTime& operator+=(const Duration& i) {
    since_epoch_ += i;
    return *this;
  }

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

inline bool operator==(const WallTime& a, const WallTime& b) {
  return a.sinceEpoch() == b.sinceEpoch();
}

inline bool operator!=(const WallTime& a, const WallTime& b) {
  return a.sinceEpoch() != b.sinceEpoch();
}

inline bool operator<(const WallTime& a, const WallTime& b) {
  return a.sinceEpoch() < b.sinceEpoch();
}

inline bool operator>(const WallTime& a, const WallTime& b) {
  return a.sinceEpoch() > b.sinceEpoch();
}

inline bool operator<=(const WallTime& a, const WallTime& b) {
  return a.sinceEpoch() <= b.sinceEpoch();
}

inline bool operator>=(const WallTime& a, const WallTime& b) {
  return a.sinceEpoch() >= b.sinceEpoch();
}

inline Duration operator-(const WallTime& a, const WallTime& b) {
  return a.sinceEpoch() - b.sinceEpoch();
}

inline WallTime operator+(const WallTime& t, const Duration& i) {
  return WallTime(t.sinceEpoch() + i);
}

inline WallTime operator-(const WallTime& t, const Duration& i) {
  return WallTime(t.sinceEpoch() - i);
}

inline WallTime operator+(const Duration& i, const WallTime& t) {
  return WallTime(t.sinceEpoch() + i);
}

// Abstract interface for a provider of current wall time.
class WallTimeClock {
 public:
  virtual ~WallTimeClock() = default;
  virtual WallTime now() const = 0;
};

#ifdef CTIME_HDR_DEFINED
class SystemClock : public WallTimeClock {
 public:
  WallTime now() const override {
    struct timeval tv;
    if (gettimeofday(&tv, nullptr)) return WallTime();
    return WallTime(Micros(tv.tv_sec * 1000000LL + tv.tv_usec));
  }
};
#endif

class TimeZone {
 public:
  TimeZone() : offset_minutes_(0) {}

  // Creates a time zone with the given UTC offset.
  constexpr explicit TimeZone(Duration offset)
      : offset_minutes_(offset.inMinutes()) {}

  // Returns the UTC offset of this timezone.
  constexpr Duration offset() const { return Minutes(offset_minutes_); }

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

// Represents an absolute 'instant in time', represented as a date and time in
// a specific timezone. Does not account for leap seconds.
class DateTime {
 public:
  // Constructs a new DateTime, representing 'now' in UTC.
  DateTime() : DateTime(WallTime(), timezone::UTC) {}

  // Constructs a new DateTime, representing the midnight of the sepcified
  // day, in the specified time zone.
  //
  // year: 4-digit.
  // month: 1..12
  // day: 1..max_day
  //
  DateTime(uint16_t year, uint8_t month, uint8_t day, TimeZone tz);

  DateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour,
           uint8_t minute, uint8_t second, uint32_t micros, TimeZone tz);

  // Constructs a new DateTime, representing the specified WallTime
  // in the specified time zone.
  DateTime(WallTime wallTime, TimeZone tz);

  // Returns WallTime corresponding to this DateTime.
  WallTime wallTime() const { return walltime_; }

  // Returns the timezone of this DateTime.
  TimeZone timeZone() const { return tz_; }

  // 4-digit.
  int16_t year() const { return year_; }

  // [1..12]
  Month month() const { return (Month)month_; }

  // [1..max_day]
  uint8_t day() const { return day_; }

  // [0..23]
  uint8_t hour() const { return hour_; }

  // [0..59]
  uint8_t minute() const { return minute_; }

  // [0..59]
  uint8_t second() const { return second_; }

  // [0..999999]
  uint32_t micros() const { return micros_; }

  DayOfWeek dayOfWeek() const { return day_of_week_; }

  // [1..366]
  uint16_t dayOfYear() const { return day_of_year_; }

#ifdef CTIME_HDR_DEFINED
  DateTime(struct tm t, TimeZone tz = timezone::UTC)
      : DateTime(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min,
                 t.tm_sec, 0, tz) {}

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

inline bool operator==(const DateTime& a, const DateTime& b) {
  return a.wallTime() == b.wallTime() &&
         a.timeZone().offset() == b.timeZone().offset();
}

inline bool operator!=(const DateTime& a, const DateTime& b) {
  return a.wallTime() != b.wallTime() &&
         a.timeZone().offset() != b.timeZone().offset();
}

}  // namespace roo_time

#if defined(__linux__)

// Convenience printers to aid testing.
#include <iomanip>
#include <ostream>

inline std::ostream& operator<<(std::ostream& os,
                                const roo_time::Duration& duration) {
  os << duration.inMicros() << " us";
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const roo_time::Uptime& t) {
  os << (t - roo_time::Uptime::Start()) << " uptime";
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const roo_time::WallTime& t) {
  os << t.sinceEpoch() << " since Epoch";
  return os;
}

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
