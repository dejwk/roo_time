#pragma once

#include <inttypes.h>

// Convenience classes for handling delays, measure elapsed time, etc.,
// providing safety against common errors like confusing time units
// or confusing 'timestamps' with 'intervals'.

namespace roo_time {

// Represents an 'amount of time', e.g. e.g. 5s, 10 min, etc.
// Internally represented with microsecond precition and 64-bit range.
// POD; pass it by value.
class Interval {
 public:
  constexpr Interval() : micros_(0) {}

  static const Interval Max() { return Interval(0x7FFFFFFFFFFFFFFF); }

  constexpr int64_t inMicros() const { return micros_; }
  constexpr int64_t inMillis() const { return micros_ / 1000LL; }
  constexpr int64_t inSeconds() const { return micros_ / 1000000LL; }
  constexpr int64_t inMinutes() const { return micros_ / 60000000LL; }
  constexpr int64_t inHours() const { return micros_ / 3600000000LL; }

  Interval& operator+=(const Interval& other) {
    micros_ += other.inMicros();
    return *this;
  }

  Interval& operator-=(const Interval& other) {
    micros_ -= other.inMicros();
    return *this;
  }

 private:
  friend constexpr Interval Micros(int64_t micros);
  friend constexpr Interval Millis(int64_t millis);
  friend constexpr Interval Seconds(int64_t seconds);
  friend constexpr Interval Minutes(int64_t minutes);
  friend constexpr Interval Hours(int64_t hours);

  constexpr Interval(int64_t micros) : micros_(micros) {}

  int64_t micros_;
};

inline constexpr Interval Micros(int64_t micros) { return Interval(micros); }

inline constexpr Interval Millis(int64_t millis) {
  return Interval(millis * 1000);
}

inline constexpr Interval Seconds(int64_t seconds) {
  return Interval(seconds * 1000 * 1000);
}

inline constexpr Interval Minutes(int64_t minutes) {
  return Interval(minutes * 1000 * 1000 * 60);
}

inline constexpr Interval Hours(int64_t hours) {
  return Interval(hours * 1000 * 1000 * 60 * 60);
}

inline bool operator==(const Interval& a, const Interval& b) {
  return a.inMicros() == b.inMicros();
}

inline bool operator!=(const Interval& a, const Interval& b) {
  return a.inMicros() != b.inMicros();
}

inline bool operator<(const Interval& a, const Interval& b) {
  return a.inMicros() < b.inMicros();
}

inline bool operator>(const Interval& a, const Interval& b) {
  return a.inMicros() > b.inMicros();
}

inline bool operator<=(const Interval& a, const Interval& b) {
  return a.inMicros() <= b.inMicros();
}

inline bool operator>=(const Interval& a, const Interval& b) {
  return a.inMicros() >= b.inMicros();
}

inline Interval operator+(const Interval& a, const Interval& b) {
  return Micros(a.inMicros() + b.inMicros());
}

inline Interval operator-(const Interval& a, const Interval& b) {
  return Micros(a.inMicros() - b.inMicros());
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

  // Interval HowLongAgo() const {
  //   return Interval(Now().ToMicros() - this->ToMicros);
  // }

  Uptime& operator+=(const Interval& i) {
    micros_ += i.inMicros();
    return *this;
  }

  Uptime& operator-=(const Interval& i) {
    micros_ -= i.inMicros();
    return *this;
  }

 private:
  friend Uptime operator+(const Uptime& u, const Interval& i);
  friend Uptime operator-(const Uptime& u, const Interval& i);
  friend Uptime operator+(const Interval& i, const Uptime& u);

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

inline Interval operator-(const Uptime& a, const Uptime& b) {
  return Micros(a.inMicros() - b.inMicros());
}

inline Uptime operator+(const Uptime& u, const Interval& i) {
  return Uptime(u.inMicros() + i.inMicros());
}

inline Uptime operator-(const Uptime& u, const Interval& i) {
  return Uptime(u.inMicros() - i.inMicros());
}

inline Uptime operator+(const Interval& i, const Uptime& u) {
  return Uptime(u.inMicros() + i.inMicros());
}

// Represents an absolute 'instant in time'. Internally represented with
// microsecond precision and 64-bit range. Does not account for leap seconds.
// POD; pass it by value.
class WallTime {
 public:
  WallTime() {}
  explicit WallTime(Interval since_epoch) : since_epoch_(since_epoch) {}

  Interval sinceEpoch() const { return since_epoch_; }

  WallTime& operator+=(const Interval& i) {
    since_epoch_ += i;
    return *this;
  }

  WallTime& operator-=(const Interval& i) {
    since_epoch_ -= i;
    return *this;
  }

 private:
  friend WallTime operator+(const WallTime&, const Interval&);
  friend WallTime operator-(const WallTime&, const Interval&);
  friend WallTime operator+(const Interval&, const WallTime&);

  Interval since_epoch_;
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

inline Interval operator-(const WallTime& a, const WallTime& b) {
  return a.sinceEpoch() - b.sinceEpoch();
}

inline WallTime operator+(const WallTime& t, const Interval& i) {
  return WallTime(t.sinceEpoch() + i);
}

inline WallTime operator-(const WallTime& t, const Interval& i) {
  return WallTime(t.sinceEpoch() - i);
}

inline WallTime operator+(const Interval& i, const WallTime& t) {
  return WallTime(t.sinceEpoch() + i);
}

// Abstract interface for a provider of current wall time.
class WallTimeClock {
 public:
  virtual ~WallTimeClock() {}
  virtual WallTime now() const = 0;
};

class TimeZone {
 public:
  // Creates a time zone with the given UTC offset.
  constexpr explicit TimeZone(Interval offset)
      : offset_minutes_(offset.inMinutes()) {}

  // Returns the UTC offset of this timezone.
  constexpr Interval offset() const { return Minutes(offset_minutes_); }

 private:
  int16_t offset_minutes_;
};

namespace timezone {
constexpr TimeZone UTC = TimeZone(Micros(0));
}

enum DayOfWeek {
  SUNDAY = 0,
  MONDAY,
  TUESDAY,
  WEDNESDAY,
  THURSDAY,
  FRIDAY,
  SATURDAY
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
  uint8_t month() const { return month_; }

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
                                const roo_time::Interval& interval) {
  os << interval.inMicros() << " us";
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
