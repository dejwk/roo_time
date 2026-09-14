#pragma once

#include "roo_time/duration.h"

namespace roo_time {
/// Represents an instant relative to the selected uptime clock origin.
///
/// Stored in signed 64-bit microseconds; clock resolution and sleep accounting
/// depend on the backend. See README for origins and concurrency requirements.
class Uptime {
 public:
  /// Returns current uptime.
  /// Generic Arduino: serialize calls and sample before the first wrap, then
  /// at intervals shorter than 2^32 microseconds. Linux: zero is first access.
  static const Uptime Now();

  /// Returns zero in the backend clock domain; not a wall-time timestamp.
  static const Uptime Start() { return Uptime(0); }

  /// Returns the maximum representable uptime value.
  static const Uptime Max() { return Uptime(0x7FFFFFFFFFFFFFFF); }

  /// Constructs zero uptime value.
  Uptime() : micros_(0) {}

  /// Copy constructor.
  Uptime(const Uptime& other) : micros_(other.micros_) {}

  /// Copy constructor for volatile sources; does not provide atomicity or
  /// synchronization. Shared mutation requires external synchronization.
  Uptime(const volatile Uptime& other) : micros_(other.micros_) {}

  /// Assignment operator.
  Uptime& operator=(const Uptime& other) {
    micros_ = other.micros_;
    return *this;
  }

  /// Assignment from volatile sources; not atomic or synchronized.
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

/// Opaque wrapping millisecond timestamp. No epoch or absolute-time accessor.
/// Ordered comparisons and subtraction require actual separation strictly less
/// than 2^31 milliseconds, within the same clock domain. Equality compares
/// bits.
class SmallTimestamp {
 public:
  constexpr SmallTimestamp() : millis_(0) {}

  /// Truncates uptime to milliseconds and retains its low 32 bits.
  SmallTimestamp(Uptime uptime)
      : millis_(static_cast<uint32_t>(uptime.inMillis())) {}
  /// Reads the platform millisecond clock directly, without extending micros().
  /// Generic Arduino uses millis(); see README for clock-domain differences.
  static SmallTimestamp Now();

  /// Shift by a duration, truncating to milliseconds; the shift must fit
  /// int32_t.
  SmallTimestamp& operator+=(Duration duration) {
    millis_ += static_cast<uint32_t>(SmallDuration(duration).inMillis());
    return *this;
  }
  SmallTimestamp& operator-=(Duration duration) {
    millis_ -= static_cast<uint32_t>(SmallDuration(duration).inMillis());
    return *this;
  }

  friend constexpr bool operator==(SmallTimestamp a, SmallTimestamp b) {
    return a.millis_ == b.millis_;
  }
  friend constexpr bool operator!=(SmallTimestamp a, SmallTimestamp b) {
    return !(a == b);
  }
  friend constexpr SmallDuration operator-(SmallTimestamp a, SmallTimestamp b) {
    const uint32_t delta = a.millis_ - b.millis_;
    assert(delta != 0x80000000u);  // Exactly half a cycle is ambiguous.
    const int64_t signed_delta =
        delta <= INT32_MAX ? static_cast<int64_t>(delta)
                           : static_cast<int64_t>(delta) - 0x100000000LL;
    return SmallDuration::Millis(static_cast<int32_t>(signed_delta));
  }

 private:
  uint32_t millis_;
};

inline SmallTimestamp operator+(SmallTimestamp t, Duration d) { return t += d; }
inline SmallTimestamp operator+(Duration d, SmallTimestamp t) { return t += d; }
inline SmallTimestamp operator-(SmallTimestamp t, Duration d) { return t -= d; }

inline constexpr bool operator<(SmallTimestamp a, SmallTimestamp b) {
  return (a - b).inMillis() < 0;
}
inline constexpr bool operator>(SmallTimestamp a, SmallTimestamp b) {
  return (a - b).inMillis() > 0;
}
inline constexpr bool operator<=(SmallTimestamp a, SmallTimestamp b) {
  return (a - b).inMillis() <= 0;
}
inline constexpr bool operator>=(SmallTimestamp a, SmallTimestamp b) {
  return (a - b).inMillis() >= 0;
}

/// Delays execution for `duration`.
///
/// Zero and negative durations are no-ops. Positive waits recheck elapsed
/// uptime and can overshoot due to scheduling. Requires a progressing clock and
/// its concurrency/sampling contracts. Call in task/loop context, not from an
/// ISR.
void Delay(Duration duration);

/// Delays execution until `deadline`.
///
/// If deadline is at or before now, returns immediately. Otherwise returns at
/// or after the deadline, subject to the same backend requirements as Delay.
void DelayUntil(Uptime deadline);

}  // namespace roo_time
