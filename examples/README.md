# Runnable examples

Start with elapsed timing, then connect a wall clock when the application needs
real dates. Each example directory explains its purpose, requirements, output,
and build/run commands. Sketches use Serial at 115200 baud.

| Example | Purpose | Requirements |
| --- | --- | --- |
| [ElapsedTime](ElapsedTime/) | Measure elapsed work | Arduino board or host emulation |
| [Esp32NtpTime](Esp32NtpTime/) | Read an ESP32 system clock after NTP setup | ESP32 and WiFi, or the documented host shim |

The documentation/example refresh adds periodic sampling, compact debounce,
timestamped measurements, settings import/export, and a daily local schedule.
They will appear here as each runnable example lands.

Run commands from the repository root with the library and its dependencies
available. Use the [platform guide](../docs/platforms.md) for build setup.

- [ElapsedTime](ElapsedTime/README.md): Measure a sensor operation with `Uptime` subtraction, then format the resulting

- [PeriodicSampler](PeriodicSampler/README.md): Sample on a stable 100 ms cadence with `DelayUntil`, explicitly skipping missed

- [CompactDebounce](CompactDebounce/README.md): Debounce two buttons using a four-byte `SmallTimestamp` per input and a

- [Esp32NtpTime](Esp32NtpTime/README.md): Connect an ESP32 to Wi-Fi, wait for an SNTP synchronization notification,

- [TimestampedMeasurements](TimestampedMeasurements/README.md): Record sensor values with a bounded ISO timestamp buffer, falling back to
