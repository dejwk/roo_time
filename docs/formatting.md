# Formatting and parsing

Choose a formatter for the meaning of the value. Duration/uptime formats and
calendar formats share some spelling but are separate languages.

| Directive | Duration / uptime | Calendar date / time |
| --- | --- | --- |
| `%H` | Total whole hours | Hour within a day |
| `%T` | Compact clock, e.g. `1:25` | `%H:%M:%S`, e.g. `00:01:25` |
| `%F` | Optional point and trimmed fraction | `%Y-%m-%d` |
| `%+` | Explicit sign | Unsupported |

Buffer functions allocate nothing. Check `FormatResult::status` before using
formatted output and `ParseResult::status` before using parsed values.
`FormatResult::size` is the required length, not necessarily the bytes written
when a buffer is too small.

[Back to the overview](../README.md) · [Runnable examples](../examples/README.md)

## Formatting durations

Include `roo_time/format.h` and use `FormatDuration` for the entire signed
64-bit microsecond range, including `Duration::Min()`, without component
saturation or heap allocation:

```cpp
char text[48];
FormatResult written = FormatDuration(Seconds(85) + Millis(1),
                                       kDurationCompactFormat, text,
                                       sizeof(text));  // "1:25.001"
FormatDuration(Hours(27), text, sizeof(text));  // Default: "27:00:00".
```

| Constant | Duration format | Example |
| --- | --- | --- |
| `kDurationClockFormat` (default) | `%H:%M:%S%F` | `27:05:06.001` |
| `kDurationCompactFormat` | `%T%F` | `1:25.001` |
| `kDurationDaysClockFormat` | `%Dd %h:%M:%S%F` | `3d 04:05:06.001` |
| `kDurationClockMillisFormat` | `%H:%M:%S.%.3f` | `27:05:06.001` |
| `kDurationClockMicrosFormat` | `%H:%M:%S.%f` | `27:05:06.001000` |

Duration format directives are independent of calendar directives:

| Directive | Meaning |
| --- | --- |
| `%D` | Total whole days, unpadded |
| `%H` | Total whole hours, at least two digits |
| `%h` | Hours within a day, `00-23` |
| `%M` | Minutes within an hour, `00-59` |
| `%S` | Seconds within a minute, `00-59` |
| `%f` | Six fractional digits, without a decimal point |
| `%.1f` through `%.6f` | Fixed fractional digits, truncating without rounding |
| `%F` | Optional decimal point and fraction, omitting trailing zeros |
| `%T` | Compact clock: `0`, `25`, `1:25`, `1:05:00`, or `27:00:00` |
| `%+` | Explicit sign (`+` for zero/positive, `-` for negative); suppresses automatic leading minus |
| `%%` | Literal percent |

`%F` preserves leading fractional zeros: one millisecond is `.001`, one
microsecond is `.000001`, and whole seconds emit no fraction. `%-H`, `%-h`,
`%-M`, `%-S`, and `%-D` disable padding. Explicit minimum widths from 1 through
255 use spaces (`%3D`) or zeros (`%03D`); digits are never truncated to fit.
Widths and `-` cannot be combined and apply only to integer directives.
Unsupported directives/modifiers, zero widths, and embedded NULs are invalid
formats and clear writable output.

Without `%+`, negative values receive one leading minus sign on nonempty
output; fractions truncate toward zero without carrying into seconds. A negative sub-millisecond
value formatted at millisecond precision retains its minus sign. Empty formats
or an empty `%F` alone produce empty output. Literal-only nonempty output also
receives the sign. The bounded-buffer and optional format-view overloads follow
the existing formatting contracts: size queries use `nullptr, 0`, sizes exclude
the NUL terminator, and output must not overlap the format.

When standard strings are enabled, `FormatDuration(value)` returns an owning
`std::string`; supply a format string or bounded format view to choose a layout.
Arduino builds also provide `FormatDurationArduino` returning `String`, with
default, NUL-terminated, bounded-range, and optional format-view forms.
Formatting errors and empty output return empty strings; Arduino allocation
failures also return empty. Buffer formatting remains usable with string and
string-view support disabled.

## Formatting uptime timestamps

`FormatUptime` uses exactly the same formatter as `FormatDuration`, interpreting
its input as a position since `Uptime::Start()`. The default complete format is
`kUptimeClockMicrosFormat` (`S%+%H:%M:%S.%f`): an `S+` or `S-` marker, total
hours rather than days, and exactly six fractional digits. The marker and sign
are part of the format, so custom formats control the entire output:

```cpp
char text[48];
Uptime stamp = Uptime::Start() + Hours(27) + Minutes(5) + Seconds(6) + Millis(1);
FormatUptime(stamp, text, sizeof(text));  // "S+27:05:06.001000"
FormatUptime(stamp, "S%+%T%F", text, sizeof(text));  // "S+27:05:06.001"
FormatUptime(stamp, kDurationCompactFormat, text, sizeof(text)); // "27:05:06.001"
```

The standalone `%+` directive emits a sign where it appears and disables the
usual leading minus. For example, negative one second with `S%+%T` produces
`S-1`; literal `S%T` instead produces `-S1`. Use `%+%H`, not `%+H`, for a sign
followed by total hours (`H` without `%` is a literal). Repeated `%+` directives
each emit a sign. `%%+` is a literal `%+` and does not disable automatic signs.
This directive works with `FormatDuration` as well as `FormatUptime`.

The bounded-buffer, optional format-view, `std::string`, and Arduino
`FormatUptimeArduino` overloads follow the duration formatter's contracts.
Sizes count the entire formatted output excluding NUL; empty formats produce
empty output. Invalid formats clear writable output and return
`kInvalidFormat`. The full signed uptime range, including its minimum value,
is supported without saturation. Uptime zero is the selected backend's clock
origin, not a Unix wall-time timestamp.

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

## Canonical ISO datetime helpers

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

## Civil and clock text

Include `roo_time/format.h` for allocation-free time text conversion:

```cpp
TimeOfDay morning = TimeOfDay::FromHms(6, 30);
char text[16];
FormatResult written = FormatTimeOfDay(morning, "%T.%f", text, sizeof(text));
TimeOfDay parsed;
ParseResult read = ParseTimeOfDay("06:30", 5, "%H:%M", &parsed);
```

`FormatTimeOfDay` and `ParseTimeOfDay` support `%H`, `%M`, `%S`, `%f`, `%T`,
`%%`, and literals. Date and offset directives are rejected. Parsing requires
hour and minute, defaults seconds/micros to zero, validates all ranges, requires
duplicate fields to agree, and leaves output unchanged on failure. `%f` consumes
up to six digits and scales shorter fractions to microseconds. Input must be
consumed entirely. Buffer APIs support bounded ranges, size queries, and NUL
termination just like `FormatCivilDay`; optional bounded string-view overloads
are available without allocating.
