load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_python//python:defs.bzl", "py_library", "py_test")
load("//test:regression.bzl", "doc_check_test", "regression_test")

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
    "mbff_hier",
    "mt_invariance01",
    "mbff_orig_name",
    "mbff_clock_power_off",
    "mbff_clock_power_on",
    "nograd01",
    "simple01",
    "simple01-obs",
    "simple01-rd",
    "simple01-ref",
    "simple01-skip-io",
    "simple01-td",
    "simple01-td-net-percentage",
    "simple01-td-tune",
    "simple01-td-repair",
    "simple01-uniform",
    "simple01-vcts",
    "simple01-vcts-clamp",
    "simple01-vcts-noclk",
    "simple01-vcts-userlat",
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

PASSFAIL_TESTS = [
    "incremental02",
    "place_ios01",
    "place_ios_top_layer",
]

# Longer-running regression tests (see regression_tests_large.tcl). These are
# tagged "manual" so they are excluded from the default `bazel test //...`
# wildcard and must be requested explicitly, e.g. via the :large_tests suite.
LARGE_TESTS = [
    "macro01",
    "macro02",
    "macro03",
    "medium01",
    "medium02",
    "medium03",
    "medium04",
    "medium05",
    "medium06",
    "large01",
    "large02",
]

ALL_TESTS = TESTS + PASSFAIL_TESTS

py_library(
    name = "helpers",
    srcs = ["helpers.py"],
    imports = ["."],
    deps = [
        "//python/openroad:openroadpy",
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
            for test in ALL_TESTS + LARGE_TESTS
            for ext in [
                "tcl",
                "py",
            ]
        ],
    ),
)

[regression_test(
    name = test_name,
    check_log = False if test_name in PASSFAIL_TESTS else True,
    check_passfail = True if test_name in PASSFAIL_TESTS else False,
    data = [":test_resources"],
) for test_name in ALL_TESTS]

[regression_test(
    name = test_name,
    size = "enormous",
    data = [":test_resources"],
    tags = ["manual"],
) for test_name in LARGE_TESTS]

# Convenience suite to run all long-running tests explicitly:
#   bazel test //src/gpl/test:large_tests
test_suite(
    name = "large_tests",
    tags = ["manual"],
    tests = [":" + test_name + "-tcl_test" for test_name in LARGE_TESTS],
)

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
    "simple01-td-net-percentage",
    "simple01-td-tune",
    "simple01-td-repair",
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
            "//python/openroad:openroadpy",
        ],
    )
    for test_name in PY_TESTS
]

cc_test(
    name = "gpl_fft_unittest",
    srcs = ["fft_test.cc"],
    linkstatic = True,  # TODO: remove once deps define all symbols
    deps = [
        "//src/gpl",  # buildcleaner: keep: needs fixing gpl layering first.
        "//src/gpl:gpl_private_hdrs",
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
        "//src/ant",
        "//src/dbSta:dbNetwork",
        "//src/dbSta:dbReadVerilog",
        "//src/dpl",
        "//src/est",
        "//src/gpl",
        "//src/grt",
        "//src/odb/src/db",
        "//src/rsz",
        "//src/stt",
        "//src/tst",
        "//src/utl",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "placer_base_test",
    srcs = ["placer_base_test.cpp"],
    deps = [
        "//src/gpl",
        "//src/gpl:gpl_private_hdrs",
        "//src/odb/src/db",
        "//src/tst:db_fixture",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

py_test(
    name = "gpl_man_tcl_check",
    srcs = ["gpl_man_tcl_check.py"],
    data = ["//src/gpl:doc_files"],
    tags = ["doc_check"],
    deps = ["//docs/src/scripts:extract_utils"],
)

doc_check_test(
    name = "gpl_readme_msgs_check",
    data = [
        "//src/gpl:doc_files",
        "//src/gpl:messages_txt",
    ],
    visibility = ["//visibility:public"],
)

doc_check_test(
    name = "gpl_messages_txt_check",
    check_log = False,
    data = ["//src/gpl:messages_txt"],
)
