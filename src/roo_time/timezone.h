#pragma once

#include "roo_time.h"

namespace roo_time {

/// Resolves a UTC instant to its complete local-minus-UTC offset.
/// No clock reads or global timezone changes. The new offset applies exactly
/// at a transition. Implementations document their minimum and maximum accepted
/// instants; passing an instant outside that range has undefined behavior.
/// Const calls must support concurrent use.
class TimeZone {
 public:
  /// Destroys the resolver.
  virtual ~TimeZone() = default;

  /// Returns the total offset. The instant must be within the concrete
  /// implementation's documented range; otherwise behavior is undefined.
  virtual UtcOffset resolveOffset(WallTime instant) const = 0;
};

/// A fixed offset, valid for the entire WallTime range.
class FixedTimeZone final : public TimeZone {
 public:
  /// Constructs a resolver with an unchanging offset. Every UtcOffset value
  /// (signed 16-bit minutes) is valid; no further validation is needed.
  explicit FixedTimeZone(UtcOffset offset) : offset_(offset) {}

  /// Returns the configured offset for every representable WallTime, from
  /// INT64_MIN through INT64_MAX microseconds since the Unix epoch, inclusive.
  UtcOffset resolveOffset(WallTime) const override { return offset_; }

 private:
  UtcOffset offset_;
};

/// Gregorian date selector, required to exist within its month every year.
/// Invalid factory arguments produce an invalid rule, never a normalized date.
class AnnualDateRule {
 public:
  /// Selects a fixed day that exists in the month every year.
  static constexpr AnnualDateRule OnDay(Month month, uint8_t day) {
    return AnnualDateRule(month, kSunday, day, Kind::kDay);
  }

  /// Selects occurrence 1–4 of the weekday in the month.
  static constexpr AnnualDateRule NthWeekday(Month month, DayOfWeek weekday,
                                             uint8_t occurrence) {
    return AnnualDateRule(month, weekday, occurrence, Kind::kNth);
  }

  /// Selects the last occurrence of the weekday in the month.
  static constexpr AnnualDateRule LastWeekday(Month month, DayOfWeek weekday) {
    return AnnualDateRule(month, weekday, 0, Kind::kLast);
  }

  /// Selects the first matching weekday on or after day, within the month.
  static constexpr AnnualDateRule WeekdayOnOrAfter(Month month,
                                                   DayOfWeek weekday,
                                                   uint8_t day) {
    return AnnualDateRule(month, weekday, day, Kind::kOnOrAfter);
  }

  /// Returns whether the selector exists within its month every year.
  bool isValid() const;

  /// Years 1–9999. Leaves result unchanged on failure.
  bool resolveDay(uint16_t year, uint8_t& result) const;

  /// Returns the selected month.
  Month month() const { return month_; }

 private:
  enum class Kind : uint8_t { kDay, kNth, kLast, kOnOrAfter };

  constexpr AnnualDateRule(Month month, DayOfWeek weekday, uint8_t value,
                           Kind kind)
      : month_(month), weekday_(weekday), value_(value), kind_(kind) {}

  Month month_;
  DayOfWeek weekday_;
  uint8_t value_;
  Kind kind_;
};

/// Clock basis used to interpret a transition's calendar fields.
enum class TransitionTimeBasis : uint8_t {
  /// UTC calendar fields.
  kUtc,

  /// Local calendar fields using the base offset.
  kBaseLocal,

  /// Local calendar fields using the base offset plus the adjustment.
  kAdjustedLocal,
};

/// Annual date and minute at which an offset transition occurs.
struct AnnualTransition {
  AnnualDateRule date;

  // 0–1440 inclusive; 1440 is midnight AFTER the selected date.
  uint16_t minutes_since_midnight;

  TransitionTimeBasis basis;
};

/// A pair of annual transitions and their seasonal offset adjustment.
/// With the zone's offsets applied, first_transition must strictly precede
/// second_transition, and both UTC instants must lie within the calendar year
/// used to evaluate the rules. These requirements must hold for every year
/// 1–9999. The caller is responsible for satisfying them; violations have
/// undefined behavior and are not checked.
struct SeasonalRules {
  /// Starts the interval using the base offset plus adjustment.
  AnnualTransition first_transition;

  /// Ends the adjusted interval and restores the base offset.
  AnnualTransition second_transition;

  /// Signed, whole-minute adjustment; base plus adjustment must fit UtcOffset.
  UtcOffset adjustment;
};

/// One adjusted interval within each UTC Gregorian year. Rules are applied
/// proleptically, without historical exceptions or geographic lookup.
/// Construction asserts field validity and offset representability, without
/// validating the SeasonalRules ordering and year-boundary preconditions.
/// Invalid construction has undefined behavior when assertions are disabled.
/// Queries allocate nothing and use no mutable shared state.
class RecurringTimeZone final : public TimeZone {
 public:
  /// Requires valid selectors, transition minutes/bases, and a representable
  /// total offset. Asserts these preconditions.
  /// The base offset applies outside the interval between the transitions.
  /// The caller must also satisfy the unchecked SeasonalRules preconditions.
  RecurringTimeZone(UtcOffset base_offset, SeasonalRules rules);

  /// Returns the offset for UTC instants from 0001-01-01T00:00:00.000000Z
  /// through 9999-12-31T23:59:59.999999Z, inclusive. Behavior is undefined
  /// outside this range (asserted in debug builds).
  UtcOffset resolveOffset(WallTime instant) const override;

 private:
  UtcOffset base_offset_;
  SeasonalRules rules_;
};

namespace rules {

// These are recurring patterns, not historical/geographic zone databases.
/// Returns the EU summer-time pattern, with UTC-based transitions.
/// Use the standard (winter) offset as the base offset.
SeasonalRules EuSummerTime();

/// Returns the US DST pattern introduced in 2007, applied proleptically.
/// Use the standard (winter) offset as the base offset.
SeasonalRules UsDstSince2007();

/// Returns the Australian eastern pattern as a -1-hour winter adjustment.
/// Use the summer offset (+11 hours) as the base offset.
SeasonalRules AustralianEasternDst();

/// Returns the New Zealand pattern as a -1-hour winter adjustment.
/// Use the summer offset (+13 hours for mainland NZ) as the base offset.
SeasonalRules NewZealandDst();

/// Returns the Lord Howe pattern as a -30-minute winter adjustment.
/// Use the summer offset (+11 hours) as the base offset.
SeasonalRules LordHoweDst();

}  // namespace rules

/// Resolves once into a fixed-offset snapshot, retaining no zone pointer.
/// The instant must be within the zone's documented range; otherwise behavior
/// is undefined. The resulting local date must also lie within
/// 0001-01-01T00:00:00.000000 through 9999-12-31T23:59:59.999999, inclusive;
/// otherwise behavior is undefined (asserted in debug builds).
DateTime ToLocal(WallTime instant, const TimeZone& zone);

}  // namespace roo_time
