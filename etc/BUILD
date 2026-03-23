load("@rules_python//python:defs.bzl", "py_binary", "py_library", "py_test")

package(features = ["layering_check"])

exports_files(
    [
        "whittle.py",
        "whittle_cleanup.tcl",
        "whittle_cut.tcl",
    ],
    visibility = ["//visibility:public"],
)

py_binary(
    name = "file_to_string",
    srcs = [
        "file_to_string.py",
    ],
    main = "file_to_string.py",
    visibility = ["//visibility:public"],
)

test_suite(
    name = "all_tests",
    tests = [
        ":test_delta_debug",
        ":test_whittle",
    ],
)

py_library(
    name = "delta_debug_lib",
    srcs = ["deltaDebug.py"],
    imports = ["."],
)

py_test(
    name = "test_delta_debug",
    srcs = ["test_deltaDebug.py"],
    main = "test_deltaDebug.py",
    deps = [":delta_debug_lib"],
)

py_test(
    name = "test_whittle",
    srcs = ["test_whittle.py"],
    deps = [":whittle_lib"],
)

py_library(
    name = "whittle_lib",
    srcs = ["whittle.py"],
    imports = ["."],
)
