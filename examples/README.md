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
