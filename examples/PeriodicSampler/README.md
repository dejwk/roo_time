# PeriodicSampler

Sample on a stable 100 ms cadence with `DelayUntil`, explicitly skipping missed
deadlines when a sensor read takes too long.

Open `PeriodicSampler.ino` in Arduino IDE with roo_time installed, select your board,
and open Serial Monitor at **115200 baud**. No additional hardware is required.

From the library root, run the emulated sketch or its bounded smoke test:

```sh
bazel run //examples/PeriodicSampler:PeriodicSampler --config=roo_testing_arduino_esp32
bazel test //examples/PeriodicSampler:sketch_test --config=roo_testing_arduino_esp32
```

The run continues until interrupted; the test terminates automatically.

Output starts with `sample 1`, `sample 2`, then a skipped-sample message.
Work takes 25 ms and 175 ms alternately. Scheduler delays can skip more samples.
Scheduling against the previous deadline prevents work time from accumulating
as drift; missed readings are deliberately discarded.
