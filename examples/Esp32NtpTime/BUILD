load("@roo_testing//roo_testing/emulation:arduino.bzl", "roo_arduino_example")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

roo_arduino_example(
    name = "Esp32NtpTime",
    sketch = "Esp32NtpTime.ino",
    deps = [
        ":emulator_support",
        "//:roo_time",
        "@roo_testing//roo_testing/frameworks/arduino-esp32/libraries/WiFi",
    ],
)

cc_test(
    name = "sketch_test",
    size = "small",
    srcs = ["test/sketch_test.cpp"],
    target_compatible_with = ["@roo_testing//roo_testing/platforms:arduino"],
    deps = [
        ":Esp32NtpTime__ino_context",
        "@roo_testing//:arduino_gtest_main",
    ],
)

# Header/API coverage for the on-device branch; this does not run real SNTP.
cc_library(
    name = "hardware_compile",
    testonly = True,
    srcs = [":Esp32NtpTime__ino_wrapper"],
    copts = ["-UROO_TESTING"],
    target_compatible_with = ["@roo_testing//roo_testing/platforms:arduino"],
    deps = [":Esp32NtpTime__ino_context"],
)

cc_library(
    name = "emulator_support",
    hdrs = ["emulator.h"],
    target_compatible_with = ["@roo_testing//roo_testing/platforms:arduino"],
    deps = [
        "@roo_testing//roo_testing/frameworks/arduino-esp32/cores/esp32",
        "@roo_testing//roo_testing/microcontrollers/esp32",
        "@roo_testing//roo_testing/transducers/wifi",
    ],
)
