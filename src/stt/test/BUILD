load("@rules_python//python:defs.bzl", "py_test")
load("//test:regression.bzl", "doc_check_test", "regression_test")

package(features = ["layering_check"])

TESTS = [
    "check",
    "flute1",
    "flute_gcd",
    "parse_clocks",
    "pd1",
    "pd2",
    "pd_gcd",
]

filegroup(
    name = "test_resources",
    # overly broad glob, could be refined later, but
    # symlinks are cheap and OpenROAD binary changes, the common
    # use case is that all tests have to be re-run.
    srcs = glob(
        ["**/*"],
        exclude = [
            test + "." + ext
            for test in TESTS
            for ext in [
                "tcl",
                "py",
            ]
        ],
    ),
)

[regression_test(
    name = test_name,
    data = [":test_resources"],
) for test_name in TESTS]

py_test(
    name = "stt_man_tcl_check",
    srcs = ["stt_man_tcl_check.py"],
    data = ["//src/stt:doc_files"],
    tags = ["doc_check"],
    deps = ["//docs/src/scripts:extract_utils"],
)

doc_check_test(
    name = "stt_readme_msgs_check",
    data = [
        "//src/stt:doc_files",
        "//src/stt:messages_txt",
    ],
    visibility = ["//visibility:public"],
)
