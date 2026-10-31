# ScheduleSettings

Import and export a date and local start time as device settings, validating
the entire edit before replacing the saved values.

Open `ScheduleSettings.ino` in Arduino IDE with roo_time installed, select your board,
and open Serial Monitor at **115200 baud**. No additional hardware is required.

From the library root, run the emulated sketch or its bounded smoke test:

```sh
bazel run //examples/ScheduleSettings:ScheduleSettings --config=roo_testing_arduino_esp32
bazel test //examples/ScheduleSettings:sketch_test --config=roo_testing_arduino_esp32
```

The run continues until interrupted; the test terminates automatically.

The sketch prints `first_day=2026-10-25 start=02:30`, rejects February 30,
then prints the same retained settings. Tests also reject `24:00` and trailing
input without changing either field. Pass explicit input lengths from your
request parser; no NUL terminator is needed for bounded inputs.

These are local calendar settings. They do not identify an instant until you
select a zone and resolve gaps or overlaps. See
[DailyLocalSchedule](../DailyLocalSchedule/README.md) for that step.
