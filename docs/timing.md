# Durations, deadlines, and compact timing

Use uptime for elapsed measurements and deadlines. Wall clocks can jump,
so they are not a substitute for a monotonic timing clock.

[Back to the overview](../README.md) · [Runnable examples](../examples/README.md)

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

Integer division and remainder support period calculations without extracting
raw counts:

```cpp
Duration half = Seconds(7) / 2;             // 3.5 seconds.
int64_t periods = Seconds(7) / Seconds(2);   // 3 complete periods.
Duration remainder = Seconds(7) % Seconds(2);  // 1 second.
```

Division truncates toward zero, at microsecond precision for `Duration` and
millisecond precision for `SmallDuration`. Duration ratios return `int64_t`,
including compact and mixed operands. Remainders have the dividend's sign or
are zero. Compact-only scalar division and remainder preserve `SmallDuration`;
mixed remainders widen to `Duration`. Divisors must be nonzero, scalar divisors
must be integers representable in `int64_t`, and quotients must fit their result
type. Preconditions assert in debug builds. `Duration::Min() % Micros(-1)` is
explicitly supported and returns zero; division by that value would overflow.

## Compact timing values

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

## Delay contracts

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

## Rounding semantics

Use the rounding direction that matches the task. In particular, converting a
negative timestamp to Unix seconds normally needs floor, while splitting a
signed elapsed amount into whole units often needs truncation toward zero.

| Millisecond conversion | `Micros(1501)` | `Micros(-1501)` |
| --- | --- | --- |
| `inMillis()` / `inMillisRoundedDown()` (toward zero) | 1 | -1 |
| `inMillisRoundedUp()` (away from zero) | 2 | -2 |
| `inMillisFloor()` (toward negative infinity) | 1 | -2 |
| `inMillisCeiling()` (toward positive infinity) | 2 | -1 |
| `inMillisRoundedNearest()` (nearest, ties away from zero) | 2 | -2 |

The same methods exist for seconds, minutes, and hours. At an exact half unit,
`Micros(500).inMillisRoundedNearest()` is 1 and the negative equivalent is -1.
`RoundedDown` and `RoundedUp` are legacy names for magnitude rounding; they do
not mean mathematical floor and ceiling for negative values.

## Compact timing contracts

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

## Duration factories

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

## Performance and program size

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
