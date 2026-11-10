// This consumer also compiles against the pre-calendar source for comparisons.
#if defined(ROO_TIME_CALENDAR_ENUMERATION)
#include "roo_time/calendar_interval.h"
#else
#include "roo_time/timezone.h"
#endif

namespace roo_time {
namespace {
volatile int result;

#if defined(ROO_TIME_CALENDAR_ENUMERATION)
bool Count(void*, const CalendarBoundary& boundary) {
  result += boundary.offset.inMinutes();
  return true;
}
#endif

// Runs the same forward-only or calendar consumer for linked-size comparisons.
int RunResourceProbe() {
#if defined(ROO_TIME_CALENDAR_RECURRING)
  RecurringTimeZone zone(UtcOffset(Hours(1)), rules::EuSummerTime());
#else
  FixedTimeZone zone(UtcOffset(Minutes(345)));
#endif
  // Preserve virtual dispatch so range/span additions to vtables are measured
  // even in the forward-only consumer, without LTO or artificial code
  // retention.
  const TimeZone* volatile selected = &zone;
  const WallTime begin = DateTime(2026, 10, 25, timezone::UTC).wallTime();
  result = selected->resolveOffset(begin).inMinutes();
#if defined(ROO_TIME_CALENDAR_ENUMERATION)
  uint32_t work = 4096;
  result += static_cast<int>(EnumerateCalendarBoundaries(
      {begin, begin + Hours(24)}, *selected, {CalendarUnit::kHour, 1}, work,
      Count, nullptr));
#endif
  return result;
}

}  // namespace
}  // namespace roo_time

int main() { return roo_time::RunResourceProbe(); }
