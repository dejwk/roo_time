# BUILD file for use with https://github.com/dejwk/roo_testing.

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "roo_time",
    includes = [
        "src",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":core",
        ":default_uptime_now",
    ],
)

cc_library(
    name = "core",
    srcs = [
        "src/roo_time.cpp",
        "src/roo_time.h",
    ],
    includes = [
        "src",
    ],
    visibility = ["//visibility:public"],
    deps = [
    ],
)

cc_library(
    name = "default_uptime_now",
    srcs = [
        "src/uptime_now.cpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":core",
    ] + select({
        "@roo_testing//roo_testing/platforms:is_roo_testing": [
            "@roo_testing//roo_testing/system:timer",
        ],
        "//conditions:default": [],
    }),
)

cc_library(
    name = "linux_uptime_now",
    srcs = [
        "src/uptime_now.cpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":core",
    ] + select({
        "@roo_testing//roo_testing/platforms:is_roo_testing": [
            "@roo_testing//roo_testing/system:timer",
        ],
        "//conditions:default": [],
    }),
)

cc_test(
    name = "roo_time_test",
    size = "small",
    srcs = [
        "test/roo_time_test.cpp",
    ],
    copts = ["-Iexternal/gtest/include"],
    includes = ["src"],
    linkstatic = 1,
    deps = [
        ":roo_time",
        "@googletest//:gtest_main",
    ],
)

# Compile the production backend inside each test after selecting its platform.
cc_library(
    name = "uptime_test_source",
    textual_hdrs = ["src/uptime_now.cpp"],
    testonly = True,
    deps = [":core"],
)

cc_test(
    name = "arduino_uptime_test",
    size = "small",
    srcs = ["test/arduino_uptime_test.cpp", "test/stubs/Arduino.h"],
    includes = ["test/stubs"],
    deps = [":uptime_test_source", "@googletest//:gtest_main"],
)

cc_test(
    name = "linux_uptime_test",
    size = "small",
    srcs = ["test/linux_uptime_test.cpp"],
    deps = [":uptime_test_source", "@googletest//:gtest_main"],
)

[
    cc_test(
        name = name,
        size = "small",
        srcs = ["test/delay_test.cpp"] + glob(["test/stubs/**/*.h"]),
        copts = ["-D" + backend],
        includes = ["test/stubs"],
        deps = [":uptime_test_source", "@googletest//:gtest_main"],
    )
    for name, backend in [
        ("idf_delay_test", "TEST_IDF_DELAY"),
        ("idf_16bit_delay_test", "TEST_16BIT_TICKS"),
        ("generic_arduino_delay_test", "TEST_GENERIC_ARDUINO_DELAY"),
        ("arduino_esp32_delay_test", "TEST_ARDUINO_ESP32_DELAY"),
        ("emulated_delay_test", "TEST_EMULATED_DELAY"),
    ]
]

cc_test(
    name = "compact_time_test",
    size = "small",
    srcs = ["test/compact_time_test.cpp"],
    deps = [":roo_time", "@googletest//:gtest_main"],
)

[
    cc_test(
        name = name,
        size = "small",
        srcs = ["test/pico_time_test.cpp", "test/stubs/pico/time.h"],
        copts = copts,
        includes = ["test/stubs"],
        deps = [":uptime_test_source", "@googletest//:gtest_main"],
    )
    for name, copts in [
        ("pico_time_test", []),
        ("pico_arduino_time_test", ["-DTEST_PICO_ARDUINO"]),
    ]
]
