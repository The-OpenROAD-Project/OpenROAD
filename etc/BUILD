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

exports_files(
    [
        "find_dup_ids.sh",
        "find_messages.py",
    ],
    visibility = ["//visibility:public"],
)

test_suite(
    name = "all_tests",
    tests = [
        ":test_whittle",
    ],
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
