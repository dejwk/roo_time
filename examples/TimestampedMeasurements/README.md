# TimestampedMeasurements

Record sensor values with a bounded ISO timestamp buffer, falling back to
explicitly labeled uptime until a controllable wall clock becomes ready.

Open `TimestampedMeasurements.ino` in Arduino IDE with roo_time installed, select your board,
and open Serial Monitor at **115200 baud**. No additional hardware is required.

From the library root, run the emulated sketch or its bounded smoke test:

```sh
bazel run //examples/TimestampedMeasurements:TimestampedMeasurements --config=roo_testing_arduino_esp32
bazel test //examples/TimestampedMeasurements:sketch_test --config=roo_testing_arduino_esp32
```

The run continues until interrupted; the test terminates automatically.

The two records have values 420 and 421. The first starts with
`source=uptime timestamp=S+00:00:00.250000`; the second starts with
`source=utc timestamp=2026-06-15T12:30:00.123456+00:00`.

The scripted clock only supplies trusted, supported calendar values. A real
provider must establish readiness and the years 1–9999 range before supplying
wall time; `WallTime::isSet()` alone only means the value is not the unset
sentinel. Keep uptime too if records must be ordered across wall-clock corrections.
