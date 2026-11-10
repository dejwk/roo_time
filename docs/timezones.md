# Time zones and local schedules

Use `ToLocal` to display an instant in a zone. Use `ResolveLocal` to turn a
civil date and time into possible UTC instants. The latter needs a policy
when the clock repeats or skips a local time.

For daily schedules, advance the civil date with `addDays(1)` and resolve the
next local occurrence. Adding `Hours(24)` advances elapsed time, which can
land on a different local hour across an offset transition.

[Back to the overview](../README.md) · [Runnable examples](../examples/README.md)

## Time zones and seasonal rules

Include `roo_time/timezone.h` for the timezone API. Core value types, including
`UtcOffset` and `DateTime`, are declared in `roo_time/wall_time.h` and remain
available through `roo_time.h`. `roo_time/format.h` includes the timezone header
for its timezone-aware overloads.

`TimeZone::resolveOffset(WallTime) const` returns the **total** local-minus-UTC
offset as a `UtcOffset`. The instant must be within the implementation's
documented range; behavior is undefined otherwise. Implementations do not read the clock
or modify global timezone settings, and must permit concurrent const queries.
The new offset applies exactly at a transition: intervals are `[start, end)`.
There is no heap allocation or mutable shared cache in the built-in resolvers.

```cpp
#include "roo_time/timezone.h"
#include "roo_time/format.h"
using namespace roo_time;

// Configure a recurring rule with a standard offset.
RecurringTimeZone local_zone(UtcOffset(Hours(1)), rules::EuSummerTime());
FixedTimeZone india(UtcOffset(Minutes(330)));

// Resolve an instant once into a snapshot containing fields and a fixed offset.
WallTime instant = DateTime(2026, 7, 1, timezone::UTC).wallTime();
DateTime local = ToLocal(instant, local_zone);
int hour = local.hour();  // 2
// local retains no reference to local_zone.
char text[kIsoDateTimeBufferSize];
FormatResult result = FormatIsoDateTime(instant, local_zone, text, sizeof(text));
// 2026-07-01T02:00:00.000000+02:00
```

`FixedTimeZone` accepts INT64_MIN + 1 through INT64_MAX microseconds since the
Unix epoch, inclusive; the unset sentinel is excluded. Every representable `UtcOffset` is valid for its constructor.
`RecurringTimeZone` accepts UTC instants from 0001-01-01T00:00:00.000000Z through
9999-12-31T23:59:59.999999Z, inclusive. Out-of-range calls have undefined behavior
and assert in debug builds. `ToLocal(instant, zone)` returns a `DateTime` directly.
It requires the same input range and a resolved local date from
0001-01-01T00:00:00.000000 through 9999-12-31T23:59:59.999999, inclusive. Violating
either precondition has undefined behavior; the local-date bounds assert in debug builds.
`DateTime` remains a fixed-offset snapshot: after shifting an instant across a
transition, resolve it again. For the reverse direction, see
[resolving local dates and times](#resolving-local-dates-and-times). Parsing
continues to use an explicit fixed offset; it does not infer a zone or choose
between duplicated/nonexistent local times.

`FormatDateTime(instant, zone, ...)` and `FormatIsoDateTime(instant, zone, ...)`
have buffer and optional `std::string` overloads, plus the existing Arduino
`...Arduino` forms. They resolve once per call, including allocating forms that
measure and then write. Numeric offset directives use the snapshot's offset.
Timezone-aware formatting has the same instant and local-date preconditions as
`ToLocal()`. Violating them has undefined behavior. Other formatting and buffer
contracts still apply. No abbreviations or geographic names are inferred from
an offset.

A recurring configuration consists of a base offset, a signed whole-minute
seasonal adjustment stored as `UtcOffset`, and two `AnnualTransition` values. Each transition has:

- A date: `AnnualDateRule::OnDay(month, day)`, `NthWeekday(month, weekday, n)`
  for n=1–4, `LastWeekday(month, weekday)`, or
  `WeekdayOnOrAfter(month, weekday, day)`.
- `uint16_t minutes_since_midnight`, in 0–1440 inclusive. 1440 means midnight
  **after** the selected date, even when that crosses a month or year boundary.
- A `TransitionTimeBasis`: `kUtc`, `kBaseLocal`, or `kAdjustedLocal`.
  Local transition fields are converted with the specified offset, never by
  recursively querying the zone.

Selectors must exist within their month in every year. Thus February 29,
fifth-weekday selectors, and on-or-after anchors that could leave the month are
invalid. `AnnualDateRule::isValid()` checks this; `resolveDay(year, output)`
returns false without changing output for invalid rules or years.

```cpp
SeasonalRules custom = {
    {AnnualDateRule::WeekdayOnOrAfter(kMarch, kFriday, 23), 120,
     TransitionTimeBasis::kBaseLocal},
    {AnnualDateRule::LastWeekday(kOctober, kThursday), 1440,
     TransitionTimeBasis::kAdjustedLocal},
    UtcOffset(Minutes(30))};
RecurringTimeZone zone(UtcOffset(Hours(2)), custom);
// Invalid structural configuration asserts during construction.
```

Construction asserts inexpensive structural preconditions: valid selectors,
transition times/bases and representable total offsets. `UtcOffset` stores the
adjustment in whole minutes. Invalid construction has undefined behavior when assertions are disabled.
There is no invalid-object state or `RecurringTimeZone::isValid()` accessor.
For every year 1–9999, after conversion using the zone's offsets,
`first_transition` must strictly precede `second_transition`, and both instants
must fall within that same UTC calendar year. These requirements are the caller's
responsibility; violations have undefined behavior and are not checked.
Resolution evaluates just these two transitions for the query's UTC year.
Negative and zero adjustments are permitted. January, December, and same-month
transitions are permitted if they satisfy the requirements above.

The adjustment applies from `first_transition` (inclusive) to
`second_transition` (exclusive); the base offset applies outside that interval.
For southern-hemisphere patterns, use the summer offset as the base and a
negative winter adjustment, so the adjusted interval stays within the year.

Built-in factories describe the following recurring patterns:

| Factory | First / second transition | Adjustment | Base offset |
| --- | --- | --- | --- |
| `rules::EuSummerTime()` | Last Sunday March / October, both 01:00 UTC | +60 min | Standard (winter) offset |
| `rules::UsDstSince2007()` | Second Sunday March 02:00 base / first Sunday November 02:00 adjusted | +60 min | Standard (winter) offset |
| `rules::AustralianEasternDst()` | First Sunday April 03:00 base / first Sunday October 02:00 adjusted | -60 min | +11 hours |
| `rules::NewZealandDst()` | First Sunday April / last Sunday September, both 02:00 adjusted | -60 min | +13 hours for mainland NZ |
| `rules::LordHoweDst()` | First Sunday April 02:00 base / first Sunday October 02:00 adjusted | -30 min | +11 hours |

These patterns are based on the [EU directive](https://eur-lex.europa.eu/legal-content/EN/TXT/PDF/?uri=CELEX%3A32000L0084),
[NIST's US description](https://www.nist.gov/pml/time-and-frequency-division/popular-links/daylight-saving-time-dst),
and [IANA Australasia data](https://data.iana.org/time-zones/tzdb/australasia).
They are applied **proleptically** throughout the supported range, including
before the named pattern was adopted. They do not promise historical accuracy,
cover every place in a named region, or update automatically with legislation.
Choose the appropriate base offset separately. No geographic database,
lunar-calendar rules, one-off exceptions, or more than two annual transitions
are included.

For custom rules, implement the single virtual `resolveOffset` method. It must
document its minimum and maximum accepted instants and return an offset for
every instant within that range.

For applications that need advanced timezone support—such as a comprehensive
named-zone database, historically accurate transition data, or exceptions that
cannot be expressed by two recurring annual transitions—use
[AceTime](https://github.com/bxparks/AceTime). `roo_time/acetime.h` is an
optional header-only adapter for its `ace_time::TimeZone`. It is not included by
`roo_time/timezone.h` and the normal `roo_time` Bazel target has no AceTime
dependency. Include it only after AceTime is available through your build:

```cpp
#include <AceTime.h>
#include "roo_time/acetime.h"

ace_time::TimeZone ace_zone = /* created with an AceTime manager or processor */;
roo_time::AceTimeZone zone(ace_zone);
```

The adapter preserves AceTime's supported instant range and requires its
resolved total offset to be a whole number of minutes.

## Resolving local dates and times

`ResolveLocal(date, time, zone)` combines a `CivilDay`, `TimeOfDay`, and timezone
without assuming how to handle daylight-saving gaps or repeated times:

```cpp
RecurringTimeZone zone(UtcOffset(Hours(1)), rules::EuSummerTime());
LocalTimeResolution repeated = ResolveLocal(CivilDay::FromYmd(2026, 10, 25),
                                             TimeOfDay::FromHms(2, 30), zone);
// repeated.status == LocalTimeStatus::kAmbiguous
// repeated.first  == 2026-10-25T00:30:00Z
// repeated.second == 2026-10-25T01:30:00Z
LocalTimeResolution missing = ResolveLocal(CivilDay::FromYmd(2026, 3, 29),
                                            TimeOfDay::FromHms(2, 30), zone);
// missing.status == LocalTimeStatus::kNonexistent; both instants are unset.
```

| Status | Result |
| --- | --- |
| `kUnique` | Only `first` is set |
| `kAmbiguous` | Both are set, with `first < second` |
| `kNonexistent` | The clock skipped this local time; both are unset |
| `kInvalidInput` | Invalid civil date or time of day; both are unset |
| `kOutOfRange` | No match, and a candidate is outside the zone's UTC range |
| `kUnsupported` | The zone does not implement reverse resolution |

`FixedTimeZone` resolves every valid local date/time uniquely. Its UTC result
can lie before year 1 or after year 9999 when the fixed offset shifts a local
calendar endpoint; this is within its supported `WallTime` range.
`RecurringTimeZone` tries its two possible offsets, checks the candidate's UTC
range, then validates its actual offset. It handles positive, negative, zero,
and fractional-hour adjustments, microseconds, and year crossings without
allocation or clock reads. Matches are limited to UTC years 1-9999. One match
in that range is unique even if another candidate is out of range. With no
matches, an out-of-range candidate gives `kOutOfRange`; otherwise the local time
is nonexistent. Existing recurring-rule construction preconditions still apply.

No overlap occurrence is selected implicitly and no gap time is shifted.
The caller decides whether to choose an occurrence, skip an event, or apply a
separate scheduling policy. Existing custom `TimeZone` classes and the optional
AceTime adapter remain forward-only by default: the new virtual `resolveLocal`
returns `kUnsupported` for valid input and `kInvalidInput` for invalid input.
Override it to provide reverse resolution, preserving the result invariants
and concurrent-const-call contract. The free function validates inputs before
dispatch; direct built-in method calls also validate inputs. Default-constructed
`LocalTimeResolution` is unsupported with both instants unset.

## Calendar ranges and offset spans

`calendarRange` and `offsetSpanAt` are optional queries. Their default returns
`CalendarStatus::kUnsupported`, preserving output arguments. Calendar traversal
requires both queries and reverse resolution; forward-only subclasses continue
to work with formatting. AceTime currently has no calendar traversal capability.

A calendar range is a nonempty `[begin, end)` UTC interval where UTC and local
calendar fields are representable. The exclusive end is an arithmetic sentinel:
convert `end - Micros(1)`, never `end`. This range can be narrower than the
provider's forward-lookup range. Failures leave query outputs unchanged.

```cpp
FixedTimeZone india(UtcOffset(Minutes(330)));
TimeRange range{WallTime::Unset(), WallTime::Unset()};
if (india.calendarRange(range) == CalendarStatus::kOk) {
  OffsetSpan span{range, timezone::UTC};
  CalendarStatus status = india.offsetSpanAt(range.begin, span);
  // status == kOk; one maximal span covers range with offset +05:30.
}
```

Fixed zones publish `[max(L, L - offset), min(U, U - offset))`, where `L` is
UTC 0001-01-01 and `U` is the arithmetic instant following UTC 9999-12-31.
Unset span queries return `kInvalidArgument`; queries outside the range return
`kOutOfRange`. The forward and reverse lookup contracts above are unchanged.
