#include "roo_time/calendar_interval.h"

#include <cstdint>

#include "roo_time/internal/calendar.h"
#include "roo_time/time_of_day.h"

namespace roo_time {
namespace {

constexpr int64_t kDayMicros = 86400000000LL;

// UTC coordinates for 0001-01-01 and the exclusive end of 9999-12-31.
constexpr int64_t kCalendarBegin = -62135596800000000LL;
constexpr int64_t kCalendarEnd = 253402300800000000LL;

// Division and alignment must floor negative pre-epoch calendar coordinates.
int64_t FloorDiv(int64_t value, int64_t divisor) {
  return value / divisor - (value % divisor < 0 ? 1 : 0);
}

// Returns the first lattice position at or after value, without scanning.
int64_t Align(int64_t value, int64_t origin, int64_t stride) {
  const int64_t before = origin + FloorDiv(value - origin, stride) * stride;
  return before < value ? before + stride : before;
}

// Computes one boundary directly. All intermediates fit int64_t for validated
// calendar coordinates, uint16_t steps and anchors. End is never converted.
int64_t FirstBoundary(int64_t local, CalendarInterval interval) {
  if (local >= kCalendarEnd) return kCalendarEnd;
  int64_t stride;
  switch (interval.unit) {
    case CalendarUnit::kMillisecond:
      stride = 1000;
      break;
    case CalendarUnit::kSecond:
      stride = 1000000;
      break;
    case CalendarUnit::kMinute:
      stride = 60000000;
      break;
    case CalendarUnit::kHour:
      stride = 3600000000LL;
      break;
    case CalendarUnit::kDay:
    case CalendarUnit::kWeek: {
      int64_t anchor_day = interval.anchor - CivilDay::FromYmd(1970, 1, 1);
      stride = kDayMicros;
      if (interval.unit == CalendarUnit::kWeek) {
        anchor_day -=
            (interval.anchor.dayOfWeek() - interval.first_weekday + 7) % 7;
        stride *= 7;
      }
      return Align(local, anchor_day * kDayMicros, stride * interval.step);
    }
    case CalendarUnit::kMonth:
    case CalendarUnit::kYear: {
      int16_t year;
      uint8_t month;
      uint8_t day;
      internal::CivilFromDays(static_cast<int32_t>(FloorDiv(local, kDayMicros)),
                              &year, &month, &day);
      const int index = interval.unit == CalendarUnit::kMonth
                            ? static_cast<int>(year) * 12 + month - 1
                            : year;
      int aligned = index / interval.step * interval.step;
      // Year zero is an alignment coordinate, never a converted calendar date.
      int y = interval.unit == CalendarUnit::kMonth ? aligned / 12 : aligned;
      int m = interval.unit == CalendarUnit::kMonth ? aligned % 12 + 1 : 1;
      if (y >= 1) {
        const int64_t first =
            static_cast<int64_t>(internal::DaysFromCivil(y, m, 1)) * kDayMicros;
        if (first >= local) return first;
      }
      aligned += interval.step;
      y = interval.unit == CalendarUnit::kMonth ? aligned / 12 : aligned;
      m = interval.unit == CalendarUnit::kMonth ? aligned % 12 + 1 : 1;
      if (y > 9999) return kCalendarEnd;
      return static_cast<int64_t>(internal::DaysFromCivil(y, m, 1)) *
             kDayMicros;
    }
    default:
      return kCalendarEnd;  // Rejected before traversal.
  }
  return Align(local, 0, stride * interval.step);
}

// Validates all reverse-result invariants, including the occurrence outside the
// viewport. A provider's supported-range edge may legitimately be unique.
CalendarStatus Classify(const TimeZone& zone, int64_t local, WallTime instant,
                        LocalOccurrence& occurrence) {
  // Decode only the fields needed by reverse lookup. A DateTime would also
  // retain a redundant instant, offset and derived fields on the stack.
  const int32_t epoch_day = static_cast<int32_t>(FloorDiv(local, kDayMicros));
  int16_t year;
  uint8_t month;
  uint8_t day;
  internal::CivilFromDays(epoch_day, &year, &month, &day);
  const LocalTimeResolution reverse = zone.resolveLocal(
      CivilDay::FromYmd(year, month, day),
      TimeOfDay::FromSinceMidnight(
          Micros(local - static_cast<int64_t>(epoch_day) * kDayMicros)));
  if (reverse.status == LocalTimeStatus::kUnsupported) {
    return CalendarStatus::kUnsupported;
  }
  if (reverse.status == LocalTimeStatus::kUnique && reverse.first == instant &&
      !reverse.second.isSet()) {
    occurrence = LocalOccurrence::kUnique;
    return CalendarStatus::kOk;
  }
  if (reverse.status == LocalTimeStatus::kAmbiguous && reverse.first.isSet() &&
      reverse.second.isSet() && reverse.first < reverse.second) {
    if (reverse.first == instant) {
      occurrence = LocalOccurrence::kEarlier;
      return CalendarStatus::kOk;
    }
    if (reverse.second == instant) {
      occurrence = LocalOccurrence::kLater;
      return CalendarStatus::kOk;
    }
  }
  return CalendarStatus::kProviderError;
}

// Enumerates one clipped span in local coordinates, where UTC order is the
// same as local order. Bounds are checked before offset arithmetic/conversion.
CalendarStatus VisitSpan(TimeRange range, UtcOffset offset,
                         const TimeZone& zone, CalendarInterval interval,
                         uint32_t& work, BoundaryVisitor visitor,
                         void* context) {
  const int64_t shift = offset.inMicros();
  const int64_t begin = range.begin.sinceEpoch().inMicros();
  const int64_t end = range.end.sinceEpoch().inMicros();
  if (begin < kCalendarBegin - shift || end > kCalendarEnd - shift) {
    return CalendarStatus::kProviderError;
  }
  const int64_t local_end = end + shift;
  int64_t local = FirstBoundary(begin + shift, interval);
  while (local < local_end) {
    if (work == 0) return CalendarStatus::kLimitExceeded;
    --work;
    CalendarBoundary boundary{WallTime::SinceEpoch(Micros(local - shift)),
                              offset, LocalOccurrence::kUnique};
    const CalendarStatus status =
        Classify(zone, local, boundary.instant, boundary.occurrence);
    if (status != CalendarStatus::kOk) return status;
    if (!visitor(context, boundary)) return CalendarStatus::kStopped;
    local = FirstBoundary(local + 1, interval);
  }
  return CalendarStatus::kOk;
}

}  // namespace

CalendarInterval CalendarInterval::Days(uint16_t step, CivilDay anchor) {
  return {CalendarUnit::kDay, step, kMonday, anchor};
}

CalendarInterval CalendarInterval::Weeks(DayOfWeek first_weekday, uint16_t step,
                                         CivilDay anchor) {
  return {CalendarUnit::kWeek, step, first_weekday, anchor};
}

bool CalendarInterval::isValid() const {
  if (step == 0) return false;
  switch (unit) {
    case CalendarUnit::kMillisecond:
      return step < 1000 && 1000 % step == 0;
    case CalendarUnit::kSecond:
    case CalendarUnit::kMinute:
      return step < 60 && 60 % step == 0;
    case CalendarUnit::kHour:
      return step < 24 && 24 % step == 0;
    case CalendarUnit::kDay:
      return anchor.isValid();
    case CalendarUnit::kWeek:
      return anchor.isValid() && first_weekday >= kSunday &&
             first_weekday <= kSaturday;
    case CalendarUnit::kMonth:
      return step < 12 && 12 % step == 0;
    case CalendarUnit::kYear:
      return step <= 9999;
  }
  return false;
}

CalendarStatus EnumerateCalendarBoundaries(
    TimeRange range, const TimeZone& zone, CalendarInterval interval,
    uint32_t& work_remaining, BoundaryVisitor visitor, void* context) {
  if (!range.begin.isSet() || !range.end.isSet() || range.end < range.begin ||
      !interval.isValid() || visitor == nullptr) {
    return CalendarStatus::kInvalidArgument;
  }
  TimeRange supported{WallTime::Unset(), WallTime::Unset()};
  CalendarStatus status = zone.calendarRange(supported);
  if (status != CalendarStatus::kOk) {
    return status == CalendarStatus::kUnsupported
               ? status
               : CalendarStatus::kProviderError;
  }
  if (!supported.begin.isSet() || !supported.end.isSet() ||
      supported.begin >= supported.end ||
      supported.begin.sinceEpoch().inMicros() < kCalendarBegin ||
      supported.end.sinceEpoch().inMicros() > kCalendarEnd) {
    return CalendarStatus::kProviderError;
  }
  if (range.begin == range.end) return CalendarStatus::kOk;
  if (range.begin < supported.begin || range.end > supported.end) {
    return CalendarStatus::kOutOfRange;
  }

  WallTime cursor = range.begin;
  UtcOffset previous;
  bool first_span = true;
  while (cursor < range.end) {
    if (work_remaining == 0) return CalendarStatus::kLimitExceeded;
    --work_remaining;
    OffsetSpan span{{WallTime::Unset(), WallTime::Unset()}, timezone::UTC};
    status = zone.offsetSpanAt(cursor, span);
    if (status != CalendarStatus::kOk) {
      return status == CalendarStatus::kUnsupported
                 ? status
                 : CalendarStatus::kProviderError;
    }
    if (!span.range.begin.isSet() || !span.range.end.isSet() ||
        span.range.begin < supported.begin || span.range.end > supported.end ||
        span.range.begin > cursor || span.range.end <= cursor ||
        (!first_span &&
         (span.range.begin != cursor || span.offset == previous))) {
      return CalendarStatus::kProviderError;
    }
    const WallTime end =
        span.range.end < range.end ? span.range.end : range.end;
    status = VisitSpan({cursor, end}, span.offset, zone, interval,
                       work_remaining, visitor, context);
    if (status != CalendarStatus::kOk) return status;
    cursor = end;
    previous = span.offset;
    first_span = false;
  }
  return CalendarStatus::kOk;
}

}  // namespace roo_time
