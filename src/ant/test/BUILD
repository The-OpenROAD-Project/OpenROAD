load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_python//python:defs.bzl", "py_test")
load("//test:regression.bzl", "doc_check_test", "regression_test")

package(features = ["layering_check"])

# From CMakeLists.txt or_integration_tests(TESTS
COMPULSORY_TESTS = [
    "check_grt1",
]

ALL_TESTS = COMPULSORY_TESTS

filegroup(
    name = "regression_resources",
    # Dependencies could be specified more narrowly per test case,
    # but at least it is not a glob of everything and there are
    # per test glob patterns below.
    srcs = [
        "helpers.tcl",
    ],
)

[
    filegroup(
        name = test_name + "_resources",
        srcs = [":regression_resources"] + glob(
            [
                test_name + ".*",
            ],
        ) + {
            "check_grt1": [
                "gcd_sky130.def",
                "sky130hs/sky130hs_tt.lib",
                "sky130hs/sky130hs_std_cell.lef",
                "sky130hs/sky130hs.tlef",
            ],
        }.get(test_name, []),
    )
    for test_name in ALL_TESTS
]

[
    regression_test(
        name = test_name,
        data = [":" + test_name + "_resources"],
        tags = [] if test_name in COMPULSORY_TESTS else ["manual"],
        visibility = ["//visibility:public"],
    )
    for test_name in ALL_TESTS
]

# Binding tests: pass/fail checks, no golden log.
regression_test(
    name = "check_antennas_cmd",
    check_log = False,
    check_passfail = True,
    data = [
        "ant_check.def",
        "ant_check.lef",
        ":regression_resources",
    ],
)

py_test(
    name = "ant_api_test",
    size = "small",
    srcs = ["ant_api.py"],
    data = [
        "ant_check.def",
        "ant_check.lef",
    ],
    main = "ant_api.py",
    deps = ["//python/openroad:openroadpy"],
)

py_test(
    name = "ant_man_tcl_check",
    srcs = ["ant_man_tcl_check.py"],
    data = ["//src/ant:doc_files"],
    tags = [
        "doc_check",
        "py",
    ],
    deps = ["//docs/src/scripts:extract_utils"],
)

doc_check_test(
    name = "ant_readme_msgs_check",
    data = [
        "//src/ant:doc_files",
        "//src/ant:messages_txt",
    ],
    visibility = ["//visibility:public"],
)

exports_files(
    [
        "ant_check.def",
        "ant_check.lef",
    ],
    visibility = ["//python/openroad:__pkg__"],
)

cc_test(
    name = "TestAntennaRules",
    size = "small",
    srcs = ["cpp/TestAntennaRules.cpp"],
    deps = [
        "//src/ant:ant_internal",
        "//src/odb/src/db",
        "//src/tst:db_fixture",
        "//src/tst:physical_builder",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestAntennaReport",
    size = "small",
    srcs = ["cpp/TestAntennaReport.cpp"],
    deps = [
        "//src/ant",
        "//src/ant:ant_internal",
        "//src/odb/src/db",
        "//src/tst:db_fixture",
        "//src/tst:physical_builder",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestAntennaSynthetic",
    size = "small",
    srcs = [
        "cpp/AntennaTestUtil.h",
        "cpp/TestAntennaSynthetic.cpp",
    ],
    deps = [
        "//src/ant",
        "//src/ant:ant_internal",
        "//src/odb/src/db",
        "//src/tst:db_fixture",
        "//src/tst:physical_builder",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@openmp",
    ],
)

cc_test(
    name = "TestAntennaDesigns",
    size = "small",
    srcs = [
        "cpp/AntennaTestUtil.h",
        "cpp/TestAntennaDesigns.cpp",
    ],
    data = [
        "ant_check.def",
        "ant_check.lef",
        "merged_spacing.lef",
        "sw130_random.def",
    ],
    deps = [
        "//src/ant",
        "//src/ant:ant_internal",
        "//src/odb/src/db",
        "//src/tst:db_fixture",
        "//src/tst:physical_builder",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
