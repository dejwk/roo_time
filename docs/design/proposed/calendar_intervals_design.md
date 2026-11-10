# Calendar intervals and timezone spans

Status: **T1 implemented; T2–T4 pending.** Written 2026-11-10 against `roo_time`
commit `0f235f6`.

## Objective

Provide bounded, allocation-free enumeration of calendar boundaries as UTC
instants, including across timezone transitions, for charts and other consumers.

## Motivation

A history chart needs marks at local hours and midnights. Adding 24 elapsed
hours does not always reach the next midnight, and repeating a local hour can
produce two valid instants. Each application currently has to reconstruct these
rules. Putting the arithmetic in `roo_time` lets applications choose intervals
without implementing their own timezone traversal.

## Background

[WallTime and DateTime](../../../src/roo_time/wall_time.h) represent instants
and fixed-offset calendar snapshots. [CivilDay](../../../src/roo_time/civil_day.h)
provides Gregorian date arithmetic. [TimeZone](../../../src/roo_time/timezone.h)
provides offset lookup and optional reverse resolution through `resolveLocal`.
The reverse result distinguishes unique, ambiguous, nonexistent, invalid, and
out-of-range local readings.

`FixedTimeZone` and `RecurringTimeZone` implement reverse resolution. The
[AceTime adapter](../../../src/roo_time/acetime.h) currently implements only
forward lookup. Supported instant ranges are documented but not queryable, and
no public API enumerates offset transitions. Existing wall-time arithmetic
uses POSIX-style time; leap seconds are not represented.

This proposal is one part of a collection:

- [Calendar locale data](../../../../roo_locale/docs/design/proposed/calendar_locale_design.md)
  owns text and regional conventions.
- [Chart time axis and execution coordination](../../../../../Projects/smarthome_cwucontrol/frontend/docs/design/proposed/time_axis_design.md)
  owns interval selection, formatting recipes, layout, and integration.

The library remains independent of `roo_locale`, `roo_windows`, fonts, pixels,
monitoring resolutions, and label density.

## Requirements

1. Return strictly increasing UTC instants for calendar-aligned boundaries in a
   caller-specified half-open interval, including both occurrences of repeated
   local times and no occurrence of nonexistent times.
2. Support millisecond through year intervals, explicit week starts and stable
   anchors, without accumulating rounding error.
3. Identify ambiguous readings even when the other occurrence is outside the
   requested interval.
4. Expose a safe conversion range and offset transitions without changing the
   behavior of existing forward-only timezone implementations.
5. Bound work, support early termination, and require no heap allocation or
   process-global timezone state.
6. Return explicit errors for unsupported providers and invalid ranges. Never
   silently replace the requested timezone with UTC.
7. Ship complete support for fixed and recurring-rule zones, with an extension
   contract exercised by a synthetic multi-transition zone.

## Design Overview

A **calendar interval** is a unit, positive step, and alignment rule. It is a
value supplied by the caller, not a choice made by the library.

A **calendar range** is a provider-declared UTC interval within which both UTC
and local Gregorian dates are representable and calendar traversal is supported.
It can be narrower than the provider's existing forward-lookup range.

An **offset span** is a maximal interval of that range over which the UTC offset
is constant. It contains its beginning and excludes its end. A transition
instant belongs to the new span. Maximality matters: a caller can inspect the
following span to find the actual transition instead of polling the clock.

A **calendar boundary** is a UTC instant, its resolved offset, and whether its
local reading is unique, the earlier occurrence, or the later occurrence.
Values are copied into a synchronous visitor; the enumerator retains nothing.

The enumerator visits offset spans in UTC order. Within a span, local calendar
coordinates have a fixed relationship to UTC. It enumerates local boundaries,
maps them back, and retains those inside the span and requested range. This
satisfies ordering and gap/fold requirements without sorting a result array.

For a configured EU-style autumn transition, hourly boundaries include:

| UTC | Local reading | Occurrence |
| --- | --- | --- |
| 2026-10-25 00:00 | 02:00 +02:00 | Earlier |
| 2026-10-25 01:00 | 02:00 +01:00 | Later |
| 2026-10-25 02:00 | 03:00 +01:00 | Unique |

The second row must still be marked ambiguous when enumeration starts at
00:30 UTC. Computing ambiguity from the returned rows would miss it.

## Design Details

### Provider capability and ranges

Add two non-pure virtual queries to `TimeZone`: `calendarRange` and
`offsetSpanAt`. Their defaults return `kUnsupported` and preserve output
arguments. Existing subclasses continue to compile and behave as before.
Opting into calendar enumeration requires these queries and `resolveLocal`.

`calendarRange` returns a nonempty, contiguous `[begin, end)` interval. Every
instant inside it supports forward conversion and reverse resolution of the
resulting fields. `end` is an arithmetic sentinel and must not be converted to
a calendar date. Instants at `end - Micros(1)` are valid. The range excludes
the unset sentinel. Range and span outputs are unchanged on failure.

For fixed and recurring zones, let `L` be UTC 0001-01-01 and `U` the arithmetic
instant after UTC 9999-12-31. Let `o_min` and `o_max` be the minimum and maximum
possible offsets. Publish the conservative safe range
`[max(L, L - o_min), min(U, U - o_max))`. This avoids overflow and guarantees
valid local dates without changing the wider existing `resolveOffset` contract.

`offsetSpanAt(t)` returns the maximal constant-offset span containing `t`,
clipped to the published range. It must make progress: `begin <= t < end`.
Adjacent spans touch without overlap and have distinct offsets. Equal-offset
transitions, including a zero seasonal adjustment, are coalesced.

Fixed zones return one span. Recurring zones reuse the existing annual
transition calculation and inspect the current/adjacent UTC years for the
nearest transitions, with explicit guards at years 1 and 9999. This is constant
work per query. Annual-rule limitations and proleptic behavior remain as
documented in the [timezone guide](../../timezones.md).

The first release leaves `AceTimeZone` on the default unsupported calendar
capability. Its existing forward formatting remains available. Named-zone
calendar traversal is not advertised until its adapter implements range,
spans, and reverse resolution against a real pinned AceTime version.

### Interval alignment

These are arithmetic contracts, independent of UI candidate tables:

| Unit | Accepted step | Alignment |
| --- | --- | --- |
| Millisecond | Positive divisor of 1000, less than 1000 | Microsecond fraction within each second |
| Second | Positive divisor of 60, less than 60 | Second field within each minute |
| Minute | Positive divisor of 60, less than 60 | Minute field within each hour |
| Hour | Positive divisor of 24, less than 24 | Hour field within each local day |
| Day | 1 through 65535 | Civil-day difference from explicit anchor is a multiple of step |
| Week | 1 through 65535 | Anchor moved backward to the supplied first weekday; day difference is a multiple of `7 * step` |
| Month | Positive divisor of 12, less than 12 | Day 1, midnight, with `(month - 1) % step == 0` |
| Year | 1 through 9999 | January 1, midnight, with `year % step == 0` |

The anchor is required only for days/weeks; other units ignore it. A day
interval's default factory uses 1970-01-01. A week factory uses 1970-01-05 and
the caller's first weekday. Normalizing that anchor gives Sunday 1970-01-04 for
a Sunday-start locale. Weeks are not ISO week numbers.

Calendar stepping uses integer field arithmetic. Month advancement uses a
linear month index, and year advancement operates on years. Neither uses an
average duration. Compute the first qualifying boundary directly; do not scan
from the epoch. Negative timestamps use floor division, never truncation
toward zero. Checked arithmetic and year bounds precede narrowing conversions.

### Enumeration and transition behavior

1. Validate arguments and the interval, then query provider capability/range.
   A reversed or unset range is invalid. A valid empty range returns `kOk`
   without visits. A nonempty range outside the provider range returns
   `kOutOfRange`; the enumerator does not clamp it.
2. Request the span containing the current UTC cursor and intersect it with
   the requested range. Charge one work unit for each span query.
3. Enumerate aligned local calendar coordinates in that intersection. Compute
   each UTC candidate using the span offset and retain only candidates in
   the half-open intersection. Charge one work unit per candidate examined.
4. Reverse-resolve each retained local reading. Match the candidate to the
   unique, earlier, or later result. An inconsistent provider or a result
   that does not contain this instant returns `kProviderError`; unsupported
   reverse lookup returns `kUnsupported`.
5. Visit the boundary and advance by the calendar interval. Continue at the
   span end when its local candidate range is exhausted.

Keep all calculations in integer microseconds/calendar fields. At a spring
jump, missing clock boundaries occur in no span. At a backward jump, repeated
boundaries occur in both spans, naturally in UTC order. Half-hour transitions
and fractional-hour offsets require no special branch.

Missing midnight means no midnight boundary; a skipped date has no boundaries.
The caller can use offset spans to update contextual date labels at the real
transition. The library does not manufacture a replacement midnight.

Use `resolveLocal` to classify the entire local reading, independent of the
enumeration viewport. A currently representable result classified unique at
the provider's supported-range edge remains unique under that provider's
documented reverse-resolution contract.

### Work, ownership, and failures

The visitor is a function pointer plus caller-owned context. It runs
synchronously and returns false to stop. There is no retained callback,
allocation, cache, registry, or scheduling state.

Let `S` be the number of spans visited and `B` the calendar candidates examined.
Enumeration costs `O(S + B)` provider/calendar operations for the built-in
constant-cost providers and uses `O(1)` working memory. Custom provider query
costs multiply their respective calls and must be documented. At an hourly
view covering one day, `B` is roughly 24 and `S` is normally 1 or 2. A centuries
view can cross thousands of spans; this is real work even with few year ticks.

The caller supplies a work budget by reference, decremented for every span and
candidate and left with its unused balance on every return. This lets a caller
charge several enumerations to one overall limit without estimating their cost.
Budget exhaustion returns `kLimitExceeded`. It does not return a misleading
success after partial traversal. Results already delivered on an error are
partial; chart consumers discard that candidate plan. Early visitor stop
returns `kStopped` and is intentional success for bounded consumers.

New virtual functions add vtable entries but no per-instance fields. The
enumerator's automatic state is estimated below 256 bytes, excluding the
visitor's own state; measure stack usage during implementation. A boundary is
approximately 16 bytes on the intended 32-bit ABI. There is no full-range
vector. Added flash is measured in the phase gates; no performance gain is
claimed before measurement.

## Proposed API

Names and contracts below are the handoff between implementations. Keep types
in `roo_time`; place interval code in `calendar_interval.h/.cpp` and timezone
query additions in `timezone.h/.cpp`.

```cpp
enum class CalendarStatus : uint8_t {
  kOk, kStopped, kInvalidArgument, kUnsupported,
  kOutOfRange, kLimitExceeded, kProviderError
};

struct TimeRange {
  WallTime begin;
  WallTime end;  // Exclusive.
};

struct OffsetSpan {
  TimeRange range;
  UtcOffset offset;
};

// New non-pure members of TimeZone; no new instance data.
virtual CalendarStatus calendarRange(TimeRange& out) const;
virtual CalendarStatus offsetSpanAt(WallTime instant, OffsetSpan& out) const;

enum class CalendarUnit : uint8_t {
  kMillisecond, kSecond, kMinute, kHour, kDay, kWeek, kMonth, kYear
};

struct CalendarInterval {
  CalendarUnit unit;
  uint16_t step;
  DayOfWeek first_weekday = kMonday;
  CivilDay anchor;  // Used only for day/week intervals.
};

enum class LocalOccurrence : uint8_t { kUnique, kEarlier, kLater };

struct CalendarBoundary {
  WallTime instant;
  UtcOffset offset;
  LocalOccurrence occurrence;
};

using BoundaryVisitor = bool (*)(void*, const CalendarBoundary&);

CalendarStatus EnumerateCalendarBoundaries(
    TimeRange range, const TimeZone& zone, CalendarInterval interval,
    uint32_t& work_remaining, BoundaryVisitor visitor, void* context);
```

`visitor` is nonnull; `context` can be null. An exhausted/zero budget returns
`kLimitExceeded` when work is needed. New functions return `kUnsupported`
until their required capability is implemented; no fake UTC fallback is used.

Representative consumer: create `CalendarInterval{CalendarUnit::kHour, 6}`,
initialize `uint32_t work_remaining = 4096`, enumerate, and construct
`DateTime(boundary.instant, boundary.offset)` inside the visitor. A chart uses
the returned occurrence to qualify an ambiguous label and handles pixel
placement itself. Day/week consumers set the explicit anchor described above.

## Implementation Plan

Authoring references: [repository instructions](../../../AGENTS.md),
[C++ authoring](../../../.github/instructions/general-cpp-code-authoring-instructions.md),
and [design authoring](../../../.github/instructions/general-design-authoring-instructions.md).

### T1 — Provider range and span contracts

Commit: `feat(time): expose calendar ranges and fixed-offset spans`.

Add status/range/span types, default unsupported virtual methods, and complete
fixed-zone behavior. Add tests to `//:timezone_test` for range endpoints,
output preservation, maximal spans, and legacy forward-only subclasses. Update
the timezone guide with an executable-style fixed-zone example. Gate: existing
timezone/local-time tests and the new tests pass; existing subclasses compile
without changes.

### T2 — Recurring transition spans

Commit: `feat(time): enumerate recurring timezone offset spans`.

Implement maximal spans using shared transition calculation, including zero
and negative adjustments and adjacent-year guards. Add span-chain tests for
positive/negative/30-minute adjustments, exact transitions, and calendar range
edges. Extend the guide example. Gate: queries cover the range contiguously,
always progress, and agree with `resolveOffset` on both sides of each boundary.

### T3 — Calendar boundary enumeration

Commit: `feat(time): add bounded calendar interval enumeration`.

Add `//:calendar_intervals`, depending on `//:core`, and
`//:calendar_interval_test`. Implement every interval unit, alignment,
occurrence classification, budgets, and visitor stop. Add a calendar guide
example in the same commit. Cover leap days, unequal months, pre-epoch dates,
week anchors, folded and missing hours/midnights, and a synthetic skipped date.
Gate: exact expected UTC sequences and round-trip invariants pass without heap
allocation. The default umbrella does not force this optional target into
consumers that only need elapsed time.

### T4 — Provider conformance and resource acceptance

Commit: `test(time): validate calendar provider contracts and resource bounds`.

Add synthetic multi-transition and deliberately broken providers, with explicit
unsupported AceTime expectations. Compile the example and measure 32-bit
`sizeof`, compiler stack-usage output, and linked flash delta for fixed and
recurring consumers. Gate: no new timezone instance storage or allocation,
enumerator own stack at most 256 bytes, and budget exhaustion bounds calls as
specified. Replace oversized automatic aggregates with scalar iteration state
when the stack gate fails. Record measured sizes; do not invent a flash target.

## Testing Plan

Run the existing `//:timezone_test`, `//:local_time_test`, `//:civil_day_test`,
`//:format_test`, and `//:acetime_test`, plus the new interval suite and example
compile target. Properties include strict UTC order, unique instant emission,
correct alignment, range containment, safe arithmetic, bounded work, and
unchanged legacy behavior. Resource gates accompany the API tests.

Serialize Bazel build/test/run commands. Preserve `~/.bazelrc` disk cache and
resource limits; all Bazel storage stays on persistent paths under
`/home/dawidk`, never `/tmp`. Use the workspace's stable output root.

## Caveats

### Rejected Alternatives

#### Fixed elapsed durations for all units

Simple and useful for elapsed-time timers, but a day/month calendar sequence
has different semantics. The interval table makes the distinction explicit.

#### Enumerate local readings, append both resolutions, and sort

This can produce correct results with sufficient buffering, but folds require
ordering across multiple readings and request boundaries require careful
padding. Span traversal achieves UTC order with constant memory.

#### Discover transitions by periodically sampling offsets

It avoids a provider API addition but can miss two changes between probes.
Maximal-span queries make completeness a provider contract.

#### Add chart tick selection or localized formatting here

Those features need pixels, fonts, or presentation data. This library exposes
calendar instants so those decisions remain with the consumer.

Recurring rules are not a historical timezone database. Calendar support is
Gregorian, years 1–9999, using the existing minute-granularity offset model.
Consumers must not describe a configured recurring rule as complete named-zone
history.

## Future Work

Implement full calendar capability in the optional AceTime adapter using its
supported named-zone range and transition facilities, including real-library
integration tests. Non-Gregorian calendars and leap-second scales remain
separate projects.
