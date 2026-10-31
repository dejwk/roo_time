#pragma once

#include "roo_time/duration.h"

namespace roo_time {

/// A local clock reading without a date or timezone, in eight bytes.
/// Valid values span 00:00:00 through 23:59:59.999999. No allocation,
/// normalization, leap seconds, or automatic wrapping at midnight.
class TimeOfDay {
 public:
  /// Constructs an invalid time, also used for an absent value.
  constexpr TimeOfDay() : micros_since_midnight_(-1) {}

  /// Returns an invalid time.
  static constexpr TimeOfDay Invalid() { return TimeOfDay(); }

  /// Returns the start of a local calendar day.
  static constexpr TimeOfDay Midnight() { return TimeOfDay(0); }

  /// Constructs a time after validating all fields before narrowing.
  /// Hours must be 0-23, minutes/seconds 0-59, and micros 0-999999.
  /// Omitted seconds and micros default to zero. Invalid fields yield invalid.
  static constexpr TimeOfDay FromHms(int32_t hour, int32_t minute,
                                     int32_t second = 0, int32_t micros = 0) {
    return hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 ||
                   second > 59 || micros < 0 || micros > 999999
               ? Invalid()
               : TimeOfDay(((int64_t{hour} * 60 + minute) * 60 + second) *
                               1000000LL +
                           micros);
  }

  /// Constructs from a clock position since local midnight.
  /// Negative input or input at/after 24 hours yields invalid; no wrapping.
  static constexpr TimeOfDay FromSinceMidnight(Duration elapsed) {
    return elapsed.inMicros() < 0 || elapsed.inMicros() >= 86400000000LL
               ? Invalid()
               : TimeOfDay(elapsed.inMicros());
  }

  /// Returns whether this value represents a valid clock reading.
  constexpr bool isValid() const { return micros_since_midnight_ >= 0; }

  /// Returns the clock position since local midnight. Requires a valid time.
  /// This is not elapsed real time across timezone offset transitions.
  constexpr Duration sinceMidnight() const {
    assert(isValid());
    return Micros(micros_since_midnight_);
  }

  /// Returns hour in 0-23. Requires a valid time.
  constexpr uint8_t hour() const {
    assert(isValid());
    return micros_since_midnight_ / 3600000000LL;
  }

  /// Returns minute in 0-59. Requires a valid time.
  constexpr uint8_t minute() const {
    assert(isValid());
    return micros_since_midnight_ / 60000000LL % 60;
  }

  /// Returns second in 0-59. Requires a valid time.
  constexpr uint8_t second() const {
    assert(isValid());
    return micros_since_midnight_ / 1000000LL % 60;
  }

  /// Returns the microsecond fraction in 0-999999. Requires a valid time.
  constexpr uint32_t micros() const {
    assert(isValid());
    return micros_since_midnight_ % 1000000LL;
  }

  /// Returns equality; two invalid times compare equal.
  friend constexpr bool operator==(TimeOfDay a, TimeOfDay b) {
    return a.micros_since_midnight_ == b.micros_since_midnight_;
  }

  /// Returns inequality, including valid versus invalid.
  friend constexpr bool operator!=(TimeOfDay a, TimeOfDay b) {
    return !(a == b);
  }

  /// Returns clock order within a day. Both times must be valid.
  friend constexpr bool operator<(TimeOfDay a, TimeOfDay b) {
    assert(a.isValid() && b.isValid());
    return a.micros_since_midnight_ < b.micros_since_midnight_;
  }

  /// Returns reverse clock order. Both times must be valid.
  friend constexpr bool operator>(TimeOfDay a, TimeOfDay b) { return b < a; }

  /// Returns non-strict clock order. Both times must be valid.
  friend constexpr bool operator<=(TimeOfDay a, TimeOfDay b) {
    return !(b < a);
  }

  /// Returns reverse non-strict clock order. Both times must be valid.
  friend constexpr bool operator>=(TimeOfDay a, TimeOfDay b) {
    return !(a < b);
  }

 private:
  explicit constexpr TimeOfDay(int64_t micros)
      : micros_since_midnight_(micros) {}

  int64_t micros_since_midnight_;
};

}  // namespace roo_time
