# BUILD file for use with https://github.com/dejwk/roo_testing.

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
    defines = ["ROO_TESTING"],
    visibility = ["//visibility:public"],
    deps = [
        ":core",
        "@roo_testing//roo_testing/system:timer",
    ],
)

cc_library(
    name = "linux_uptime_now",
    srcs = [
        "src/uptime_now.cpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":core",
    ],
)

cc_test(
    name = "roo_time_test",
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
    size = "small",
)
