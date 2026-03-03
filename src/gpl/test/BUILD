load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_python//python:defs.bzl", "py_library", "py_test")
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

TESTS = [
    "ar01",
    "ar02",
    "clust01",
    "clust02",
    "clust03",
    "cluster_place01",
    "convergence01",
    "core01",
    "density01",
    "diverge01",
    "error01",
    "incremental01",
    "incremental02",
    "nograd01",
    "simple01",
    "simple01-obs",
    "simple01-rd",
    "simple01-ref",
    "simple01-skip-io",
    "simple01-td",
    "simple01-td-tune",
    "simple01-uniform",
    "simple02",
    "simple02-rd",
    "simple03",
    "simple03-rd",
    "simple04",
    "simple04-rd",
    "simple05",
    "simple06",
    "simple07",
    "simple08",
    "simple09",
    "simple10",
    "region01",
]

py_library(
    name = "helpers",
    srcs = ["helpers.py"],
    imports = ["."],
    deps = [
        "//openroad:openroadpy",
    ],
)

filegroup(
    name = "asap7pdk",
    srcs = glob([
        "asap7/*.lib.gz",
        "asap7/*.lib",
        "asap7/*.lef",
        "asap7/*.tcl",
    ]),
)

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

# TODO: Enable once difference between bazel and ctest is resolved.
MANUAL_FOR_BAZEL_TESTS = [
    "incremental02",
]

[regression_test(
    name = test_name,
    data = [":test_resources"],
    tags = ["manual"] if test_name in MANUAL_FOR_BAZEL_TESTS else [],
) for test_name in TESTS]

PY_TESTS = [
    "ar01",
    "ar02",
    "convergence01",
    "core01",
    "error01",
    "incremental01",
    "nograd01",
    "simple01",
    "simple01-obs",
    "simple01-ref",
    "simple01-skip-io",
    "simple01-td",
    "simple01-td-tune",
    "simple01-uniform",
    "simple02",
    "simple03",
    "simple04",
    "simple05",
    "simple07",
    "simple08",
    "simple09",
    "simple10",
]

[
    py_test(
        name = test_name,
        srcs = [test_name + ".py"],
        data = [
            ":test_resources",
        ],
        deps = [
            ":helpers",
            "//openroad:openroadpy",
        ],
    )
    for test_name in PY_TESTS
]

cc_test(
    name = "gpl_fft_unittest",
    srcs = ["fft_test.cc"],
    features = ["-layering_check"],  # TODO: includes private headers
    linkstatic = True,  # TODO: remove once deps define all symbols
    deps = [
        "//src/gpl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@spdlog",
    ],
)

cc_test(
    name = "mbff_test",
    srcs = ["mbff_test.cpp"],
    data = [
        "library/test/test0.lef",
        "library/test/test0.lib",
    ],
    deps = [
        "//:openroad_lib",
        "//bazel:runfiles",
        "//src/ant",
        "//src/dbSta",
        "//src/dbSta:dbReadVerilog",
        "//src/dpl",
        "//src/est",
        "//src/gpl",
        "//src/grt",
        "//src/odb",
        "//src/rsz",
        "//src/stt",
        "//src/tst",
        "//src/utl",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
