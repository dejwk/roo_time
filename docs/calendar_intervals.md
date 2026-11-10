# Calendar boundaries

Use `roo_time/calendar_interval.h` to enumerate aligned calendar boundaries in
UTC order. Bazel consumers depend on the optional `//:calendar_intervals`
target. The default `//:roo_time` umbrella does not pull in the enumerator.

Choose an interval explicitly; the library does not choose chart density or
format labels. The following complete example counts hourly boundaries around
an autumn clock change. Both readings of 02:00 are marked ambiguous even when
the requested window contains only one of them.

```cpp
#include "roo_time/calendar_interval.h"

using namespace roo_time;

struct Summary {
  unsigned count = 0;
  unsigned repeated = 0;
};

bool VisitBoundary(void* context, const CalendarBoundary& boundary) {
  Summary& summary = *static_cast<Summary*>(context);
  ++summary.count;
  if (boundary.occurrence != LocalOccurrence::kUnique) ++summary.repeated;
  DateTime local(boundary.instant, boundary.offset);
  // Use local fields for presentation; copy boundary to retain its values.
  return true;
}

int main() {
  RecurringTimeZone zone(UtcOffset(Hours(1)), rules::EuSummerTime());
  TimeRange range{DateTime(2026, 10, 25, timezone::UTC).wallTime(),
                  DateTime(2026, 10, 26, timezone::UTC).wallTime()};
  uint32_t work_remaining = 4096;
  Summary summary;
  CalendarStatus status = EnumerateCalendarBoundaries(
      range, zone, {CalendarUnit::kHour, 1}, work_remaining, VisitBoundary,
      &summary);
  return status == CalendarStatus::kOk && summary.count == 24 &&
                 summary.repeated == 2
             ? 0
             : 1;
}
```

Each visit contains an instant, its UTC offset, and `kUnique`, `kEarlier` or
`kLater`. A visitor runs synchronously, may use a null context, and returns false
to stop with `kStopped`. No callback or context is retained. Built-in providers
and the enumerator allocate nothing and use no process-global timezone state.

| Unit | Step | Alignment |
| --- | --- | --- |
| Millisecond | Divisor of 1000, below 1000 | Fraction within each second |
| Second | Divisor of 60, below 60 | Second within each minute |
| Minute | Divisor of 60, below 60 | Minute within each hour |
| Hour | Divisor of 24, below 24 | Hour within each local day |
| Day | 1–65535 | Days from an explicit anchor |
| Week | 1–65535 | Weeks from an anchor normalized backward to the first weekday |
| Month | Divisor of 12, below 12 | Midnight on day 1, `(month - 1) % step == 0` |
| Year | 1–9999 | January 1, `year % step == 0` |

All steps must be positive. `CalendarInterval::Days(step)` uses 1970-01-01 as
its anchor. `CalendarInterval::Weeks(first_weekday, step)` starts from
1970-01-05, normalized backward (Sunday becomes 1970-01-04). Both factories
accept an explicit final anchor argument. Day/week aggregates require a valid
anchor; other units ignore it. Week alignment does not use ISO week numbers.

Months and years use calendar fields; adding an average duration would drift.
Day/week anchors remain stable when the requested viewport moves. Negative
timestamps use floor division. The first boundary is computed directly without
scanning from the epoch.

The requested range is `[begin, end)`. An unset/reversed range, invalid interval
or null visitor returns `kInvalidArgument`. A nonempty range outside the
provider's [calendar range](timezones.md#calendar-ranges-and-offset-spans)
returns `kOutOfRange`; it is not clamped. Empty ranges return `kOk` without
visits or work after validating arguments and querying the provider range.
Fixed and recurring zones support traversal; forward-only providers, including
the current AceTime adapter, return `kUnsupported`. There is no UTC fallback.

Offset spans are traversed in UTC order. Missing times occur in no span, and
repeated readings occur in both spans. Midnight changes and even a skipped
date follow the same rule: no replacement midnight is invented. Use offset
spans directly when contextual date labels need to change at the real offset
transition. Occurrences come from reverse resolution of the entire reading,
independently of viewport edges.

The shared `uint32_t` work balance is decremented before every span query and
candidate examined. A span without boundaries still costs one unit. Querying
the capability/range and computing alignment do not consume units. Exhaustion
returns `kLimitExceeded`, even after partial output; caller-directed stop
returns `kStopped`. All returns preserve the unused balance. Several plans can
share one overall budget. Discard partial plans on errors. A provider violating
range, progress, adjacency or reverse-result invariants returns `kProviderError`.

Traversal uses O(1) working storage and O(S+B) operations for S spans and B
candidates with built-in providers. Custom query costs multiply the relevant
calls. A centuries-long view can cross thousands of spans even with few year
boundaries. Recurring zones apply their configured rules proleptically; they
are not historical named-zone databases. All dates are Gregorian years 1–9999
and use POSIX time without leap seconds.

Measured 32-bit object sizes, stack frames and linked consumer flash deltas are
recorded in [resource acceptance](calendar_interval_resources.md), with a
reproducible measurement script.
