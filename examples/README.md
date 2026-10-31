# Runnable examples

Start with elapsed timing. Add a wall clock when your application needs actual
dates, and keep local schedule settings separate from resolved instants.
Every sketch uses Serial at **115200 baud**. Follow each example's README for
requirements, commands, expected output, and emulator limitations.

| Example | What it demonstrates | Hardware |
| --- | --- | --- |
| [ElapsedTime](ElapsedTime/README.md) | Measure sensor work; distinguish duration from uptime | Any supported Arduino board |
| [PeriodicSampler](PeriodicSampler/README.md) | Stable cadence and an explicit overrun policy | Any supported Arduino board |
| [CompactDebounce](CompactDebounce/README.md) | Independent button state with compact timestamps; scripted bounce trace | Any supported Arduino board |
| [Esp32NtpTime](Esp32NtpTime/README.md) | Wi-Fi timeout, confirmed SNTP readiness, UTC and seasonal local display | ESP32 and Wi-Fi credentials |
| [TimestampedMeasurements](TimestampedMeasurements/README.md) | Bounded ISO records with uptime fallback; scripted clock readiness | Any supported Arduino board |
| [ScheduleSettings](ScheduleSettings/README.md) | Strict parsing and atomic replacement of date/time settings | Any supported Arduino board |
| [DailyLocalSchedule](DailyLocalSchedule/README.md) | Daily calendar planning with explicit gap/overlap policies | Any supported Arduino board |

All seven compile and run under roo_testing's Arduino ESP32 host profile.
The tests include each actual sketch and call a bounded number of setup/loop
iterations; they terminate automatically. Test sources live under `test/`,
which Arduino IDE does not compile as part of a sketch. Examples with scripted inputs run
the same trace on a board. The NTP host shim uses host time, without exchanging
NTP packets. Host validation does not replace testing Wi-Fi/SNTP on an ESP32.

From the repository root:

```sh
bazel build //examples/... --config=roo_testing_arduino_esp32
bazel test //examples/... --config=roo_testing_arduino_esp32
bazel run //examples/ElapsedTime:ElapsedTime --config=roo_testing_arduino_esp32
```

The runnable command continues until interrupted. Add `--config=asan` to build
or test with AddressSanitizer. Arduino sketches are excluded from the ESP-IDF
profile; the library tests run in both profiles. See the
[platform guide](../docs/platforms.md) for setup and clock requirements.
