# BUILD file for use with https://github.com/dejwk/roo_testing.

cc_library(
    name = "roo_time",
    includes = [
        ".",
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
        "roo_time.cpp",
        "roo_time.h",
    ],
    defines = ["ROO_EMULATOR"],
    includes = [
        ".",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing:arduino",
    ],
)

cc_library(
    name = "default_uptime_now",
    srcs = [
        "uptime_now.cpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":core",
    ],
)
