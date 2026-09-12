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

The core types store signed 64-bit microseconds and their value operations use no
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
| `DateTime` | Calendar fields for a wall time at a fixed UTC offset | Displaying a date, hour, or day of week |

Use uptime for timing work and wall time for dates. `UtcOffset` supplies a fixed UTC
offset; synchronization, RTC communication, and daylight-saving rules are supplied
by your application or companion libraries.

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

`TimeZone` remains available as a deprecated alias for `UtcOffset`; existing
code continues to compile with a deprecation warning. The `timeZone()` accessor
and `timezone::UTC` constant retain their existing names.

Offsets are fixed; they do not automatically follow daylight-saving changes.
For an application-specific rule, see the [DST example](#daylight-saving-example).
Also, `DateTime()` means the Unix epoch, not the current time.

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
    return WallTime(Millis(rtc_.millisSinceEpoch()));
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

For standalone Pico SDK builds, compile `src/roo_time.cpp` and
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
  and `WallTime()` represent the Unix epoch, not the current time. `DateTime`
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

`WallTimeClock::now()` returns a value without validity or synchronization status.
The adapter/application must separately track whether an RTC has been initialized
or a network clock synchronized. `SystemClock` reads the system wall clock; it
does not initiate NTP synchronization. It returns the Unix epoch if
`gettimeofday` fails, which is indistinguishable from a successful epoch reading.

Wall time can jump forward or backward after synchronization or manual adjustment.
Use `Uptime` for elapsed-time measurement and deadlines. Fixed offsets do not
handle DST transitions, ambiguous local times, or changes to timezone rules; the
[DST example](#daylight-saving-example) supplies an application-specific rule.

### Rounding semantics

`Duration` narrowing conversions expose three rounding modes:

- `inXxx()` and `inXxxRoundedDown()` round **toward zero**.
- `inXxxRoundedUp()` rounds **away from zero**.
- `inXxxRoundedNearest()` rounds to nearest, with ties (exact half) **away from zero**.

Examples for milliseconds:

- `Micros(1501).inMillisRoundedDown()` is `1`, `Micros(-1501).inMillisRoundedDown()` is `-1`.
- `Micros(1501).inMillisRoundedUp()` is `2`, `Micros(-1501).inMillisRoundedUp()` is `-2`.
- `Micros(500).inMillisRoundedNearest()` is `1`, `Micros(-500).inMillisRoundedNearest()` is `-1`.

### Daylight-saving example

Applications can choose a fixed offset based on the timestamp. This example
implements UTC+01:00 in winter and UTC+02:00 in summer, with transitions at 01:00
UTC on the last Sundays of March and October. It illustrates one rule rather
than providing a geographic timezone database or historical rule changes.

```cpp
Duration utcOffset(WallTime t) {
  int16_t y = DateTime(t, timezone::UTC).year();
  // Figure out the day of the week of the last day of March that year.
  DateTime mar31(y, 3, 31, timezone::UTC);
  // Figure out the down-offset from mar31 to the 2AM last Sunday of March.
  // Keep in mind that 2AM is 1AM UTC.
  DayOfWeek march31dow = mar31.dayOfWeek();
  WallTime summerStart = mar31.wallTime() - Hours(24 * march31dow) + Hours(1);
  // Similar calculation for the winter time. Note that 3AM is now 1AM UTC.
  DateTime oct31(y, 10, 31, timezone::UTC);
  DayOfWeek oct31dow = oct31.dayOfWeek();
  WallTime summerEnd = oct31.wallTime() - Hours(24 * oct31dow) + Hours(1);
  // Now, see if the specified time point is within the summer time range.
  return t >= summerStart && t < summerEnd ? Hours(2) : Hours(1);
}

class DSTWatch {
 public:
  DSTWatch(WallTimeClock& clock) : clock_(clock) {}

  DateTime nowLocal() {
    WallTime t = clock_.now();
    return DateTime(t, UtcOffset(utcOffset(t)));
  }

 private:
  WallTimeClock& clock_;
};
```

### Compact timing contracts

`SmallDuration` stores signed 32-bit milliseconds; `SmallTimestamp` stores an
opaque unsigned 32-bit millisecond timestamp. Each occupies four bytes:

`Uptime` converts implicitly to `SmallTimestamp`, truncating to milliseconds and
retaining the low 32 bits. `SmallTimestamp::Now()` uses that same conversion.
There is no absolute-time accessor or recoverable epoch on a compact timestamp.
The clock backend's existing sampling and concurrency requirements still apply.

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
returns `Duration`. Compact multiplication by an integer stays compact.

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
