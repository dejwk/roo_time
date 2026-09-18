#pragma once

#include <cassert>
#include <cstdint>

namespace roo_time {

/// Weekday numbering, Sunday through Saturday.
enum DayOfWeek {
  kSunday = 0,
  kMonday = 1,
  kTuesday = 2,
  kWednesday = 3,
  kThursday = 4,
  kFriday = 5,
  kSaturday = 6
};

/// Gregorian month numbering, January through December.
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

/// Returns whether a Gregorian year is leap. Requires a year in [1, 9999].
bool IsLeapYear(int32_t year);

/// Returns the month length. Requires a year in [1, 9999] and month in [1, 12].
uint8_t DaysInMonth(int32_t year, Month month);

/// A Gregorian date without time of day or timezone, stored in four bytes.
/// Supports years 1–9999 and an invalid value. No allocation or normalization.
class CivilDay {
 public:
  /// Constructs an invalid date.
  constexpr CivilDay() : days_since_epoch_(INT32_MIN) {}

  /// Returns the invalid value, also used for an absent date.
  static constexpr CivilDay Invalid() { return CivilDay(); }

  /// Constructs a date, returning invalid for any out-of-range component.
  static CivilDay FromYmd(int32_t year, int32_t month, int32_t day);

  /// Returns whether this value represents a date.
  constexpr bool isValid() const { return days_since_epoch_ != INT32_MIN; }

  /// Returns year in [1, 9999]. Requires a valid date.
  int16_t year() const;

  /// Returns month in [1, 12]. Requires a valid date.
  Month month() const;

  /// Returns day of month. Requires a valid date.
  uint8_t day() const;

  /// Returns weekday. Requires a valid date.
  DayOfWeek dayOfWeek() const;

  /// Adds calendar days. Invalid input or an out-of-range result yields
  /// invalid.
  CivilDay addDays(int32_t days) const;

  /// Returns equality; two invalid dates compare equal.
  friend constexpr bool operator==(CivilDay a, CivilDay b) {
    return a.days_since_epoch_ == b.days_since_epoch_;
  }

  /// Returns inequality, including valid versus invalid.
  friend constexpr bool operator!=(CivilDay a, CivilDay b) { return !(a == b); }

  /// Returns chronological order. Both dates must be valid.
  friend bool operator<(CivilDay a, CivilDay b) {
    assert(a.isValid() && b.isValid());
    return a.days_since_epoch_ < b.days_since_epoch_;
  }

  /// Returns reverse chronological order. Both dates must be valid.
  friend bool operator>(CivilDay a, CivilDay b) { return b < a; }

  /// Returns non-strict chronological order. Both dates must be valid.
  friend bool operator<=(CivilDay a, CivilDay b) { return !(b < a); }

  /// Returns non-strict reverse order. Both dates must be valid.
  friend bool operator>=(CivilDay a, CivilDay b) { return !(a < b); }

 private:
  explicit constexpr CivilDay(int32_t days) : days_since_epoch_(days) {}

  int32_t days_since_epoch_;
};

}  // namespace roo_time
