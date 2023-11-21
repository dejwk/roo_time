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
        "src/roo_time/system.h",
    ],
    defines = ["ROO_EMULATOR"],
    includes = [
        "src",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing:arduino",
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
    ],
)
