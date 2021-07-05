cc_library(
    name = "roo_time",
    visibility = ["//visibility:public"],
    srcs = glob(
        [
            "**/*.cpp",
            "**/*.h",
        ],
        exclude = ["test/**"],
    ),
    includes = [
        ".",
    ],
    defines = [ "ROO_EMULATOR" ],
    deps = [
        "//roo_testing:arduino",
    ],
)

cc_test(
    name = "roo_time_test",
    srcs = [
        "test/roo_time_test.cpp",
    ],
    copts = ["-Iexternal/gtest/include"],
    linkstatic = 1,
    deps = [
        "//lib/roo_time",
        "@gtest//:gtest_main",
        "//roo_testing/transducers/time:default_system_clock",
    ],
)
