# CompactDebounce

Debounce two buttons using a four-byte `SmallTimestamp` per input and a
scripted bounce trace, including a test across the 32-bit millisecond rollover.

Open `CompactDebounce.ino` in Arduino IDE with roo_time installed, select your board,
and open Serial Monitor at **115200 baud**. No additional hardware is required.

From the library root, run the emulated sketch or its bounded smoke test:

```sh
bazel run //examples/CompactDebounce:CompactDebounce --config=roo_testing_arduino_esp32
bazel test //examples/CompactDebounce:sketch_test --config=roo_testing_arduino_esp32
```

The run continues until interrupted; the test terminates automatically.

The trace accepts button 1 at 23 ms, button 0 at 25 ms, and its release at
50 ms. In firmware, use `SmallTimestamp::Now()` for every input; do not mix
clock domains. Poll within 2^31 ms of the last state change. If inputs can be
idle longer, reinitialize their debounce state before resuming.

The test constructs timestamps near rollover so it does not need to wait
49.7 days. Compact timestamps save RAM but cannot represent calendar time
or arbitrarily long intervals.
