# BUILD file for use with https://github.com/dejwk/roo_testing.

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
