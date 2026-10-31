load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@roo_testing//roo_testing/emulation:arduino.bzl", "roo_arduino_example")

roo_arduino_example(
    name = "Esp32NtpTime",
    sketch = "Esp32NtpTime.ino",
    deps = [
        "//:roo_time",
        "@roo_testing//roo_testing/microcontrollers/esp32",
        "@roo_testing//roo_testing/transducers/wifi:wifi",
    ],
)

cc_test(
    name = "sketch_test",
    size = "small",
    srcs = ["test.cpp"],
    target_compatible_with = ["@roo_testing//roo_testing/platforms:arduino"],
    deps = [":Esp32NtpTime__ino_context", "@roo_testing//:arduino_gtest_main"],
)
