load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@roo_testing//roo_testing/emulation:arduino.bzl", "roo_arduino_example")

roo_arduino_example(
    name = "TimestampedMeasurements",
    sketch = "TimestampedMeasurements.ino",
    deps = [
        "//:roo_time",
    ],
)

cc_test(
    name = "sketch_test",
    size = "small",
    srcs = ["test.cpp"],
    target_compatible_with = ["@roo_testing//roo_testing/platforms:arduino"],
    deps = [":TimestampedMeasurements__ino_context", "@roo_testing//:arduino_gtest_main"],
)
