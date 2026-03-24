load("@rules_python//python:defs.bzl", "py_test")
load("//test:regression.bzl", "doc_check_test", "regression_test")

package(features = ["layering_check"])

# From CMakeLists.txt or_integration_tests(TESTS
COMPULSORY_TESTS = [
    "ant_check",
    "ant_report",
    "check_api1",
    "check_drt1",
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
            "ant_report": [
                "ant_check.lef",
                "ant_check.def",
            ],
            "check_api1": [
                "merged_spacing.lef",
                "sw130_random.def",
            ],
            "check_drt1": [
                "merged_spacing.lef",
                "sw130_random.def",
            ],
            "check_grt1": [
                "gcd_sky130.def",
                "sky130hs/sky130hs_tt.lib",
                "sky130hs/sky130hs_std_cell.lef",
                "sky130hs/sky130hs.tlef",
            ],
            "no-check_grt1": [
                "gcd_sky130.def",
                "sky130hs/sky130hs_tt.lib",
                "sky130hs/sky130hs_std_cell.lef",
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

py_test(
    name = "ant_man_tcl_check",
    srcs = ["ant_man_tcl_check.py"],
    data = ["//src/ant:doc_files"],
    tags = ["doc_check"],
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
