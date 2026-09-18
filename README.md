# roo_time

`roo_time` provides time types for microcontroller C++ code: durations, elapsed
time, wall-clock timestamps, and calendar dates. It makes units explicit and
keeps time points separate from durations, so common mistakes become compiler
errors.

```cpp
#include "roo_time.h"

using namespace roo_time;

// Measure elapsed time.
Uptime start = Uptime::Now();
Delay(Millis(25));
Duration elapsed = Uptime::Now() - start;
int64_t elapsed_ms = elapsed.inMillis();

// Express a deadline with an explicit unit.
Uptime deadline = Uptime::Now() + Seconds(2);
DelayUntil(deadline);

// start + 20;          // Compile error: which unit?
// start + deadline;    // Compile error: cannot add two time points.
```

Duration, uptime, and wall-time types store signed 64-bit microseconds.
`CivilDay` stores a Gregorian date in four bytes. Their value operations use no
heap allocation. The primary target is ESP32 with Arduino or ESP-IDF; Pico SDK and compatible
RP2040 Arduino cores, native Linux, and host emulation are also supported. Generic Arduino
requires regular clock sampling and serialized calls; see
[clock backends](#clock-backends) for platform-specific behavior.

| Type | Represents | Typical use |
| --- | --- | --- |
| `Duration` | An amount of time | `Millis(250)`, `Seconds(2)`, `Hours(24)` |
| `Uptime` | A point on the device/process uptime clock | Measuring elapsed time and setting deadlines |
| `WallTime` | A timestamp relative to the Unix epoch | Reading an RTC or a synchronized system clock |
| `SmallDuration` / `SmallTimestamp` | Compact millisecond durations / wrapping timestamps | Storing many short-lived timing values |
| `CivilDay` | A date without time or timezone | Checked Gregorian dates and calendar-day arithmetic |
| `DateTime` | Calendar fields for a wall time at a fixed UTC offset | Displaying a date, hour, or day of week |

Use uptime for timing work and wall time for dates. `UtcOffset` supplies a fixed UTC
offset. `TimeZone` resolves seasonal offsets using built-in recurring rules or
a custom implementation; synchronization and RTC communication remain separate.

The examples below cover [durations](#durations-and-deadlines),
[wall time](#reading-wall-time), [calendar dates](#calendar-dates-and-utc-offsets),
and [RTC adapters](#connecting-an-rtc). The [reference](#reference-and-contracts)
collects the detailed ranges, rounding rules, and backend contracts.

## Durations and deadlines

Construct durations in the units you mean, then use arithmetic and comparisons
without converting everything to raw integers:

```cpp
Duration timeout = Seconds(2) + Millis(500);
Duration retry_interval = Millis(250);
bool shorter = retry_interval < timeout;
int64_t timeout_ms = timeout.inMillis();  // 2500
```

`Delay(duration)` blocks for a duration. `DelayUntil(deadline)` blocks until an
uptime deadline, returning immediately if it has already passed. Scheduling can
make either wait longer than requested; use them in task/loop context.

Subtracting two uptimes gives a duration. Adding a duration to an uptime gives
another uptime. The same arithmetic works for wall time, but mixing wall time and
uptime is a compile-time error.

## Compact timing values (prototype)

`SmallDuration` and `SmallTimestamp` store millisecond timing values in four bytes
each. Dedicated integer factories make compact storage explicit:

```cpp
SmallDuration interval = SmallMillis(250);
SmallTimestamp start = SmallTimestamp::Now();
SmallTimestamp deadline = start + interval;
SmallDuration remaining = deadline - SmallTimestamp::Now();
Duration full_remaining = remaining;  // Implicit, lossless widening.
```

Compact timestamps wrap. Use their ordering and differences only within a window
strictly shorter than 2^31 milliseconds (about 24.86 days). See
[compact timing contracts](#compact-timing-contracts) and
[duration factories](#duration-factories) for range and compatibility
details.

## Reading wall time

On ESP32 or Linux, `SystemClock` reads the platform's system wall clock:

```cpp
SystemClock my_clock;
WallTime now = my_clock.now();
WallTime tomorrow = now + Hours(24);
int64_t seconds_since_epoch = now.sinceEpoch().inSeconds();
```

Configure synchronization separately—for example, NTP on ESP32—and track whether
the clock is ready. `SystemClock` reads the clock; it does not synchronize it.
See the [ESP32 NTP example](examples/Esp32NtpTime/Esp32NtpTime.ino) for setup.

RTC drivers can expose the same `WallTimeClock` interface. For a DS3231, the
companion [roo_time_ds3231](https://github.com/dejwk/roo_time_ds3231) library supplies
an adapter. Other devices can use a [small adapter of their own](#connecting-an-rtc).

## Civil dates

Use `CivilDay` for a date without an instant or timezone:

```cpp
CivilDay birthday = CivilDay::FromYmd(2000, 2, 29);
CivilDay next_day = birthday.addDays(1);  // 2000-03-01
```

It supports years 1–9999, checked construction and arithmetic, and an invalid
value. `DateTime::civilDay()` extracts its local date. `ParseCivilDay` and
`FormatCivilDay` share the existing numeric text engine. See
[civil-date contracts and examples](docs/civil_day.md).

## Calendar dates and UTC offsets

Convert between wall time and calendar fields with `DateTime`:

```cpp
UtcOffset local_offset(Hours(2));  // Fixed UTC+02:00.
DateTime appointment(2026, 9, 12, 14, 30, 0, 0, local_offset);
WallTime instant = appointment.wallTime();

// View the same instant in UTC: 12:30 on the same date.
DateTime utc(instant, timezone::UTC);
int hour = utc.hour();
DayOfWeek weekday = utc.dayOfWeek();
```

For the current local date and time, use `DateTime(my_clock.now(), local_offset)`.
The object also exposes `year()`, `month()`, `day()`, `minute()`, `second()`, and
`micros()`.

Breaking change: `TimeZone` is now an abstract offset resolver, replacing the
former alias for `UtcOffset`. Replace old fixed-offset `TimeZone` declarations
with `UtcOffset`. Prefer `DateTime::utcOffset()` over the deprecated `timeZone()`
accessor, and `UtcOffset::asDuration()` over the deprecated `offset()` accessor.
`timezone::UTC` remains the zero fixed offset. Offsets support direct equality
and ordering comparisons, plus `inMinutes()`, `inSeconds()`, `inMillis()`, and
`inMicros()`.

Offsets are fixed; they do not automatically follow daylight-saving changes.
For seasonal offsets, see [time zones](#time-zones-and-seasonal-rules).
Also, `DateTime()` means the Unix epoch, not the current time.

## Formatting and parsing calendar dates

Include `roo_time/format.h` for allocation-free buffer formatting, strict parsing,
and optional string conveniences. The core `roo_time.h` remains independent of
this header. Bazel users can depend on `:format` directly or on `:roo_time`.
The view overloads use `roo_backport`.

```cpp
#include "roo_time/format.h"

using namespace roo_time;
DateTime appointment(2026, 9, 12, 14, 30, 0, 42, UtcOffset(Hours(2)));
char buffer[48];
FormatResult formatted = FormatDateTime(
    appointment, "%FT%T.%f%:z", buffer, sizeof(buffer));
// On success: 2026-09-12T14:30:00.000042+02:00

DateTime parsed;
ParseResult status = ParseDateTime(
    "2026-09-12 14:30", 16, "%F %H:%M", UtcOffset(Hours(2)), &parsed);
if (status.status == TextStatus::kOk) {
  WallTime instant = parsed.wallTime();
  // Use instant...
}

#if ROO_TIME_HAS_STRING_VIEW
// Neither view needs a terminating NUL; both are consumed within their bounds.
status = ParseDateTime(roo::string_view("2026-09-12"), "%F",
                      timezone::UTC, &parsed);
#endif

#if ROO_TIME_HAS_STD_STRING
std::string text = FormatDateTime(appointment, "%F %T");
#endif

#if defined(ARDUINO)
String arduino_text = FormatDateTimeArduino(appointment, "%F %T");
#endif
```

The portable format language is ASCII and independent of locale and the process
timezone:

| Directive | Formatting | Parsing |
| --- | --- | --- |
| `%Y` | Four-digit year | Exactly four digits, 0001–9999 |
| `%m`, `%d` | Two-digit month and day | Exactly two digits, validated Gregorian date |
| `%H`, `%M`, `%S` | Two-digit hour, minute, second | Exactly two digits, 00–23 / 00–59 / 00–59 |
| `%f` | Six-digit microseconds | One to six digits, scaled to microseconds; extra digits rejected |
| `%z`, `%:z` | `+hhmm` / `+hh:mm`, including positive zero | Same syntax, or `Z` for UTC |
| `%F`, `%T` | `%Y-%m-%d` / `%H:%M:%S` | Same expansions |
| `%%` | Literal `%` | Literal `%` |

Other characters match literally, including whitespace. Embedded NULs in formats
and unknown directives are errors. Delimit a variable-width `%f` from following
numeric fields. Offset text is limited to ±23:59; larger `UtcOffset` values can
still be used when no offset directive is present. An explicit parsed offset
overrides the supplied default, which is otherwise used as-is. Negative zero
offsets become UTC; these functions do not claim complete ISO 8601/RFC 3339 support.

Parsing requires year, month, and day; omitted time fields default to zero.
Repeated fields must agree. Parsing consumes the entire input and rejects invalid
dates, leap seconds, and out-of-range fields before constructing a `DateTime`.
It leaves the destination unchanged on failure. `ParseResult::position` reports
consumed bytes on success or the input error offset on failure (zero for an
invalid format; end of input for missing required fields).

`FormatResult::size` reports required bytes excluding NUL on success or
`kBufferTooSmall`, and zero on other errors. A positive output capacity requires a
nonnull buffer, which is always NUL-terminated. Insufficient space returns a
truncated prefix and `kBufferTooSmall`; other errors clear the buffer. Use
`nullptr, 0` for a size query: it returns `kBufferTooSmall` even for an empty
format, whose terminating NUL needs one byte. Output must not overlap the format.
The pointer overloads also accept an explicit format length for bounded input
without `roo::string_view`.

The `std::string` overload returns an empty string on formatting error, also the
successful output of an empty format. Allocation failures follow normal
`std::string` behavior. `FormatDateTimeArduino` is available only with `ARDUINO`
defined, returns Arduino `String`, and does not require `std::string`. It uses a
small stack buffer for typical output and temporary heap storage for longer
output; formatting or allocation failure returns an empty `String`.

`ROO_TIME_HAS_STD_STRING` detects `<string>` with `__has_include`. It defaults to
zero on toolchains without header detection; explicitly set it to 1 only when the
standard string implementation works, or to 0 to omit the allocating overloads.
`ROO_TIME_HAS_STRING_VIEW` defaults to the same value because the existing
`roo_backport/string_view.h` itself includes `<string>`. It can independently be
set to 1 on a capable toolchain when disabling only the allocating overloads.
Pointer/buffer overloads remain available with both macros set to zero.

Formatting uses native `strftime` for compatible two-digit calendar directives
on Linux and ESP32, checking the result against the portable contract. Years,
fractions, offsets, and parsing use shared portable code. Define
`ROO_TIME_HAS_STRFTIME=0` when compiling `format.cpp` to force the fallback, or 1
to enable the native adapter on another platform with `<ctime>` and
`std::strftime`. No path uses `mktime`, modifies `TZ`, or depends on `time_t`'s
range. Both formatting backends run the same conformance tests.

### Canonical ISO datetime helpers

`FormatIsoDateTime` and `ParseIsoDateTime` provide a named convenience for the
library's ISO 8601 extended profile. Formatting always emits
`YYYY-MM-DDTHH:MM:SS.ffffff±HH:MM`, preserving all six microsecond digits and the
stored offset. UTC prints as `+00:00`. This is one explicitly defined profile;
it does not cover all ISO 8601 representations.

```cpp
char iso[kIsoDateTimeBufferSize];  // 33 bytes, including NUL.
FormatResult written = FormatIsoDateTime(appointment, iso, sizeof(iso));
DateTime restored;
ParseResult read = ParseIsoDateTime("2026-09-12T14:30:00.000042+02:00", &restored);

#if ROO_TIME_HAS_STD_STRING
std::string text = FormatIsoDateTime(appointment);
#endif
#if defined(ARDUINO)
String text_arduino = FormatIsoDateTimeArduino(appointment);
read = ParseIsoDateTime(text_arduino, &restored);
#endif
```

Parsing also accepts whole seconds, one to six fractional digits, and `Z` for
UTC, e.g. `2026-09-12T12:30:00Z`. The date, time through seconds, uppercase `T`,
and explicit offset are required. Offset numerals require a colon. Whitespace,
lowercase `t`/`z`, excess precision, and invalid dates are rejected. The same
offset range, error statuses, and unchanged-destination guarantee apply as for
the general API. Input can be a NUL-terminated string, a pointer plus length,
`roo::string_view` when enabled, or Arduino `String` on Arduino.

## Connecting an RTC

Implement `WallTimeClock::now()` to return the device's UTC timestamp. This
illustrative adapter assumes a driver with `begin()` and `millisSinceEpoch()`:

```cpp
#include "my_rtc_time_lib.h"
#include "roo_time.h"

using namespace roo_time;

class MyClock : public WallTimeClock {
 public:
  // optional; initializes the driver.
  void begin() {
    rtc_.begin();
  }

  WallTime now() const override {
    // Read time, e.g. as milliseconds since Epoch.
    return WallTime::SinceEpoch(Millis(rtc_.millisSinceEpoch()));
  }

 private:
  MyRtcDevice rtc_;
};

```

If the driver returns calendar fields instead, construct a UTC `DateTime` and
return its `wallTime()`. Initialize the adapter according to the driver's needs:

```cpp
MyClock my_clock;

void setup() {
  my_clock.begin();
}

void loop() {
  WallTime now = my_clock.now();
  WallTime tomorrow = now + Hours(24);
  int64_t seconds_since_epoch = now.sinceEpoch().inSeconds();
  // ...
}
```

## Optional std::chrono interoperability

Include `roo_time/chrono.h` to enable explicit duration conversions when the
standard library provides `<chrono>`. The core `roo_time.h` remains independent
of that header.

```cpp
#include "roo_time/chrono.h"
#if ROO_TIME_HAS_CHRONO
Duration interval = FromChrono(std::chrono::milliseconds(1500));
auto standard = ToChrono(interval);  // std::chrono::microseconds
auto milliseconds = ToChrono<std::chrono::milliseconds>(interval);
SmallDuration compact(FromChrono(std::chrono::seconds(2)));
#endif
```

The adapter uses `__has_include` to detect the header. On toolchains without
header detection it defaults to disabled; define `ROO_TIME_HAS_CHRONO=1` only if
`<chrono>` is supported. Define it to `0` to keep the adapter disabled even when
the header exists. Detection covers duration types, not working hardware clocks.

Conversions follow `duration_cast`: integer results truncate toward zero, and
intermediate and destination values must be representable. Floating inputs must
be finite. Clock/time-point conversions are intentionally absent because epochs
and sleep accounting may differ.

## Host emulation

Host builds support both Arduino and ESP-IDF through roo_testing 2.0. With
Bazelisk 1.21 or newer, a plain command defaults to Arduino and prints a notice:

    bazel test ...
    bazel test ... --config=asan
    bazel test ... --config=roo_testing_arduino_esp32
    bazel test ... --config=roo_testing_idf_esp32
    .roo_testing/bin/test_all_profiles ...

The files under .roo_testing are vendored from roo_testing; follow their
canonical-source headers when refreshing them.

Arduino examples are native runnable targets in their source packages. For
example:

    bazel run //examples/ElapsedTime:ElapsedTime

## Reference and contracts

The following details define behavior at platform and value boundaries.

### Clock backends

The primary microcontroller target is ESP32, with Arduino or ESP-IDF. Pico SDK device
builds and RP2040 Arduino cores exposing `pico/time.h` use the native Pico timer.
Other Arduino cores retain the generic `micros()` backend and its sampling and
serialization requirements.
Native Linux and roo_testing backends support host use and emulation.

Both uptime acquisition and blocking delays are platform-dependent. `SystemClock`
also requires the platform's `gettimeofday`; it is exposed on Linux and builds
that define `ESP_PLATFORM`. Other platforms need an appropriate backend; the
Arduino metadata's `architectures=*` does not guarantee every core or host OS.

| Backend | Uptime origin and resolution | Sleep and rollover behavior |
| --- | --- | --- |
| ESP32 Arduino / ESP-IDF | `esp_timer_get_time()`, microseconds since timer initialization during startup | Native 64-bit counter; light sleep is included after wakeup; deep sleep restarts the application and counter. |
| Pico SDK / SDK-backed RP2040 Arduino | Native `time_us_64()`, microseconds since the hardware timer origin | No software wrap extension or periodic sampling requirement. Sleep accounting requires the hardware timer to remain running. |
| Generic Arduino | Extended low 32 bits of `micros()`; resolution comes from the Arduino core | Call before the first rollover and at intervals strictly shorter than 2^32 microseconds (about 71.6 minutes). Whether sleep is counted depends on the core. |
| Native Linux | `steady_clock`, relative to the first uptime access; converted to whole microseconds | Independent of wall-clock adjustments. Suspend accounting follows the host steady clock; this is not a boot-time or persisted clock. |
| roo_testing | Emulated system uptime | Time advancement and host synchronization follow the emulator's configuration. |

`Uptime::Start()` is the zero value in the selected clock's domain. Timestamps
from different devices, processes, or restarts must not be compared as if they
shared an origin. A microsecond storage unit does not promise microsecond hardware
resolution or wakeup accuracy.

The generic Arduino extension can recover one counter wrap between samples. It
cannot reconstruct missed full periods, including periods before its first call.
All accesses to its shared clock state must be serialized by the application,
including accesses inside `Delay` and `DelayUntil`; do not call it concurrently
from tasks, cores, or an ISR. Native ESP32, Pico, and Linux clock acquisition do not use
that shared extension state. ISR use on ESP32 additionally requires the platform
API and all called code to be available in the interrupt's execution context.

Value objects are not atomic. Concurrent reads of an unchanged object are fine;
shared mutation requires synchronization. In particular, `Uptime`'s copy and
assignment operations accepting `volatile` sources do not make a 64-bit access
atomic on a smaller MCU or establish synchronization between threads.

For standalone Pico SDK builds, compile `src/roo_time/duration.cpp`,
`src/roo_time/civil_day.cpp`, `src/roo_time/internal/calendar.cpp`,
`src/roo_time/wall_time.cpp`, `src/roo_time/timezone.cpp`, and
`src/uptime_now.cpp`, add `src` to the include path, and link `pico_time`.
The SDK supplies `PICO_ON_DEVICE`; compatible RP2040 Arduino cores are detected
through `ARDUINO_ARCH_RP2040` and availability of `pico/time.h`. Pico waits use
`sleep_us`, allowing the SDK to manage lower-power waits; its normal interrupt
and timer configuration requirements apply.

### Value ranges and contracts

- `Duration` and `WallTime` store signed 64-bit microseconds. Construction,
  arithmetic, and conversion require all relevant intermediate and final values
  to be representable. General arithmetic is unchecked and does not saturate;
  signed overflow is undefined behavior. Floating inputs must be finite and their
  scaled values representable; fractional microseconds are truncated toward zero.
- `Duration::Max()` and `Uptime::Max()` are finite values, not infinity. Adding to
  either can overflow. Clock-generated uptime is nonnegative; applications must
  also keep shifted uptimes and differences within the signed 64-bit range.
- `Duration::toComponents()` is the explicit saturation exception: magnitudes
  beyond 67,108,863 days, 23:59:59.999999 are clamped to that limit, retaining the
  sign. Thus large values do not round-trip through components. `FromComponents`
  requires normalized fields: hours 0–23, minutes/seconds 0–59, microseconds
  0–999999, and days within the 26-bit field's range.
- The supported calendar contract is Gregorian years 1–9999. Component inputs
  must describe a valid date, with hours 0–23, minutes/seconds 0–59 and microseconds
  0–999999. These preconditions are unchecked: invalid input is not an error value
  or a request for normalization. Converting wall time must produce a local date
  in this range after applying the offset. The timestamp storage range is much
  wider than this calendar contract.
- `UtcOffset` represents a fixed UTC offset, not a geographic zone or a DST rule.
  Supply whole minutes within the signed 16-bit minute range. Construction
  truncates sub-minute offsets toward zero and does not validate the range.
- Wall time follows Unix/POSIX time without distinct leap seconds. `DateTime()`
  and `WallTime()` continue to represent the Unix epoch. Use `WallTime::Unset()`
  for an explicit unset value. Prefer `WallTime::Epoch()` or
  `WallTime::SinceEpoch(duration)` over the deprecated constructors. `DateTime`
  equality compares both the instant and offset; compare `wallTime()` when only
  the instant matters.

`tmStruct()` exports local calendar fields with a zero-based `tm_yday` and
`tm_isdst = -1`. It does not carry the fixed UTC offset. Passing it to `mktime`
uses the C library's configured local timezone, which may differ. The `tm`
constructor uses the supplied calendar fields and explicit `UtcOffset`; it does
not interpret `tm_isdst`, `tm_wday`, or `tm_yday`.

### Delay contracts

`Delay` treats zero and negative durations as no-ops. For positive durations it
rechecks elapsed uptime after platform waits. ESP32 waits are bounded by the
largest finite RTOS tick delay, with conversion performed in 64 bits. Generic
Arduino waits are capped at half the 32-bit microsecond counter period (about
35.8 minutes), leaving the other half as scheduling margin; the actual interval
between clock samples must still remain below one full period. Pico, Linux, and
roo_testing pass the full remaining duration to their delay APIs. `DelayUntil`
returns immediately for a deadline at or before the current uptime; otherwise it
waits until that uptime deadline has been reached or passed.

These guarantees require a progressing clock and the backend's sampling and
serialization contracts. Coarse RTOS ticks and scheduling can cause overshoot;
there is no upper bound on wakeup latency. Short waits, including ESP32 waits
below an RTOS tick, can busy-wait. Use these blocking APIs in normal task/loop context, not from an ISR or a context that
prevents the underlying clock or scheduler from progressing. They do not configure
deep sleep or schedule asynchronous callbacks.

### Wall-clock validity and synchronization

`WallTimeClock::now()` returns unset wall time on read errors. Check `isSet()`
before arithmetic or conversion to `DateTime`; these operations require valid
inputs and assert that precondition in debug builds. `INT64_MIN` microseconds is
reserved for invalid time; arithmetic results must fit and remain valid.
Validity does not imply synchronization: the adapter/application must separately
track whether an RTC has been initialized or a network clock synchronized.
`SystemClock` does not initiate NTP synchronization. It returns unset wall time
if `gettimeofday` fails, distinguishable from a successful epoch reading.

Wall time can jump forward or backward after synchronization or manual adjustment.
Use `Uptime` for elapsed-time measurement and deadlines. Fixed offsets do not handle DST transitions. `TimeZone` resolves UTC instants
to seasonal offsets. Reverse local-time resolution and automatic rule updates
are not provided.

### Rounding semantics

`Duration` narrowing conversions expose three rounding modes:

- `inXxx()` and `inXxxRoundedDown()` round **toward zero**.
- `inXxxRoundedUp()` rounds **away from zero**.
- `inXxxRoundedNearest()` rounds to nearest, with ties (exact half) **away from zero**.

Examples for milliseconds:

- `Micros(1501).inMillisRoundedDown()` is `1`, `Micros(-1501).inMillisRoundedDown()` is `-1`.
- `Micros(1501).inMillisRoundedUp()` is `2`, `Micros(-1501).inMillisRoundedUp()` is `-2`.
- `Micros(500).inMillisRoundedNearest()` is `1`, `Micros(-500).inMillisRoundedNearest()` is `-1`.

### Time zones and seasonal rules

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
transition, resolve it again. These APIs take an instant, not ambiguous local
calendar fields. Parsing continues to use an explicit fixed offset; it does not
infer a zone or choose between duplicated/nonexistent local times.

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

### Compact timing contracts

`SmallDuration` stores signed 32-bit milliseconds; `SmallTimestamp` stores an
opaque unsigned 32-bit millisecond timestamp. Each occupies four bytes:

`Uptime` converts implicitly to `SmallTimestamp`, truncating to milliseconds and
retaining the low 32 bits. `SmallTimestamp::Now()` reads the backend directly:
ESP32, Pico, Linux, and roo_testing truncate their native microsecond clocks to
milliseconds and retain the low 32 bits, sharing Uptime's origin and sleep behavior.
Generic Arduino uses `millis()` directly, without 64-bit arithmetic or the shared
`micros()` extension. Compact reads need no periodic sampling and do not maintain
the extension used by `Uptime::Now()`. Concurrency and sleep behavior follow the
Arduino core's `millis()` implementation.

On generic Arduino, use compact readings consistently for a timing operation:
`millis()` and extended `micros()` may differ in resolution, rounding, and sleep
accounting. Converting Uptime does not necessarily produce the same clock reading,
especially if its sampling contract was violated. There is no absolute-time
accessor or recoverable epoch on a compact timestamp.

Timestamp shifts wrap modulo 2^32. Ordering and subtraction use the closer signed
difference, so a tick value of zero follows `0xFFFFFFFF`. The caller must ensure
that compared timestamps are from the same clock domain and actually separated
by **strictly less than 2^31 milliseconds** (about 24.86 days). Exactly half a
cycle is ambiguous and asserted in debug builds; other violations cannot always
be detected. Equality compares stored bits, so instants one full cycle apart can
compare equal. Sorting requires the entire collection to fit in one half-cycle
window.

Compact duration arithmetic does **not** wrap. Results must fit signed 32-bit
milliseconds. Widening to `Duration` is implicit; narrowing an existing `Duration`
is explicit and truncates fractional milliseconds toward zero:

```cpp
Duration full = Seconds(2.5);
SmallDuration compact(full);
```

### Duration factories

`Micros`, `Millis`, `Seconds`, `Minutes`, and `Hours` always return `Duration`,
including with `auto`. Integer counts scale in 64 bits; floating inputs truncate
fractional microseconds toward zero.

`SmallMillis`, `SmallSeconds`, `SmallMinutes`, and `SmallHours` accept integers
only and return `SmallDuration`. Scaled milliseconds must fit signed 32 bits;
range violations are caller errors checked with debug assertions. There is no
`SmallMicros`: use explicit narrowing for intentional precision loss.

```cpp
auto timeout = Seconds(2);           // Duration, mutable.
auto interval = SmallSeconds(2);     // SmallDuration.
SmallDuration fractional(Seconds(0.5));  // Explicit narrowing: 500 ms.
Duration widened = interval;         // Implicit, lossless widening.
```

Arithmetic between compact durations stays compact; mixing with `Duration`
returns `Duration`. Multiplication accepts integer factors representable in
signed 64 bits, preserves their width, and requires representable intermediate
and final results. Floating multiplication uses the factor's floating-point type,
then truncates toward zero to microseconds for `Duration` or milliseconds for
`SmallDuration`. Both operand orders are supported; compact multiplication stays
compact. For example, `Seconds(2) * 0.5` is one second, and
`SmallMillis(3) * 0.5` is one millisecond.

Floating factors and products must be finite, and intermediate and final values
must be representable. These are caller preconditions, not a saturating conversion
or error-return API. Converting duration counts to floating point can lose
precision, especially for large counts or on targets with 32-bit `double`.
Integer multiplication keeps its exact integer path. Compact factories remain
integer-only; floating multiplication does not change their input contract.

The former integer-expression template and its Int32 unit aliases have been
removed. Replace compact initializers such as `SmallDuration d = Seconds(2)`
with `SmallDuration d = SmallSeconds(2)` or explicit `SmallDuration(Seconds(2))`.
Full-duration narrowing remains explicit. Timestamp shifts accept either duration
size and truncate fractional milliseconds, with a signed 32-bit shift limit.

### Performance and program size

`Duration`, `Uptime`, and `WallTime` each wrap a 64-bit value, and the core value
operations allocate no heap memory. Optimized value operations can compile like
equivalent `int64_t` arithmetic. This does not make them as cheap as 32-bit time
arithmetic on every MCU; 64-bit division and floating-point conversions can be
significant on smaller targets. Clock acquisition and blocking delays have their
backend's costs.

Unused functionality can be removed by the compiler/linker when the build enables
suitable sectioning and garbage collection. Calendar conversion and virtual clock
adapters have costs when used. Exact flash, RAM, and execution costs depend on the
toolchain, target, and application; no cross-MCU size or timing bound is promised.
