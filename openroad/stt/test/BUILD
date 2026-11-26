load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//bazel:man_tcl_checker.bzl", "man_tcl_check_test")
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

cc_test(
    name = "TestNeighbors",
    srcs = ["cpp/TestNeighbors.cpp"],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/stt:pdr_neighbors",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestSteinerTreeBuilder",
    srcs = ["cpp/TestSteinerTreeBuilder.cpp"],
    deps = [
        "//openroad/stt",
        "//openroad/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

man_tcl_check_test(
    name = "stt_man_tcl_check",
    module = "stt",
)

doc_check_test(
    name = "stt_readme_msgs_check",
    data = [
        "//openroad/stt:doc_files",
        "//openroad/stt:messages_txt",
    ],
    visibility = ["//visibility:public"],
)
