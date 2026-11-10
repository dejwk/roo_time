#include "roo_time/acetime.h"

int main() {
  using namespace roo_time;

  // Verifies the optional adapter maps AceTime's total offset to UtcOffset.
  AceTimeZone zone(ace_time::TimeZone(-19800));
  if (zone.resolveOffset(WallTime::Epoch()).asDuration() != Minutes(-330))
    return 1;
  if (zone.resolveOffset(WallTime::SinceEpoch(Micros(-1))).asDuration() !=
      Minutes(-330))
    return 2;
  // Verifies the adapter explicitly remains forward-only for calendar queries.
  TimeRange range{WallTime::Epoch(), WallTime::Epoch() + Hours(24)};
  OffsetSpan span{range, UtcOffset(Hours(3))};
  if (zone.calendarRange(range) != CalendarStatus::kUnsupported) return 3;
  if (zone.offsetSpanAt(range.begin, span) != CalendarStatus::kUnsupported)
    return 4;
  if (range.begin != WallTime::Epoch() ||
      range.end != WallTime::Epoch() + Hours(24) ||
      span.range.begin != range.begin || span.range.end != range.end ||
      span.offset != UtcOffset(Hours(3)))
    return 5;
  return 0;
}
