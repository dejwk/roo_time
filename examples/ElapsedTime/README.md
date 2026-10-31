# ElapsedTime

Measure a sensor operation with `Uptime` subtraction, then format the resulting
`Duration` separately from the uptime timestamp.

Open `ElapsedTime.ino` in Arduino IDE with roo_time installed, select your board,
and open Serial Monitor at **115200 baud**. No additional hardware is required.

From the library root, run the emulated sketch or its bounded smoke test:

```sh
bazel run //examples/ElapsedTime:ElapsedTime --config=roo_testing_arduino_esp32
bazel test //examples/ElapsedTime:sketch_test --config=roo_testing_arduino_esp32
```

The run continues until interrupted; the test terminates automatically.

Output resembles `S+00:00:00.025123 sensor read took 00:00:00.025`.
Exact timestamps and elapsed times depend on the scheduler.
