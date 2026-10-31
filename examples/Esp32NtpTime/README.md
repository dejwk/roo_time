# Esp32NtpTime

Connect an ESP32 to Wi-Fi, wait for an SNTP synchronization notification,
then display UTC and a local recurring EU summer-time offset.

Open `Esp32NtpTime.ino` in Arduino IDE with roo_time installed, select your board,
and open Serial Monitor at **115200 baud**. This sketch requires an ESP32 and Wi-Fi credentials.

From the library root, run the emulated sketch or its bounded smoke test:

```sh
bazel run //examples/Esp32NtpTime:Esp32NtpTime --config=roo_testing_arduino_esp32
bazel test //examples/Esp32NtpTime:sketch_test --config=roo_testing_arduino_esp32
```

The run continues until interrupted; the test terminates automatically.

On hardware, set credentials first. Wi-Fi has a 10-second timeout and SNTP
a 20-second timeout. An SNTP notification confirms the first sync; it does not
guarantee accuracy or future freshness. The sketch keeps showing time if Wi-Fi
later disconnects. Restart after a Wi-Fi connection timeout.

On the host, the in-process access point supplies Wi-Fi connectivity. The device
clock begins near the Unix epoch; roo_testing's virtual SNTP server supplies
host UTC after a response delay and calls the registered callback. The sketch
prints `SNTP synchronization confirmed.`, then `UTC:` and `Local:` timestamps.
Only Wi-Fi setup/credentials are emulator-specific; there is no manually invoked
readiness callback in the fixture and no NTP packet exchange.

During coordinated development, use the local roo_testing checkout containing
SNTP emulation (the published dependency is updated after its release):

```sh
bazel test //examples/Esp32NtpTime:sketch_test --config=roo_testing_arduino_esp32 \
  --override_module=roo_testing=/path/to/roo_testing
```

The recurring pattern uses UTC+1 in winter and UTC+2 in summer. It is a
proleptic rule, not an IANA zone with historical or future government changes.
