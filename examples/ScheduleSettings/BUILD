load("@roo_testing//roo_testing/emulation:arduino.bzl", "roo_arduino_example")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

roo_arduino_example(
    name = "ScheduleSettings",
    sketch = "ScheduleSettings.ino",
    deps = [
        "//:roo_time",
    ],
)

cc_test(
    name = "sketch_test",
    size = "small",
    srcs = ["test/sketch_test.cpp"],
    target_compatible_with = ["@roo_testing//roo_testing/platforms:arduino"],
    deps = [
        ":ScheduleSettings__ino_context",
        "@roo_testing//:arduino_gtest_main",
    ],
)
