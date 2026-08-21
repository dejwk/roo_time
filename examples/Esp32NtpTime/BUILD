load("@roo_testing//roo_testing/emulation:arduino.bzl", "roo_arduino_example")

roo_arduino_example(
    name = "Esp32NtpTime",
    sketch = "Esp32NtpTime.ino",
    visibility = ["//visibility:public"],
    deps = [
        "//:roo_time",
        "@roo_testing//roo_testing/microcontrollers/esp32",
        "@roo_testing//roo_testing/transducers/wifi:wifi",
    ],
)
