# DailyLocalSchedule

Plan a daily 02:30 local operation across seasonal clock changes: advance
`CivilDay`, skip nonexistent times, and choose the earlier ambiguous occurrence.

Open `DailyLocalSchedule.ino` in Arduino IDE with roo_time installed, select your board,
and open Serial Monitor at **115200 baud**. No additional hardware is required.

From the library root, run the emulated sketch or its bounded smoke test:

```sh
bazel run //examples/DailyLocalSchedule:DailyLocalSchedule --config=roo_testing_arduino_esp32
bazel test //examples/DailyLocalSchedule:sketch_test --config=roo_testing_arduino_esp32
```

The run continues until interrupted; the test terminates automatically.

Spring output skips `2026-03-29`. Autumn output chooses
`2026-10-25T00:30:00.000000+00:00` for the repeated local 02:30.
The same local time can be separated by 23 or 25 hours on adjacent dates;
adding `Hours(24)` to the previous instant changes the intended local time.

This is a deterministic planning example, not a running wall-clock scheduler.
Firmware also needs a synchronized clock, a policy for missed runs and clock
corrections, and persisted run tracking if restarts must not repeat an operation.
The selected EU rules are recurring patterns, not a historical zone database.
`PlanDay` distinguishes a deliberate gap skip from a resolution error.
