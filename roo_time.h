#pragma once

extern "C" {
unsigned long int micros();  // Provided by <Arduino.h>
}

// Convenience classes for handling delays, measure elapsed time, etc.,
// providing safety against common errors like confusing time units
// or confusing 'timestamps' with 'intervals'.

namespace roo_time {

// Represents an 'amount of time', e.g. e.g. 5s, 10 min, etc.
// Internally represented with microsecond precition and 64-bit range.
// POD; pass it by value.
class Interval {
 public:
  Interval() : micros_(0) {}

  static const Interval Max() { return Interval(0x7FFFFFFFFFFFFFFF); }

  int64_t ToMicros() const { return micros_; }
  int64_t ToMillis() const { return micros_ / 1000LL; }
  int64_t ToSeconds() const { return micros_ / 1000000LL; }
  int64_t ToMinutes() const { return micros_ / 60000000LL; }
  int64_t ToHours() const { return micros_ / 3600000000LL; }

  Interval& operator+=(const Interval& other) {
    micros_ += other.ToMicros();
    return *this;
  }

  Interval& operator-=(const Interval& other) {
    micros_ -= other.ToMicros();
    return *this;
  }

 private:
  friend Interval Micros(int64_t micros);
  friend Interval Millis(int64_t millis);
  friend Interval Seconds(int64_t seconds);
  friend Interval Minutes(int64_t minutes);
  friend Interval Hours(int64_t hours);

  Interval(int64_t micros) : micros_(micros) {}

  int64_t micros_;
};

inline Interval Micros(int64_t micros) { return Interval(micros); }

inline Interval Millis(int64_t millis) { return Interval(millis * 1000); }

inline Interval Seconds(int64_t seconds) {
  return Interval(seconds * 1000 * 1000);
}

inline Interval Minutes(int64_t minutes) {
  return Interval(minutes * 1000 * 1000 * 60);
}

inline Interval Hours(int64_t hours) {
  return Interval(hours * 1000 * 1000 * 60 * 60);
}

inline bool operator==(const Interval& a, const Interval& b) {
  return a.ToMicros() == b.ToMicros();
}

inline bool operator!=(const Interval& a, const Interval& b) {
  return a.ToMicros() != b.ToMicros();
}

inline bool operator<(const Interval& a, const Interval& b) {
  return a.ToMicros() < b.ToMicros();
}

inline bool operator>(const Interval& a, const Interval& b) {
  return a.ToMicros() > b.ToMicros();
}

inline bool operator<=(const Interval& a, const Interval& b) {
  return a.ToMicros() <= b.ToMicros();
}

inline bool operator>=(const Interval& a, const Interval& b) {
  return a.ToMicros() >= b.ToMicros();
}

inline Interval operator+(const Interval& a, const Interval& b) {
  return Micros(a.ToMicros() + b.ToMicros());
}

inline Interval operator-(const Interval& a, const Interval& b) {
  return Micros(a.ToMicros() - b.ToMicros());
}

// Represents an 'instant in time', relatively to the start time of the
// process. Internally represented with microsecond precition and 64-bit range.
//  POD; pass it by value.
class Uptime {
 public:
  static const Uptime Now() { return Uptime(micros()); }
  static const Uptime Start() { return Uptime(0); }
  static const Uptime Max() { return Uptime(0x7FFFFFFFFFFFFFFF); }

  Uptime() : micros_(0) {}

  int64_t ToMicros() const { return micros_; }
  int64_t ToMillis() const { return micros_ / 1000LL; }
  int64_t ToSeconds() const { return micros_ / 1000000LL; }
  int64_t ToMinutes() const { return micros_ / 60000000LL; }
  int64_t ToHours() const { return micros_ / 3600000000LL; }

  // Interval HowLongAgo() const {
  //   return Interval(Now().ToMicros() - this->ToMicros);
  // }

  Uptime& operator+=(const Interval& i) {
    micros_ += i.ToMicros();
    return *this;
  }

  Uptime& operator-=(const Interval& i) {
    micros_ -= i.ToMicros();
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
  return a.ToMicros() == b.ToMicros();
}

inline bool operator!=(const Uptime& a, const Uptime& b) {
  return a.ToMicros() != b.ToMicros();
}

inline bool operator<(const Uptime& a, const Uptime& b) {
  return a.ToMicros() < b.ToMicros();
}

inline bool operator>(const Uptime& a, const Uptime& b) {
  return a.ToMicros() > b.ToMicros();
}

inline bool operator<=(const Uptime& a, const Uptime& b) {
  return a.ToMicros() <= b.ToMicros();
}

inline bool operator>=(const Uptime& a, const Uptime& b) {
  return a.ToMicros() >= b.ToMicros();
}

inline Interval operator-(const Uptime& a, const Uptime& b) {
  return Micros(a.ToMicros() - b.ToMicros());
}

inline Uptime operator+(const Uptime& u, const Interval& i) {
  return Uptime(u.ToMicros() + i.ToMicros());
}

inline Uptime operator-(const Uptime& u, const Interval& i) {
  return Uptime(u.ToMicros() - i.ToMicros());
}

inline Uptime operator+(const Interval& i, const Uptime& u) {
  return Uptime(u.ToMicros() + i.ToMicros());
}

}  // namespace roo_time
