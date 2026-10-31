# RTC and standard-library integrations

Adapters connect application clocks and external libraries to roo_time value
types. The core library does not configure NTP, manage RTC hardware, or run
a scheduler.

[Back to the overview](../README.md) · [Runnable examples](../examples/README.md)

## Connecting an RTC

An RTC adapter implements `WallTimeClock::now()` and returns a UTC timestamp.
Return `WallTime::Unset()` when a read fails or the RTC reports invalid calendar
state. Track initialization/synchronization separately from successful reads.
For a DS3231, the companion [roo_time_ds3231](https://github.com/dejwk/roo_time_ds3231)
library provides an adapter.

When a driver supplies calendar fields, call `DateTime::TryFromComponents`
with `timezone::UTC` and return the validated snapshot's `wallTime()`. When a
driver supplies epoch counts, validate the count against the unit factory's
range before converting it. Do not add a display offset to a UTC RTC reading.
A driver that stores local calendar fields needs an explicit timezone policy,
including how to handle repeated or nonexistent local times.

The adapter's `now() const` contract does not imply that the device bus is
thread-safe. Serialize driver access as required. An I/O read can mutate driver
state; use a const-correct driver interface or explicitly owned mutable state
with synchronization rather than casting away constness.

For application code, read once, check `isSet()`, then use or display that
snapshot. An unset value must not participate in arithmetic or conversion to
`DateTime`. Reading a clock does not synchronize it or establish its accuracy.
See the [examples index](../examples/README.md) for runnable clock scenarios.

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
