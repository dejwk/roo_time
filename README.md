# roo_time

Time values for embedded C++: express units explicitly, measure elapsed work,
read UTC clocks, and work with calendar dates and local schedules.

Use **uptime** for deadlines and elapsed measurements. Use **wall time** for
absolute timestamps. Use a **civil date and time of day** for settings such as
“run every morning at 06:30”, then resolve them through a timezone.

## Choose the value you need

| Type | Meaning | Default state |
| --- | --- | --- |
| `Duration` | Amount of time, such as `Millis(250)` | Zero |
| `Uptime` | Instant in this device/process uptime clock | Zero in that clock domain; does not read the clock |
| `WallTime` | Absolute UTC instant relative to Unix epoch | No public default constructor; choose `Unset()`, `Epoch()`, or `SinceEpoch()` |
| `DateTime` | Calendar fields at a fixed UTC offset | Unix epoch in UTC; not the current time |
| `CivilDay` | Gregorian date without a clock or timezone | Invalid |
| `TimeOfDay` | Local clock reading without a date or timezone | Invalid; use `Midnight()` for valid zero |
| `UtcOffset` | Fixed local-minus-UTC offset | UTC (zero offset) |
| `SmallDuration` / `SmallTimestamp` | Four-byte millisecond duration / wrapping timestamp | Zero / zero timestamp bits |

`Duration`, `Uptime`, `WallTime`, and `TimeOfDay` store signed 64-bit microseconds.
`CivilDay` is four bytes. Value operations allocate no heap memory. Clock
origins, resolution, and sleep accounting depend on the
[backend](docs/platforms.md#clock-backends).

## Measure elapsed work

```cpp
#include "roo_time.h"
#include "roo_time/format.h"

using namespace roo_time;

Uptime start = Uptime::Now();
Delay(Millis(25));  // Represents work in this short illustration.
Duration elapsed = Uptime::Now() - start;
char text[48];
FormatResult result = FormatDuration(elapsed, text, sizeof(text));
if (result.status == TextStatus::kOk) {
  // text is a clock-like duration, e.g. "00:00:00.025".
}
```

Subtracting uptime instants gives a duration; adding a duration produces a
new instant. A timeout can compare `Uptime::Now()` with `start + Seconds(2)`.
`DelayUntil(deadline)` blocks until a deadline; it returns immediately for one
that has passed. For cooperative work, compare deadlines without blocking.

See [ElapsedTime](examples/ElapsedTime/) and the
[timing guide](docs/timing.md) for deadline behavior, compact values, arithmetic,
and rounding. General arithmetic must remain representable; it does not saturate.

## Read a wall clock

```cpp
#include "roo_time.h"

using namespace roo_time;

SystemClock clock;  // ESP32 or Linux; synchronization is configured separately.
WallTime now = clock.now();
// The application must also establish readiness and a supported calendar range.
const WallTime first = DateTime(1, 1, 1, timezone::UTC).wallTime();
const WallTime last = DateTime(9999, 12, 31, 23, 59, 59, 999999,
                               timezone::UTC).wallTime();
if (now.isSet() && now >= first && now <= last) {
  DateTime utc(now, timezone::UTC);
  // utc.hour(), utc.minute(), etc. expose the UTC calendar fields.
  // Only use them when the application also knows the clock is ready.
}
```

`isSet()` means a read produced a value, not that NTP or an RTC has been
synchronized. Clock-read errors produce `WallTime::Unset()`. Check validity
before timestamp arithmetic or conversion to `DateTime`. A wall clock can jump;
use uptime for measuring elapsed work. Calendar conversion additionally requires
local years 1–9999, a much smaller range than timestamp storage.

[Esp32NtpTime](examples/Esp32NtpTime/) shows platform setup. The
[calendar guide](docs/calendar.md) explains UTC instants, checked calendar
construction, civil dates, and fixed-offset snapshots.

## Work with a local schedule

```cpp
#include "roo_time/timezone.h"

using namespace roo_time;

RecurringTimeZone zone(UtcOffset(Hours(1)), rules::EuSummerTime());
CivilDay day = CivilDay::FromYmd(2026, 10, 25);
TimeOfDay time = TimeOfDay::FromHms(2, 30);
LocalTimeResolution occurrence = ResolveLocal(day, time, zone);
if (occurrence.status == LocalTimeStatus::kAmbiguous) {
  // Choose occurrence.first or occurrence.second as an explicit policy.
}
```

Checked date/time factories and parsers reject invalid fields without
normalizing them. `ResolveLocal` distinguishes unique, repeated, missing,
invalid, unsupported, and out-of-range times. It does not choose a DST policy.
For daily repetition, advance `day.addDays(1)` and resolve again; adding 24
elapsed hours can change the local hour. See the [timezone guide](docs/timezones.md).
Built-in rules are recurring patterns, not historical geographic databases.

## Print or parse values

Include `roo_time/format.h`. Buffer APIs allocate nothing, report required
sizes excluding NUL, and support bounded text and format ranges. Check their
status: a too-small buffer contains only a truncated prefix. Optional
`std::string`, Arduino `String`, and string-view conveniences are also available.

- `FormatDuration`: default `HH:MM:SS` with an optional trimmed fraction.
- `FormatUptime`: default `S+HH:MM:SS.ffffff`, with total hours and an explicit sign.
- `FormatIsoDateTime`: a fixed ISO profile with date, time, microseconds, and UTC offset.
- `FormatCivilDay` / `ParseCivilDay`: date fields only.
- `FormatTimeOfDay` / `ParseTimeOfDay`: local clock fields only.

The [formatting guide](docs/formatting.md) explains the two format languages,
errors, precision, padding, parsing, and allocation options. In particular, `%F`
and `%T` mean different things for durations and calendar values.

## Examples and reference

Follow the [examples index](examples/README.md) for runnable scenarios.

- [Timing and rounding](docs/timing.md)
- [Wall clocks and calendar values](docs/calendar.md)
- [Formatting and parsing](docs/formatting.md)
- [Time zones and local schedules](docs/timezones.md)
- [Platforms, builds, and clock origins](docs/platforms.md)
- [RTC and std::chrono integrations](docs/integrations.md)
- [Release notes and migration](RELEASE_NOTES.md)

Use `roo_time.h` for core value types; `roo_time/timezone.h` for zone resolution;
`roo_time/format.h` for text; and `roo_time/chrono.h` or `roo_time/acetime.h` for
optional adapters. Scheduling execution and hardware synchronization belong to
the application or companion libraries.

## Build and run

The primary target is ESP32 with Arduino or ESP-IDF. Pico SDK, compatible
RP2040 Arduino cores, native Linux, and host emulation also have uptime backends.
Generic Arduino requires periodic sampling and serialized uptime reads.
See the [platform guide](docs/platforms.md) before relying on a clock's origin,
sleep accounting, or concurrency behavior.

With Bazelisk and the repository's roo_testing setup:

```sh
bazel test //... --config=roo_testing_arduino_esp32
bazel test //... --config=roo_testing_idf_esp32
bazel run //examples/ElapsedTime:ElapsedTime --config=roo_testing_arduino_esp32
```

Arduino examples are Arduino-only targets; core library tests also run under
ESP-IDF. Building the host examples does not flash a board. See individual
example notes for credentials, hardware, and emulator limitations.
