cc_library(
    name = "roo_time",
    visibility = ["//visibility:public"],
    srcs = glob(
        [
            "src/**/*.cpp",
            "src/**/*.h",
        ],
    ),
    hdrs = ["src/roo_time.h"],
    includes = ["src"],
)

cc_test(
    name = "roo_time_test",
    srcs = [
        "test/roo_time_test.cpp",
    ],
    includes = ["src"],
    copts = ["-Iexternal/gtest/include"],
    linkstatic = 1,
    deps = [
        ":roo_time",
        "@gtest//:gtest_main",
    ],
)
