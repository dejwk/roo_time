# Platforms, builds, and clock origins

Clock origins, sleep accounting, and resolution belong to the selected
backend. Stored microseconds do not imply microsecond wakeup accuracy.
The runnable sketches are Arduino examples; the library also supports ESP-IDF.

[Back to the overview](../README.md) · [Runnable examples](../examples/README.md)

## Host emulation

Host builds support both Arduino and ESP-IDF through roo_testing. With
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

## Clock backends

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
Add `src/roo_time/format.cpp` when using text formatting or parsing. Buffer-only
builds can disable the optional string/view overloads; view overloads also need
`roo_backport` on the include path.
The SDK supplies `PICO_ON_DEVICE`; compatible RP2040 Arduino cores are detected
through `ARDUINO_ARCH_RP2040` and availability of `pico/time.h`. Pico waits use
`sleep_us`, allowing the SDK to manage lower-power waits; its normal interrupt
and timer configuration requirements apply.
