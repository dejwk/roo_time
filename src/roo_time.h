#pragma once

/// Umbrella header for the roo_time module.
///
/// Provides duration, uptime, and wall-time abstractions.

#include <cassert>
#include <inttypes.h>
#include <type_traits>
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

class Duration;
class SmallDuration;
template <int64_t MicrosPerUnit, typename Rep> class IntegerTime;

namespace internal {
// Empty CRTP base: one implementation of all read-only unit conversions, with
// no storage or virtual dispatch. Derived types provide inMicros().
template <typename Derived> class DurationConversions {
public:
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
  [[nodiscard]] constexpr int64_t inHours() const {
    return inHoursRoundedDown();
  }

  /// Returns duration in milliseconds, rounded toward zero.
  [[nodiscard]] constexpr int64_t inMillisRoundedDown() const {
    return derived().inMicros() / 1000LL;
  }

  /// Returns duration in seconds, rounded toward zero.
  [[nodiscard]] constexpr int64_t inSecondsRoundedDown() const {
    return derived().inMicros() / 1000000LL;
  }

  /// Returns duration in minutes, rounded toward zero.
  [[nodiscard]] constexpr int64_t inMinutesRoundedDown() const {
    return derived().inMicros() / 60000000LL;
  }

  /// Returns duration in hours, rounded toward zero.
  [[nodiscard]] constexpr int64_t inHoursRoundedDown() const {
    return derived().inMicros() / 3600000000LL;
  }

  /// Returns duration in milliseconds, rounded away from zero.
  [[nodiscard]] constexpr int64_t inMillisRoundedUp() const {
    int64_t q = derived().inMicros() / 1000LL;
    int64_t r = derived().inMicros() % 1000LL;
    if (r == 0)
      return q;
    return derived().inMicros() > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in seconds, rounded away from zero.
  [[nodiscard]] constexpr int64_t inSecondsRoundedUp() const {
    int64_t q = derived().inMicros() / 1000000LL;
    int64_t r = derived().inMicros() % 1000000LL;
    if (r == 0)
      return q;
    return derived().inMicros() > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in minutes, rounded away from zero.
  [[nodiscard]] constexpr int64_t inMinutesRoundedUp() const {
    int64_t q = derived().inMicros() / 60000000LL;
    int64_t r = derived().inMicros() % 60000000LL;
    if (r == 0)
      return q;
    return derived().inMicros() > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in hours, rounded away from zero.
  [[nodiscard]] constexpr int64_t inHoursRoundedUp() const {
    int64_t q = derived().inMicros() / 3600000000LL;
    int64_t r = derived().inMicros() % 3600000000LL;
    if (r == 0)
      return q;
    return derived().inMicros() > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in milliseconds, rounded to nearest (ties away from
  /// zero).
  [[nodiscard]] constexpr int64_t inMillisRoundedNearest() const {
    int64_t q = derived().inMicros() / 1000LL;
    int64_t r = derived().inMicros() % 1000LL;
    int64_t ar = r < 0 ? -r : r;
    if (ar * 2 < 1000LL)
      return q;
    return derived().inMicros() > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in seconds, rounded to nearest (ties away from zero).
  [[nodiscard]] constexpr int64_t inSecondsRoundedNearest() const {
    int64_t q = derived().inMicros() / 1000000LL;
    int64_t r = derived().inMicros() % 1000000LL;
    int64_t ar = r < 0 ? -r : r;
    if (ar * 2 < 1000000LL)
      return q;
    return derived().inMicros() > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in minutes, rounded to nearest (ties away from zero).
  [[nodiscard]] constexpr int64_t inMinutesRoundedNearest() const {
    int64_t q = derived().inMicros() / 60000000LL;
    int64_t r = derived().inMicros() % 60000000LL;
    int64_t ar = r < 0 ? -r : r;
    if (ar * 2 < 60000000LL)
      return q;
    return derived().inMicros() > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in hours, rounded to nearest (ties away from zero).
  [[nodiscard]] constexpr int64_t inHoursRoundedNearest() const {
    int64_t q = derived().inMicros() / 3600000000LL;
    int64_t r = derived().inMicros() % 3600000000LL;
    int64_t ar = r < 0 ? -r : r;
    if (ar * 2 < 3600000000LL)
      return q;
    return derived().inMicros() > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in milliseconds as floating-point value.
  [[nodiscard]] constexpr float inMillisFloat() const {
    return derived().inMicros() / 1000.0;
  }

  /// Returns duration in seconds as floating-point value.
  [[nodiscard]] constexpr float inSecondsFloat() const {
    return derived().inMicros() / 1000000.0;
  }

  /// Returns duration in minutes as floating-point value.
  [[nodiscard]] constexpr float inMinutesFloat() const {
    return derived().inMicros() / 60000000.0;
  }

  /// Returns duration in hours as floating-point value.
  [[nodiscard]] constexpr float inHoursFloat() const {
    return derived().inMicros() / 3600000000.0;
  }

private:
  constexpr const Derived &derived() const {
    return static_cast<const Derived &>(*this);
  }
};

constexpr int32_t CheckedSmallMillis(int64_t millis) {
  assert(millis >= INT32_MIN && millis <= INT32_MAX);
  return static_cast<int32_t>(millis);
}
} // namespace internal

/// Represents an amount of time (e.g. 5s, 10min).
///
/// Stored with microsecond precision and 64-bit range. Pass by value.
/// Arithmetic is unchecked; inputs, intermediate results, and results must fit.
/// For rounding and component saturation semantics, see README contracts.
class Duration : public internal::DurationConversions<Duration> {
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

  /// Adds another duration to this one.
  Duration &operator+=(const Duration &other) {
    micros_ += other.inMicros();
    return *this;
  }

  /// Subtracts another duration from this one.
  Duration &operator-=(const Duration &other) {
    micros_ -= other.inMicros();
    return *this;
  }

  /// Breaks duration into components (days, hours, minutes, ...).
  /// Magnitudes above 67,108,863 days, 23:59:59.999999 saturate, preserving
  /// sign.
  Components toComponents() const;

  /// Reconstructs duration from normalized components.
  /// Requires hours < 24, minutes/seconds < 60, and micros < 1000000.
  static Duration FromComponents(const Components &components);

private:
  template <int64_t, typename> friend class IntegerTime;

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

/// A count in a compile-time unit. Conversion to Duration scales in int64_t.
/// Rep is an integer type; scaled microseconds must fit int64_t when widened.
/// Helpers have read-only duration accessors; arithmetic yields Duration.
template <int64_t MicrosPerUnit, typename Rep = int32_t>
class IntegerTime
    : public internal::DurationConversions<IntegerTime<MicrosPerUnit, Rep>> {
  static_assert(MicrosPerUnit > 0, "The time unit must be positive");
  static_assert(std::is_integral<Rep>::value && sizeof(Rep) <= sizeof(int64_t),
                "The count must be an integer of at most 64 bits");

public:
  constexpr explicit IntegerTime(Rep count) : count_(count) {}

  [[nodiscard]] constexpr int64_t inMicros() const {
    if (std::is_signed<Rep>::value) {
      assert(static_cast<int64_t>(count_) >= INT64_MIN / MicrosPerUnit &&
             static_cast<int64_t>(count_) <= INT64_MAX / MicrosPerUnit);
    } else {
      assert(static_cast<uint64_t>(count_) <=
             static_cast<uint64_t>(INT64_MAX / MicrosPerUnit));
    }
    return static_cast<int64_t>(count_) * MicrosPerUnit;
  }

  constexpr operator Duration() const { return Duration(inMicros()); }

  Duration::Components toComponents() const {
    return static_cast<Duration>(*this).toComponents();
  }

private:
  Rep count_;
};

using Int32Micros = IntegerTime<1, int32_t>;
using Int32Millis = IntegerTime<1000, int32_t>;
using Int32Seconds = IntegerTime<1000000, int32_t>;
using Int32Minutes = IntegerTime<60000000, int32_t>;
using Int32Hours = IntegerTime<3600000000LL, int32_t>;

/// Retains an integer count in micros until a duration is needed.
template <typename Rep,
          typename std::enable_if<std::is_integral<Rep>::value, int>::type = 0>
inline constexpr IntegerTime<1LL, Rep> Micros(Rep count) {
  return IntegerTime<1LL, Rep>(count);
}

/// Floating microseconds are truncated, preserving the original factory
/// behavior.
template <typename Rep, typename std::enable_if<
                            std::is_floating_point<Rep>::value, int>::type = 0>
inline constexpr Duration Micros(Rep count) {
  return Micros(static_cast<int64_t>(count));
}

/// Retains an integer count in millis until a duration is needed.
template <typename Rep,
          typename std::enable_if<std::is_integral<Rep>::value, int>::type = 0>
inline constexpr IntegerTime<1000LL, Rep> Millis(Rep count) {
  return IntegerTime<1000LL, Rep>(count);
}

/// Floating input retains the existing full Duration result type.
inline constexpr Duration Millis(float count) {
  return Duration(static_cast<int64_t>(count * 1000LL));
}

/// Floating input retains the existing full Duration result type.
inline constexpr Duration Millis(double count) {
  return Duration(static_cast<int64_t>(count * 1000LL));
}

/// Retains an integer count in seconds until a duration is needed.
template <typename Rep,
          typename std::enable_if<std::is_integral<Rep>::value, int>::type = 0>
inline constexpr IntegerTime<1000000LL, Rep> Seconds(Rep count) {
  return IntegerTime<1000000LL, Rep>(count);
}

/// Floating input retains the existing full Duration result type.
inline constexpr Duration Seconds(float count) {
  return Duration(static_cast<int64_t>(count * 1000 * 1000));
}

/// Floating input retains the existing full Duration result type.
inline constexpr Duration Seconds(double count) {
  return Duration(static_cast<int64_t>(count * 1000 * 1000));
}

/// Retains an integer count in minutes until a duration is needed.
template <typename Rep,
          typename std::enable_if<std::is_integral<Rep>::value, int>::type = 0>
inline constexpr IntegerTime<60000000LL, Rep> Minutes(Rep count) {
  return IntegerTime<60000000LL, Rep>(count);
}

/// Floating input retains the existing full Duration result type.
inline constexpr Duration Minutes(float count) {
  return Duration(static_cast<int64_t>(count * 1000 * 1000 * 60));
}

/// Floating input retains the existing full Duration result type.
inline constexpr Duration Minutes(double count) {
  return Duration(static_cast<int64_t>(count * 1000 * 1000 * 60));
}

/// Retains an integer count in hours until a duration is needed.
template <typename Rep,
          typename std::enable_if<std::is_integral<Rep>::value, int>::type = 0>
inline constexpr IntegerTime<3600000000LL, Rep> Hours(Rep count) {
  return IntegerTime<3600000000LL, Rep>(count);
}

/// Floating input retains the existing full Duration result type.
inline constexpr Duration Hours(float count) {
  return Duration(static_cast<int64_t>(count * 1000 * 1000 * 60 * 60));
}

/// Floating input retains the existing full Duration result type.
inline constexpr Duration Hours(double count) {
  return Duration(static_cast<int64_t>(count * 1000 * 1000 * 60 * 60));
}

/// Signed 32-bit milliseconds. Arithmetic must remain representable; it does
/// not wrap. Widening to Duration is implicit and lossless.
class SmallDuration : public internal::DurationConversions<SmallDuration> {
public:
  constexpr SmallDuration() : millis_(0) {}

  static constexpr SmallDuration Millis(int32_t millis) {
    return SmallDuration(millis, 0);
  }

  /// Narrow explicitly from a full duration, truncating toward zero.
  /// The millisecond result must fit int32_t (asserted in debug builds).
  constexpr explicit SmallDuration(Duration duration)
      : millis_(internal::CheckedSmallMillis(duration.inMillis())) {}

  /// Unit expressions may select compact storage implicitly. The scaled result
  /// must fit, even when the original count fits its own representation.
  template <int64_t Unit, typename Rep>
  constexpr SmallDuration(IntegerTime<Unit, Rep> value)
      : SmallDuration(static_cast<Duration>(value)) {}

  [[nodiscard]] constexpr int64_t inMicros() const {
    return static_cast<int64_t>(millis_) * 1000;
  }
  [[nodiscard]] constexpr int32_t inMillis() const { return millis_; }
  constexpr operator Duration() const { return roo_time::Micros(inMicros()); }

  Duration::Components toComponents() const {
    return static_cast<Duration>(*this).toComponents();
  }

  SmallDuration &operator+=(SmallDuration other) {
    millis_ = internal::CheckedSmallMillis(static_cast<int64_t>(millis_) +
                                           other.millis_);
    return *this;
  }
  SmallDuration &operator-=(SmallDuration other) {
    millis_ = internal::CheckedSmallMillis(static_cast<int64_t>(millis_) -
                                           other.millis_);
    return *this;
  }

private:
  constexpr SmallDuration(int32_t millis, int) : millis_(millis) {}
  int32_t millis_;
};

namespace internal {
template <typename T> struct IsDurationLike : std::false_type {};
template <> struct IsDurationLike<Duration> : std::true_type {};
template <> struct IsDurationLike<SmallDuration> : std::true_type {};
template <int64_t Unit, typename Rep>
struct IsDurationLike<IntegerTime<Unit, Rep>> : std::true_type {};

template <typename A, typename B>
using EnableDurationPair = typename std::enable_if<
    IsDurationLike<A>::value && IsDurationLike<B>::value, int>::type;
} // namespace internal

/// Returns true if both durations are equal.
inline constexpr bool operator==(const Duration &a, const Duration &b) {
  return a.inMicros() == b.inMicros();
}

/// Returns true if durations differ.
inline constexpr bool operator!=(const Duration &a, const Duration &b) {
  return a.inMicros() != b.inMicros();
}

/// Returns true if `a` is shorter than `b`.
inline constexpr bool operator<(const Duration &a, const Duration &b) {
  return a.inMicros() < b.inMicros();
}

/// Returns true if `a` is longer than `b`.
inline constexpr bool operator>(const Duration &a, const Duration &b) {
  return a.inMicros() > b.inMicros();
}

/// Returns true if `a` is not longer than `b`.
inline constexpr bool operator<=(const Duration &a, const Duration &b) {
  return a.inMicros() <= b.inMicros();
}

/// Returns true if `a` is not shorter than `b`.
inline constexpr bool operator>=(const Duration &a, const Duration &b) {
  return a.inMicros() >= b.inMicros();
}

/// Returns the sum of two durations.
inline constexpr Duration operator+(const Duration &a, const Duration &b) {
  return Micros(a.inMicros() + b.inMicros());
}

/// Returns the difference between two durations.
inline constexpr Duration operator-(const Duration &a, const Duration &b) {
  return Micros(a.inMicros() - b.inMicros());
}

/// Multiplies duration by an integer factor.
inline constexpr Duration operator*(const Duration &a, int b) {
  return Micros(a.inMicros() * b);
}

/// Multiplies duration by an integer factor.
inline constexpr Duration operator*(int a, const Duration &b) {
  return Micros(a * b.inMicros());
}

// All unit combinations share these constrained operators. Mixing a helper or
// full Duration into arithmetic widens the result before the operation.
template <typename A, typename B, internal::EnableDurationPair<A, B> = 0>
inline constexpr bool operator==(const A &a, const B &b) {
  return a.inMicros() == b.inMicros();
}

template <typename A, typename B, internal::EnableDurationPair<A, B> = 0>
inline constexpr bool operator!=(const A &a, const B &b) {
  return a.inMicros() != b.inMicros();
}

template <typename A, typename B, internal::EnableDurationPair<A, B> = 0>
inline constexpr bool operator<(const A &a, const B &b) {
  return a.inMicros() < b.inMicros();
}

template <typename A, typename B, internal::EnableDurationPair<A, B> = 0>
inline constexpr bool operator>(const A &a, const B &b) {
  return a.inMicros() > b.inMicros();
}

template <typename A, typename B, internal::EnableDurationPair<A, B> = 0>
inline constexpr bool operator<=(const A &a, const B &b) {
  return a.inMicros() <= b.inMicros();
}

template <typename A, typename B, internal::EnableDurationPair<A, B> = 0>
inline constexpr bool operator>=(const A &a, const B &b) {
  return a.inMicros() >= b.inMicros();
}

template <typename A, typename B, internal::EnableDurationPair<A, B> = 0>
inline constexpr Duration operator+(const A &a, const B &b) {
  return Micros(a.inMicros() + b.inMicros());
}

template <typename A, typename B, internal::EnableDurationPair<A, B> = 0>
inline constexpr Duration operator-(const A &a, const B &b) {
  return Micros(a.inMicros() - b.inMicros());
}

template <typename D, typename std::enable_if<
                          internal::IsDurationLike<D>::value, int>::type = 0>
inline constexpr Duration operator*(const D &value, int factor) {
  return Micros(value.inMicros() * factor);
}

template <typename D, typename std::enable_if<
                          internal::IsDurationLike<D>::value, int>::type = 0>
inline constexpr Duration operator*(int factor, const D &value) {
  return Micros(value.inMicros() * factor);
}

inline constexpr SmallDuration operator+(SmallDuration a, SmallDuration b) {
  return SmallDuration::Millis(internal::CheckedSmallMillis(
      static_cast<int64_t>(a.inMillis()) + b.inMillis()));
}

inline constexpr SmallDuration operator-(SmallDuration a, SmallDuration b) {
  return SmallDuration::Millis(internal::CheckedSmallMillis(
      static_cast<int64_t>(a.inMillis()) - b.inMillis()));
}

inline constexpr SmallDuration operator*(SmallDuration value, int factor) {
  return SmallDuration::Millis(internal::CheckedSmallMillis(
      static_cast<int64_t>(value.inMillis()) * factor));
}
inline constexpr SmallDuration operator*(int factor, SmallDuration value) {
  return value * factor;
}

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
  static SmallTimestamp Now() { return Uptime::Now(); }

  SmallTimestamp &operator+=(SmallDuration duration) {
    millis_ += static_cast<uint32_t>(duration.inMillis());
    return *this;
  }
  SmallTimestamp &operator-=(SmallDuration duration) {
    millis_ -= static_cast<uint32_t>(duration.inMillis());
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
    assert(delta != 0x80000000u); // Exactly half a cycle is ambiguous.
    const int64_t signed_delta =
        delta <= INT32_MAX ? static_cast<int64_t>(delta)
                           : static_cast<int64_t>(delta) - 0x100000000LL;
    return SmallDuration::Millis(static_cast<int32_t>(signed_delta));
  }

private:
  uint32_t millis_;
};

inline SmallTimestamp operator+(SmallTimestamp t, SmallDuration d) {
  return t += d;
}
inline SmallTimestamp operator+(SmallDuration d, SmallTimestamp t) {
  return t += d;
}
inline SmallTimestamp operator-(SmallTimestamp t, SmallDuration d) {
  return t -= d;
}

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
/// Zero and negative durations are no-ops. Positive waits recheck elapsed uptime
/// and can overshoot due to scheduling. Requires a progressing clock and its
/// concurrency/sampling contracts. Call in task/loop context, not from an ISR.
void Delay(Duration duration);

/// Delays execution until `deadline`.
///
/// If deadline is at or before now, returns immediately. Otherwise returns at
/// or after the deadline, subject to the same backend requirements as Delay.
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
/// Validity and synchronization status must be tracked separately by the caller.
/// Wall time may jump; use Uptime for elapsed measurement and deadlines.
class WallTimeClock {
 public:
  /// Virtual destructor.
  virtual ~WallTimeClock() = default;

  /// Returns current wall time.
  virtual WallTime now() const = 0;
};

#ifdef CTIME_HDR_DEFINED
/// Wall-time clock backed by `gettimeofday`; does not initiate synchronization.
/// Returns the Unix epoch on failure, without a separate error indication.
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

/// Fixed UTC offset in whole signed 16-bit minutes, without DST or zone rules.
/// Supply a representable whole-minute offset; construction is unchecked and
/// truncates sub-minute input toward zero.
class UtcOffset {
 public:
  /// Constructs a zero UTC offset.
  UtcOffset() : offset_minutes_(0) {}

  /// Constructs a fixed UTC offset.
  constexpr explicit UtcOffset(Duration offset)
      : offset_minutes_(offset.inMinutes()) {}

  /// Returns this fixed offset as a duration.
  [[nodiscard]] constexpr Duration offset() const {
    return Minutes(offset_minutes_);
  }

 private:
  int16_t offset_minutes_;
};

/// Deprecated compatibility name. Prefer UtcOffset for fixed UTC offsets.
using TimeZone [[deprecated("Use UtcOffset instead")]] = UtcOffset;

namespace timezone {
constexpr UtcOffset UTC = UtcOffset(Micros(0));
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
/// Supports valid Gregorian dates in years 1-9999; inputs are not validated or
/// normalized. Wall-time conversion must also yield a local date in that range.
/// Does not account for leap seconds.
class DateTime {
 public:
  /// Constructs `DateTime` representing the Unix epoch in UTC.
  DateTime() : DateTime(WallTime(), timezone::UTC) {}

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

  /// Constructs `DateTime` for `wallTime` in time zone `tz`.
  DateTime(WallTime wallTime, UtcOffset tz);

  /// Returns `WallTime` corresponding to this `DateTime`.
  [[nodiscard]] WallTime wallTime() const { return walltime_; }

  /// Returns the fixed UTC offset of this `DateTime`.
  [[nodiscard]] UtcOffset timeZone() const { return tz_; }

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
  WallTime walltime_;
  UtcOffset tz_;
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

} // namespace roo_time

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
