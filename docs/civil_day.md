# Civil dates

`roo_time/civil_day.h` provides a four-byte Gregorian `CivilDay` without a clock,
time of day, or timezone. It supports years 1–9999 and an invalid value. It is
also available through `roo_time.h` and the Bazel `:core` target.

```cpp
using namespace roo_time;
CivilDay date = CivilDay::FromYmd(2024, 2, 29);
CivilDay tomorrow = date.addDays(1);  // 2024-03-01
bool leap = IsLeapYear(date.year());
uint8_t length = DaysInMonth(date.year(), date.month());
```

Default construction and `CivilDay::Invalid()` produce the same invalid value.
`FromYmd` validates signed 32-bit components before narrowing; it rejects invalid
dates instead of normalizing them. `addDays` returns invalid for invalid input
or a result outside the supported range, including arithmetic extremes.

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
