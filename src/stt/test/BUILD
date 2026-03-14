load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//test:regression.bzl", "regression_test")

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
    name = "stt_flute_test",
    srcs = ["cpp/TestFlute.cc"],
    deps = [
        "//src/stt",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "stt_pd_test",
    srcs = ["cpp/TestPrimDijkstra.cc"],
    deps = [
        "//src/stt",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "stt_builder_test",
    srcs = ["cpp/TestSteinerTreeBuilder.cc"],
    deps = [
        "//src/odb",
        "//src/stt",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "stt_thread_safety_test",
    srcs = ["cpp/TestThreadSafety.cc"],
    deps = [
        "//src/odb",
        "//src/stt",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
