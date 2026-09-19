# [roo_time 1.5.0](https://github.com/dejwk/roo_time/releases/tag/1.5.0)

Published 2026-09-18.

roo_time 1.5.0 adds compact timing, date/time formatting and parsing, and clearer APIs for UTC offsets and wall-clock validity.
- UTC offsets: Added UtcOffset with direct comparisons, unit accessors, and asDuration(), plus DateTime::utcOffset(). TimeZone, offset(), and timeZone() remain available as deprecated compatibility APIs.
- Wall-time construction and validity: Added WallTime::Epoch(), SinceEpoch(), Unset(), and isSet(). Existing constructors are deprecated; default construction still represents the Unix epoch. SystemClock now returns unset time on read failure.
- Compact timing: Added four-byte SmallDuration and SmallTimestamp types, compact duration factories, and rollover-aware comparisons for intervals shorter than approximately 24.86 days.
- Formatting and parsing: Added allocation-free buffer APIs, strict calendar validation, microsecond precision, numeric UTC offsets, and ISO datetime helpers. Optional overloads support standard and Arduino strings.
- Duration improvements: Added optional std::chrono conversions, floating-point multiplication, improved integer-width handling, and constexpr minimum and maximum values.
- Platform and correctness fixes: Improved Arduino uptime rollover handling, Linux monotonic uptime, Pico SDK support, and delay handling. Fixed duration conversion overflow, pre-epoch calendar conversion, and tm day-of-year values.
Migration notes: Prefer the new offset accessors and named wall-time factories. Check isSet() on clock readings before arithmetic or calendar conversion. Both WallTime() and DateTime() retain their epoch defaults. Deprecation warnings may affect builds treating warnings as errors.

Adds a dependency on roo_backport for string-view support. UTC offsets remain fixed; automatic timezone and DST resolution are not included.

---

# [roo_time 1.4.7](https://github.com/dejwk/roo_time/releases/tag/1.4.7)

Published 2026-08-29.

### Added

- `Esp32NtpTime` can now run under the `roo_testing` ESP32 emulator.
- The example configures an in-process Wi‑Fi access point and uses the emulator’s host-backed time support, enabling local testing of the NTP setup flow.

### Updated

- Updated the `roo_testing` dependency to version 2.1.0.


---

# [roo_testing 1.4.6](https://github.com/dejwk/roo_time/releases/tag/1.4.6)

Published 2026-08-21.

* Testing: updated to a common asan config provided by roo_testing.

**Full Changelog**: https://github.com/dejwk/roo_time/compare/1.4.5...1.4.6

---

# [roo_time 1.4.5](https://github.com/dejwk/roo_time/releases/tag/1.4.5)

Published 2026-08-18.

Testing: upgraded to roo_testing 2.0.0; execution of examples in an emulated environment has been simplified to a single bazel command execution.

---

# [roo_time 1.4.4](https://github.com/dejwk/roo_time/releases/tag/1.4.4)

Published 2026-03-18.

Fixed compilation issues on ESP8266.

---

# [roo_time 1.4.3](https://github.com/dejwk/roo_time/releases/tag/1.4.3)

Published 2026-02-24.

* Added public documentation comments.
* Fixed behavior of rounding functions in case of negative durations.
* Fixed the != operator for DateTime.
* Minor API tweaks.

**Full Changelog**: https://github.com/dejwk/roo_time/compare/1.4.2...1.4.3

---

# [roo_time 1.4.2](https://github.com/dejwk/roo_time/releases/tag/1.4.2)

Published 2026-02-23.

Small optimization for ESP32. Now, Uptime::Now() is just an inline wrapper over the native clock API.

**Full Changelog**: https://github.com/dejwk/roo_time/compare/1.4.1...1.4.2


---

# [roo_time 1.4.1](https://github.com/dejwk/roo_time/releases/tag/1.4.1)

Published 2026-01-25.

Testing: fixed unit tests broken by changes in behavior of the recent bazel.

**Full Changelog**: https://github.com/dejwk/roo_time/compare/1.4.0...1.4.1

---

# [roo_time 1.4.0](https://github.com/dejwk/roo_time/releases/tag/1.4.0)

Published 2026-01-06.

Added compatibility for esp-idf and for Raspberry Pi Pico (RP2040-based).

**Full Changelog**: https://github.com/dejwk/roo_time/compare/1.3.3...1.4.0

---

# [roo_time 1.3.3](https://github.com/dejwk/roo_time/releases/tag/1.3.3)

Published 2025-11-12.

Updating the roo_testing dependency.

**Full Changelog**: https://github.com/dejwk/roo_time/compare/1.3.2...1.3.3

---

# [roo_time 1.3.2](https://github.com/dejwk/roo_time/releases/tag/1.3.2)

Published 2025-10-30.

Picking up the updated roo_testing dependency.

---

# [roo_time 1.3.1](https://github.com/dejwk/roo_time/releases/tag/1.3.1)

Published 2025-10-30.

* Better continuous integration, and .gitignore.

**Full Changelog**: https://github.com/dejwk/roo_time/compare/1.3.0...1.3.1

---

# [roo_time 1.3.0](https://github.com/dejwk/roo_time/releases/tag/1.3.0)

Published 2025-10-05.

* Renamed 'Interval' to 'Duration', to align terminology with the C++ standard library.
* Added address sanitizer tests.

**Full Changelog**: https://github.com/dejwk/roo_time/compare/1.2.1...1.3.0

---

# [roo_time 1.2.1](https://github.com/dejwk/roo_time/releases/tag/1.2.1)

Published 2025-08-09.

**Full Changelog**: https://github.com/dejwk/roo_time/compare/1.2.0...1.2.1

---

# [roo_time 1.2.0](https://github.com/dejwk/roo_time/releases/tag/1.2.0)

Published 2025-08-08.

Making roo_time a bazel module, for easier unit testing of depending libraries.

**Full Changelog**: https://github.com/dejwk/roo_time/compare/1.1.3...1.2.0

---

# [roo_time 1.1.3](https://github.com/dejwk/roo_time/releases/tag/1.1.3)

Published 2025-07-04.

Added rounding and floating-point functions to the Interval.

---

# [1.1.2](https://github.com/dejwk/roo_time/releases/tag/1.1.2)

Published 2025-03-24.

Changed the convention for days of week constants. Fixed tests.

---

# [](https://github.com/dejwk/roo_time/releases/tag/1.1.1)

Published 2024-12-29.

Added an enum for months.

---

# [roo_time 1.1](https://github.com/dejwk/roo_time/releases/tag/1.1)

Published 2024-11-04.

Bug fixes, adding support for breaking intervals into components and reconstituting them, moving support for DS3231 out to a separate library.



---

# [roo_time 1.0.7](https://github.com/dejwk/roo_time/releases/tag/1.0.7)

Published 2024-08-08.

Adding support for breaking intervals into components.

Adding DelayUntil(deadline).

**Full Changelog**: https://github.com/dejwk/roo_time/compare/1.0.6...1.0.7

---

# [roo_time 1.0.6](https://github.com/dejwk/roo_time/releases/tag/1.0.6)

Published 2024-01-03.

Adding a convenience function to Delay(Interval). Depending on the length of the interval, might call delay() or delayMicroseconds().

---

# [roo_time 1.0.5](https://github.com/dejwk/roo_time/releases/tag/1.0.5)

Published 2023-12-23.

Minor fixes.

---

# [roo_time 1.0.4](https://github.com/dejwk/roo_time/releases/tag/1.0.4)

Published 2023-12-17.

Added support for converting floating point values to time intervals, e.g. Seconds(2.5), etc.

**Full Changelog**: https://github.com/dejwk/roo_time/compare/1.0.3...1.0.4

---

# [roo_time 1.0.3](https://github.com/dejwk/roo_time/releases/tag/1.0.3)

Published 2023-12-07.

Improving precision of ESP32 system clock to microsecond (from second).

---

# [roo_time 1.0.2](https://github.com/dejwk/roo_time/releases/tag/1.0.2)

Published 2023-11-27.

* Made the library compatible with Arduino IDE
* Added examples

---

# [](https://github.com/dejwk/roo_time/releases/tag/1.0.1)

Published 2023-11-10.

Minor update; include library.json for PlatformIO registry.

---

# [roo_time 1.0](https://github.com/dejwk/roo_time/releases/tag/1.0)

Published 2023-06-16.

Initial release.

---

