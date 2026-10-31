# Wall clocks and calendar values

A `WallTime` is an absolute UTC instant. `DateTime` is a fixed-offset snapshot
of that instant; `CivilDay` and `TimeOfDay` hold calendar fields without an
instant. A UTC offset is a value; a timezone resolves which offset applies.

Check clock validity before use. A successful read and a synchronized clock
are separate facts.

[Back to the overview](../README.md) · [Runnable examples](../examples/README.md)

## Reading wall time

On ESP32 or Linux, `SystemClock` reads the platform's system wall clock:

```cpp
SystemClock my_clock;
WallTime now = my_clock.now();
if (now.isSet()) {
  WallTime later = now + Hours(24);
  int64_t seconds_since_epoch = now.sinceEpoch().inSecondsFloor();
}
```

Configure synchronization separately—for example, NTP on ESP32—and track whether
the clock is ready. `SystemClock` reads the clock; it does not synchronize it.
See the [ESP32 NTP example](../examples/Esp32NtpTime/Esp32NtpTime.ino) for setup.

RTC drivers can expose the same `WallTimeClock` interface. For a DS3231, the
companion [roo_time_ds3231](https://github.com/dejwk/roo_time_ds3231) library supplies
an adapter. Other devices can use a [small adapter of their own](integrations.md#connecting-an-rtc).

## Civil dates

`roo_time/civil_day.h` provides a four-byte Gregorian `CivilDay` without a clock,
time of day, or timezone. It supports years 1–9999 and an invalid value. It is
also available through `roo_time.h` and the Bazel `:core` target.

```cpp
using namespace roo_time;
CivilDay date = CivilDay::FromYmd(2024, 2, 29);
CivilDay tomorrow = date.addDays(1);  // 2024-03-01
int32_t days_apart = tomorrow - date;  // 1 calendar day.
bool leap = IsLeapYear(date.year());
uint8_t length = DaysInMonth(date.year(), date.month());
```

Default construction and `CivilDay::Invalid()` produce the same invalid value.
`FromYmd` validates signed 32-bit components before narrowing; it rejects invalid
dates instead of normalizing them. `addDays` returns invalid for invalid input
or a result outside the supported range, including arithmetic extremes.

Subtracting valid dates (`a - b`) returns the signed number of calendar days
from `b` to `a` as `int32_t`. Every difference in the supported range fits,
including the endpoints (+/-3,652,058 days). Invalid operands violate a
precondition and assert in debug builds. The count is independent of UTC
offsets; adjacent dates remain one calendar day apart across DST transitions.

Accessors and ordering require valid dates (asserted in debug builds). Equality
is defined for every value, and two invalid dates compare equal. `IsLeapYear`
requires years 1–9999; `DaysInMonth` also requires months January–December.

`DateTime::civilDay()` extracts the existing local date. For a timezone-aware
clock, use `ToLocal(clock.now(), zone).civilDay()` with a valid clock reading in
the zone's supported range. Adding a civil day is calendar arithmetic, which can
differ from adding 24 elapsed hours across an offset transition. There is no
implicit conversion from a date to an instant: callers must choose a time and
resolve their timezone policy separately.

Include `roo_time/format.h` (Bazel `:format`) for allocation-free numeric text:

```cpp
char text[11];
FormatResult written = FormatCivilDay(date, "%d.%m.%Y", text, sizeof(text));
CivilDay parsed;
ParseResult read = ParseCivilDay(text, written.size, "%d.%m.%Y", &parsed);
```

`ParseCivilDay` and `FormatCivilDay` share the date-time text engine. They support
`%Y` (four digits), `%m` and `%d` (two digits), `%F` (`%Y-%m-%d`), `%%`, and
literal separators. Time and offset directives are invalid formats. Parsing
requires all three date fields, consumes the complete input, validates the
Gregorian date, and leaves the destination unchanged on failure. Duplicate
fields must agree. Unpadded dates are not accepted.

The pointer-and-length overloads work without STL strings; bounded string-view
overloads follow `ROO_TIME_HAS_STRING_VIEW`. Formatting returns the required
length excluding NUL and NUL-terminates output when capacity is nonzero.
Insufficient capacity returns `kBufferTooSmall`; `nullptr, 0` queries the size.
Formatting invalid dates returns `kInvalidInput` and clears writable output.

## Time of day

`TimeOfDay` represents a local clock reading without a date or timezone. It
stores signed 64-bit microseconds and supports 00:00:00 through
23:59:59.999999. Default construction and `Invalid()` represent an absent or
invalid value; `Midnight()` is valid zero. Checked factories never normalize:

```cpp
TimeOfDay morning = TimeOfDay::FromHms(6, 30);  // Seconds/micros default to zero.
TimeOfDay precise = TimeOfDay::FromSinceMidnight(Hours(6) + Minutes(30));
bool same = morning == precise;
DateTime appointment;
bool valid = DateTime::TryFromComponents(CivilDay::FromYmd(2026, 11, 1),
                                         morning, UtcOffset(Hours(1)),
                                         appointment);
TimeOfDay local_time = appointment.timeOfDay();
```

`FromHms` validates signed 32-bit fields before narrowing. Out-of-range input,
including hour 24 and leap seconds, yields invalid. `FromSinceMidnight` rejects
negative durations and values at or after 24 hours. Accessors (`hour`, `minute`,
`second`, `micros`, `sinceMidnight`) and ordering require valid values and assert
in debug builds. Two invalid values compare equal. Ordering compares clock
positions within one day and does not wrap at midnight. `sinceMidnight()` is a
clock position, independent of real elapsed time across DST transitions.
Composition with a date requires an explicit fixed offset; invalid date/time
inputs return false and leave the `DateTime` output unchanged.

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

For the current local date and time, check `my_clock.now().isSet()` before
constructing `DateTime` from that reading. Keep synchronization readiness
separate from read validity.
The object also exposes `year()`, `month()`, `day()`, `minute()`, `second()`, and
`micros()`.

For calendar fields from settings, RTCs, or network input, validate before
constructing a snapshot:

```cpp
DateTime checked;
if (DateTime::TryFromComponents(2024, 2, 29, 12, 30, 0, 0,
                                local_offset, checked)) {
  // checked contains the validated date/time.
}
```

`TryFromComponents` accepts signed 32-bit fields and checks them before
narrowing. Invalid dates, years outside 1-9999, or out-of-range time fields
return false and leave the output unchanged. Constructors retain their
existing precondition that supplied components are valid.

`UtcOffset` is a fixed offset; `TimeZone` is a resolver for a complete offset
at a particular instant. Use `utcOffset()` on a `DateTime` and `asDuration()`
on an offset. `timezone::UTC` is the zero fixed offset. Offsets support equality,
ordering, and unit accessors. Migration notes for removed APIs are in
[the release notes](../RELEASE_NOTES.md).

Offsets are fixed; they do not automatically follow daylight-saving changes.
For seasonal offsets, see [time zones](timezones.md#time-zones-and-seasonal-rules).
Also, `DateTime()` means the Unix epoch, not the current time.

## Value ranges and contracts

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
- The supported calendar contract is Gregorian years 1–9999. Unchecked
  `DateTime` constructors require components describing a valid date, with hours 0–23, minutes/seconds 0–59 and microseconds
  0–999999. These preconditions are unchecked: invalid input is not an error value
  or a request for normalization. Converting wall time must produce a local date
  in this range after applying the offset. The timestamp storage range is much
  wider than this calendar contract.
- `UtcOffset` represents a fixed UTC offset, not a geographic zone or a DST rule.
  Supply whole minutes within the signed 16-bit minute range. Construction
  truncates sub-minute offsets toward zero and does not validate the range.
- Wall time follows Unix/POSIX time without distinct leap seconds. `DateTime()`
  represents the Unix epoch in UTC. `WallTime` has no public default constructor:
  explicitly use `WallTime::Unset()`, `WallTime::Epoch()`, or
  `WallTime::SinceEpoch(duration)`. `DateTime` equality compares both the instant
  and offset; compare `wallTime()` when only the instant matters.

`tmStruct()` exports local calendar fields with a zero-based `tm_yday` and
`tm_isdst = -1`. It does not carry the fixed UTC offset. Passing it to `mktime`
uses the C library's configured local timezone, which may differ. The `tm`
constructor uses the supplied calendar fields and explicit `UtcOffset`; it does
not interpret `tm_isdst`, `tm_wday`, or `tm_yday`.

## Wall-clock validity and synchronization

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
to seasonal offsets. Local-time resolution is available for fixed and recurring zones; automatic
rule updates are not provided.
