#ifndef ROO_TIME_CALENDAR_INTERVAL_H_
#define ROO_TIME_CALENDAR_INTERVAL_H_

#include <cstdint>

#include "roo_time/civil_day.h"
#include "roo_time/timezone.h"
#include "roo_time/wall_time.h"

namespace roo_time {

/// Calendar coordinate used to align boundaries in a timezone.
enum class CalendarUnit : uint8_t {
  kMillisecond,
  kSecond,
  kMinute,
  kHour,
  kDay,
  kWeek,
  kMonth,
  kYear
};

/// Calendar alignment, independent of elapsed durations and UI density.
/// Sub-day steps divide their parent field (1000, 60, 60, 24), strictly below
/// that field's size. Months divide 12 and start at day 1 of months 1, 1+step,
/// etc. Years start January 1 with year % step == 0, for steps 1-9999.
/// Day/week steps accept 1-65535 and use a stable explicit anchor. Other units
/// ignore the anchor and weekday. No normalization or allocation.
struct CalendarInterval {
  CalendarUnit unit;
  uint16_t step;
  DayOfWeek first_weekday = kMonday;
  CivilDay anchor;

  /// Creates day alignment relative to @p anchor (default 1970-01-01).
  /// Invalid steps or anchors are rejected by isValid and enumeration.
  static CalendarInterval Days(uint16_t step = 1,
                               CivilDay anchor = CivilDay::FromYmd(1970, 1, 1));

  /// Creates week alignment using @p first_weekday. Moves @p anchor backward
  /// to that weekday, even at the year-1 edge, before applying 7 * step days.
  /// The default anchor is Monday 1970-01-05. These are not ISO week numbers.
  static CalendarInterval Weeks(DayOfWeek first_weekday, uint16_t step = 1,
                                CivilDay anchor = CivilDay::FromYmd(1970, 1,
                                                                    5));

  /// Checks the unit, step and relevant anchor/weekday without changing them.
  bool isValid() const;
};

/// Identifies a reading independently of the enumeration's requested range.
enum class LocalOccurrence : uint8_t { kUnique, kEarlier, kLater };

/// One aligned UTC instant, its fixed offset and reverse-lookup occurrence.
/// Values passed to the visitor are temporary; copy any values to retain.
struct CalendarBoundary {
  WallTime instant;
  UtcOffset offset;
  LocalOccurrence occurrence;
};

/// Synchronous visitor with caller-owned context; false requests early stop.
using BoundaryVisitor = bool (*)(void*, const CalendarBoundary&);

/// Visits calendar boundaries in strictly increasing UTC order in @p range.
/// Repeated readings produce both instants; gaps produce none. Requires a
/// nonnull @p visitor; @p context may be null. No allocation or retained state.
/// Validates arguments and provider range before traversal. Empty ranges need
/// no work; nonempty ranges must fit entirely within the provider range.
/// Decrements @p work_remaining per span query and candidate examined,
/// retaining its unused balance on every return. Exhaustion returns
/// kLimitExceeded; visitor stop returns kStopped. Errors after visits leave
/// partial results. Unsupported providers return kUnsupported without a UTC
/// fallback. Broken range/span/reverse-result contracts return kProviderError.
/// Cost is O(S+B) with O(1) storage for S spans and B candidates, plus provider
/// query costs.
CalendarStatus EnumerateCalendarBoundaries(
    TimeRange range, const TimeZone& zone, CalendarInterval interval,
    uint32_t& work_remaining, BoundaryVisitor visitor, void* context);

}  // namespace roo_time

#endif  // ROO_TIME_CALENDAR_INTERVAL_H_
