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
        "src/roo_time/ds3231.h",
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
        "//roo_testing:arduino",
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
