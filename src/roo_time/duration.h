#pragma once

#include <inttypes.h>

#include <cassert>
#include <type_traits>

/// Convenience classes for handling delays and elapsed time measurement.
///
/// Helps avoid common mistakes such as mixing time units or confusing
/// timestamps with durations.
namespace roo_time {

class Duration;
class SmallDuration;
template <typename Rep,
          typename std::enable_if<std::is_integral<Rep>::value, int>::type = 0>
constexpr Duration Micros(Rep count);

namespace internal {
// Empty CRTP base: one implementation of all read-only unit conversions, with
// no storage or virtual dispatch. Derived types provide inMicros().
template <typename Derived>
class DurationConversions {
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

  /// Returns duration in milliseconds, rounded toward zero. For non-negative
  /// durations, this is equivalent to inMillisFloor().
  [[nodiscard]] constexpr int64_t inMillisRoundedDown() const {
    return derived().inMicros() / 1000LL;
  }

  /// Returns duration in seconds, rounded toward zero. For non-negative
  /// durations, this is equivalent to inSecondsFloor().
  [[nodiscard]] constexpr int64_t inSecondsRoundedDown() const {
    return derived().inMicros() / 1000000LL;
  }

  /// Returns duration in minutes, rounded toward zero. For non-negative
  /// durations, this is equivalent to inMinutesFloor().
  [[nodiscard]] constexpr int64_t inMinutesRoundedDown() const {
    return derived().inMicros() / 60000000LL;
  }

  /// Returns duration in hours, rounded toward zero. For non-negative
  /// durations, this is equivalent to inHoursFloor().
  [[nodiscard]] constexpr int64_t inHoursRoundedDown() const {
    return derived().inMicros() / 3600000000LL;
  }

  /// Returns duration in milliseconds, rounded away from zero. For
  /// non-negative durations, this is equivalent to inMillisCeiling().
  [[nodiscard]] constexpr int64_t inMillisRoundedUp() const {
    int64_t q = derived().inMicros() / 1000LL;
    int64_t r = derived().inMicros() % 1000LL;
    if (r == 0) return q;
    return derived().inMicros() > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in seconds, rounded away from zero. For non-negative
  /// durations, this is equivalent to inSecondsCeiling().
  [[nodiscard]] constexpr int64_t inSecondsRoundedUp() const {
    int64_t q = derived().inMicros() / 1000000LL;
    int64_t r = derived().inMicros() % 1000000LL;
    if (r == 0) return q;
    return derived().inMicros() > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in minutes, rounded away from zero. For non-negative
  /// durations, this is equivalent to inMinutesCeiling().
  [[nodiscard]] constexpr int64_t inMinutesRoundedUp() const {
    int64_t q = derived().inMicros() / 60000000LL;
    int64_t r = derived().inMicros() % 60000000LL;
    if (r == 0) return q;
    return derived().inMicros() > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in hours, rounded away from zero. For non-negative
  /// durations, this is equivalent to inHoursCeiling().
  [[nodiscard]] constexpr int64_t inHoursRoundedUp() const {
    int64_t q = derived().inMicros() / 3600000000LL;
    int64_t r = derived().inMicros() % 3600000000LL;
    if (r == 0) return q;
    return derived().inMicros() > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in milliseconds rounded toward negative infinity. For
  /// non-negative durations, this is equivalent to inMillisRoundedDown().
  [[nodiscard]] constexpr int64_t inMillisFloor() const {
    return FloorInUnits(1000LL);
  }

  /// Returns duration in seconds rounded toward negative infinity. For
  /// non-negative durations, this is equivalent to inSecondsRoundedDown().
  [[nodiscard]] constexpr int64_t inSecondsFloor() const {
    return FloorInUnits(1000000LL);
  }

  /// Returns duration in minutes rounded toward negative infinity. For
  /// non-negative durations, this is equivalent to inMinutesRoundedDown().
  [[nodiscard]] constexpr int64_t inMinutesFloor() const {
    return FloorInUnits(60000000LL);
  }

  /// Returns duration in hours rounded toward negative infinity. For
  /// non-negative durations, this is equivalent to inHoursRoundedDown().
  [[nodiscard]] constexpr int64_t inHoursFloor() const {
    return FloorInUnits(3600000000LL);
  }

  /// Returns duration in milliseconds rounded toward positive infinity. For
  /// non-negative durations, this is equivalent to inMillisRoundedUp().
  [[nodiscard]] constexpr int64_t inMillisCeiling() const {
    return CeilingInUnits(1000LL);
  }

  /// Returns duration in seconds rounded toward positive infinity. For
  /// non-negative durations, this is equivalent to inSecondsRoundedUp().
  [[nodiscard]] constexpr int64_t inSecondsCeiling() const {
    return CeilingInUnits(1000000LL);
  }

  /// Returns duration in minutes rounded toward positive infinity. For
  /// non-negative durations, this is equivalent to inMinutesRoundedUp().
  [[nodiscard]] constexpr int64_t inMinutesCeiling() const {
    return CeilingInUnits(60000000LL);
  }

  /// Returns duration in hours rounded toward positive infinity. For
  /// non-negative durations, this is equivalent to inHoursRoundedUp().
  [[nodiscard]] constexpr int64_t inHoursCeiling() const {
    return CeilingInUnits(3600000000LL);
  }

  /// Returns duration in milliseconds, rounded to nearest (ties away from
  /// zero).
  [[nodiscard]] constexpr int64_t inMillisRoundedNearest() const {
    int64_t q = derived().inMicros() / 1000LL;
    int64_t r = derived().inMicros() % 1000LL;
    int64_t ar = r < 0 ? -r : r;
    if (ar * 2 < 1000LL) return q;
    return derived().inMicros() > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in seconds, rounded to nearest (ties away from zero).
  [[nodiscard]] constexpr int64_t inSecondsRoundedNearest() const {
    int64_t q = derived().inMicros() / 1000000LL;
    int64_t r = derived().inMicros() % 1000000LL;
    int64_t ar = r < 0 ? -r : r;
    if (ar * 2 < 1000000LL) return q;
    return derived().inMicros() > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in minutes, rounded to nearest (ties away from zero).
  [[nodiscard]] constexpr int64_t inMinutesRoundedNearest() const {
    int64_t q = derived().inMicros() / 60000000LL;
    int64_t r = derived().inMicros() % 60000000LL;
    int64_t ar = r < 0 ? -r : r;
    if (ar * 2 < 60000000LL) return q;
    return derived().inMicros() > 0 ? q + 1 : q - 1;
  }

  /// Returns duration in hours, rounded to nearest (ties away from zero).
  [[nodiscard]] constexpr int64_t inHoursRoundedNearest() const {
    int64_t q = derived().inMicros() / 3600000000LL;
    int64_t r = derived().inMicros() % 3600000000LL;
    int64_t ar = r < 0 ? -r : r;
    if (ar * 2 < 3600000000LL) return q;
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
  // Divides a microsecond count into positive-sized units with mathematical
  // floor semantics, despite C++ integer division truncating toward zero.
  constexpr int64_t FloorInUnits(int64_t units) const {
    const int64_t micros = derived().inMicros();
    const int64_t quotient = micros / units;
    return micros % units < 0 ? quotient - 1 : quotient;
  }

  // Divides a microsecond count into positive-sized units with mathematical
  // ceiling semantics.
  constexpr int64_t CeilingInUnits(int64_t units) const {
    const int64_t micros = derived().inMicros();
    const int64_t quotient = micros / units;
    return micros % units > 0 ? quotient + 1 : quotient;
  }

  constexpr const Derived& derived() const {
    return static_cast<const Derived&>(*this);
  }
};

constexpr int32_t CheckedSmallMillis(int64_t millis) {
  assert(millis >= INT32_MIN && millis <= INT32_MAX);
  return static_cast<int32_t>(millis);
}
}  // namespace internal

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
  static constexpr Duration Max() { return Duration(INT64_MAX); }

  /// Returns the minimum representable duration.
  static constexpr Duration Min() { return Duration(INT64_MIN); }

  /// Returns duration in microseconds.
  [[nodiscard]] constexpr int64_t inMicros() const { return micros_; }

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
  /// Magnitudes above 67,108,863 days, 23:59:59.999999 saturate, preserving
  /// sign.
  Components toComponents() const;

  /// Reconstructs duration from normalized components.
  /// Requires hours < 24, minutes/seconds < 60, and micros < 1000000.
  static Duration FromComponents(const Components& components);

 private:
  template <typename Rep,
            typename std::enable_if<std::is_integral<Rep>::value, int>::type>
  friend constexpr Duration Micros(Rep count);

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

namespace internal {
// Check before conversion or multiplication, including unsigned 64-bit inputs.
template <int64_t Unit, typename Rep>
constexpr int64_t ScaleTimeCount(Rep count) {
  static_assert(sizeof(Rep) <= sizeof(int64_t), "At most 64-bit integers");
  if (std::is_signed<Rep>::value) {
    assert(static_cast<int64_t>(count) >= INT64_MIN / Unit &&
           static_cast<int64_t>(count) <= INT64_MAX / Unit);
  } else {
    assert(static_cast<uint64_t>(count) <= uint64_t(INT64_MAX / Unit));
  }
  return static_cast<int64_t>(count) * Unit;
}
}  // namespace internal

/// Constructs a full duration in microseconds.
template <typename Rep,
          typename std::enable_if<std::is_integral<Rep>::value, int>::type>
inline constexpr Duration Micros(Rep count) {
  return Duration(internal::ScaleTimeCount<1>(count));
}

/// Floating microseconds are truncated, preserving the original factory
/// behavior.
template <typename Rep, typename std::enable_if<
                            std::is_floating_point<Rep>::value, int>::type = 0>
inline constexpr Duration Micros(Rep count) {
  return Micros(static_cast<int64_t>(count));
}

/// Constructs a full duration in millis.
template <typename Rep,
          typename std::enable_if<std::is_integral<Rep>::value, int>::type = 0>
inline constexpr Duration Millis(Rep count) {
  return Micros(internal::ScaleTimeCount<1000LL>(count));
}

/// Floating input retains the existing full Duration result type.
inline constexpr Duration Millis(float count) {
  return Duration(static_cast<int64_t>(count * 1000LL));
}

/// Floating input retains the existing full Duration result type.
inline constexpr Duration Millis(double count) {
  return Duration(static_cast<int64_t>(count * 1000LL));
}

/// Constructs a full duration in seconds.
template <typename Rep,
          typename std::enable_if<std::is_integral<Rep>::value, int>::type = 0>
inline constexpr Duration Seconds(Rep count) {
  return Micros(internal::ScaleTimeCount<1000000LL>(count));
}

/// Floating input retains the existing full Duration result type.
inline constexpr Duration Seconds(float count) {
  return Duration(static_cast<int64_t>(count * 1000 * 1000));
}

/// Floating input retains the existing full Duration result type.
inline constexpr Duration Seconds(double count) {
  return Duration(static_cast<int64_t>(count * 1000 * 1000));
}

/// Constructs a full duration in minutes.
template <typename Rep,
          typename std::enable_if<std::is_integral<Rep>::value, int>::type = 0>
inline constexpr Duration Minutes(Rep count) {
  return Micros(internal::ScaleTimeCount<60000000LL>(count));
}

/// Floating input retains the existing full Duration result type.
inline constexpr Duration Minutes(float count) {
  return Duration(static_cast<int64_t>(count * 1000 * 1000 * 60));
}

/// Floating input retains the existing full Duration result type.
inline constexpr Duration Minutes(double count) {
  return Duration(static_cast<int64_t>(count * 1000 * 1000 * 60));
}

/// Constructs a full duration in hours.
template <typename Rep,
          typename std::enable_if<std::is_integral<Rep>::value, int>::type = 0>
inline constexpr Duration Hours(Rep count) {
  return Micros(internal::ScaleTimeCount<3600000000LL>(count));
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

  [[nodiscard]] constexpr int64_t inMicros() const {
    return static_cast<int64_t>(millis_) * 1000;
  }
  [[nodiscard]] constexpr int32_t inMillis() const { return millis_; }
  constexpr operator Duration() const { return roo_time::Micros(inMicros()); }

  Duration::Components toComponents() const {
    return static_cast<Duration>(*this).toComponents();
  }

  SmallDuration& operator+=(SmallDuration other) {
    millis_ = internal::CheckedSmallMillis(static_cast<int64_t>(millis_) +
                                           other.millis_);
    return *this;
  }
  SmallDuration& operator-=(SmallDuration other) {
    millis_ = internal::CheckedSmallMillis(static_cast<int64_t>(millis_) -
                                           other.millis_);
    return *this;
  }

 private:
  constexpr SmallDuration(int32_t millis, int) : millis_(millis) {}
  int32_t millis_;
};

/// Constructs compact millis from an integer; scaled milliseconds must fit
/// int32_t.
template <typename Rep,
          typename std::enable_if<std::is_integral<Rep>::value, int>::type = 0>
inline constexpr SmallDuration SmallMillis(Rep count) {
  return SmallDuration::Millis(
      internal::CheckedSmallMillis(internal::ScaleTimeCount<1>(count)));
}

/// Constructs compact seconds from an integer; scaled milliseconds must fit
/// int32_t.
template <typename Rep,
          typename std::enable_if<std::is_integral<Rep>::value, int>::type = 0>
inline constexpr SmallDuration SmallSeconds(Rep count) {
  return SmallDuration::Millis(
      internal::CheckedSmallMillis(internal::ScaleTimeCount<1000>(count)));
}

/// Constructs compact minutes from an integer; scaled milliseconds must fit
/// int32_t.
template <typename Rep,
          typename std::enable_if<std::is_integral<Rep>::value, int>::type = 0>
inline constexpr SmallDuration SmallMinutes(Rep count) {
  return SmallDuration::Millis(
      internal::CheckedSmallMillis(internal::ScaleTimeCount<60000>(count)));
}

/// Constructs compact hours from an integer; scaled milliseconds must fit
/// int32_t.
template <typename Rep,
          typename std::enable_if<std::is_integral<Rep>::value, int>::type = 0>
inline constexpr SmallDuration SmallHours(Rep count) {
  return SmallDuration::Millis(
      internal::CheckedSmallMillis(internal::ScaleTimeCount<3600000>(count)));
}

/// Returns true if both durations are equal.
inline constexpr bool operator==(const Duration& a, const Duration& b) {
  return a.inMicros() == b.inMicros();
}

/// Returns true if durations differ.
inline constexpr bool operator!=(const Duration& a, const Duration& b) {
  return a.inMicros() != b.inMicros();
}

/// Returns true if `a` is shorter than `b`.
inline constexpr bool operator<(const Duration& a, const Duration& b) {
  return a.inMicros() < b.inMicros();
}

/// Returns true if `a` is longer than `b`.
inline constexpr bool operator>(const Duration& a, const Duration& b) {
  return a.inMicros() > b.inMicros();
}

/// Returns true if `a` is not longer than `b`.
inline constexpr bool operator<=(const Duration& a, const Duration& b) {
  return a.inMicros() <= b.inMicros();
}

/// Returns true if `a` is not shorter than `b`.
inline constexpr bool operator>=(const Duration& a, const Duration& b) {
  return a.inMicros() >= b.inMicros();
}

/// Returns the sum of two durations.
inline constexpr Duration operator+(const Duration& a, const Duration& b) {
  return Micros(a.inMicros() + b.inMicros());
}

/// Returns the difference between two durations.
inline constexpr Duration operator-(const Duration& a, const Duration& b) {
  return Micros(a.inMicros() - b.inMicros());
}

/// Multiplies by an integer factor representable in int64_t. Product must fit.
template <typename Rep,
          typename std::enable_if<std::is_integral<Rep>::value, int>::type = 0>
inline constexpr Duration operator*(Duration value, Rep factor) {
  return Micros(value.inMicros() * internal::ScaleTimeCount<1>(factor));
}

template <typename Rep,
          typename std::enable_if<std::is_integral<Rep>::value, int>::type = 0>
inline constexpr Duration operator*(Rep factor, Duration value) {
  return value * factor;
}

/// Multiplies in the factor's floating-point type, then truncates toward zero
/// to microseconds. Factor and product must be finite; intermediate and final
/// values must be representable. Floating conversion may lose precision.
template <typename Rep, typename std::enable_if<
                            std::is_floating_point<Rep>::value, int>::type = 0>
inline constexpr Duration operator*(Duration value, Rep factor) {
  return Micros(static_cast<int64_t>(value.inMicros() * factor));
}

template <typename Rep, typename std::enable_if<
                            std::is_floating_point<Rep>::value, int>::type = 0>
inline constexpr Duration operator*(Rep factor, Duration value) {
  return value * factor;
}

inline constexpr SmallDuration operator+(SmallDuration a, SmallDuration b) {
  return SmallDuration::Millis(internal::CheckedSmallMillis(
      static_cast<int64_t>(a.inMillis()) + b.inMillis()));
}

inline constexpr SmallDuration operator-(SmallDuration a, SmallDuration b) {
  return SmallDuration::Millis(internal::CheckedSmallMillis(
      static_cast<int64_t>(a.inMillis()) - b.inMillis()));
}

template <typename Rep,
          typename std::enable_if<std::is_integral<Rep>::value, int>::type = 0>
inline constexpr SmallDuration operator*(SmallDuration value, Rep factor) {
  return SmallDuration::Millis(
      internal::CheckedSmallMillis(static_cast<int64_t>(value.inMillis()) *
                                   internal::ScaleTimeCount<1>(factor)));
}
template <typename Rep,
          typename std::enable_if<std::is_integral<Rep>::value, int>::type = 0>
inline constexpr SmallDuration operator*(Rep factor, SmallDuration value) {
  return value * factor;
}

/// Multiplies in the factor's floating-point type, then truncates toward zero
/// to milliseconds. Factor and product must be finite; intermediate and final
/// values must be representable. Floating conversion may lose precision.
template <typename Rep, typename std::enable_if<
                            std::is_floating_point<Rep>::value, int>::type = 0>
inline constexpr SmallDuration operator*(SmallDuration value, Rep factor) {
  return SmallDuration::Millis(internal::CheckedSmallMillis(
      static_cast<int64_t>(value.inMillis() * factor)));
}

template <typename Rep, typename std::enable_if<
                            std::is_floating_point<Rep>::value, int>::type = 0>
inline constexpr SmallDuration operator*(Rep factor, SmallDuration value) {
  return value * factor;
}

}  // namespace roo_time
