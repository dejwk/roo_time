cc_library(
    name = "roo_time",
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = [
        ":core",
        ":default_uptime_now",
    ],
)

# Use instead of :roo_time when want to provide custom implementation of
# Uptime::Now().
cc_library(
    name = "core",
    srcs = ["src/roo_time.cpp"],
    hdrs = ["src/roo_time.h"],
    includes = ["src"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "default_uptime_now",
    srcs = ["src/uptime_now.cpp"],
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
)
