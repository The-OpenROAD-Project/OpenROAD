# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors
load("@rules_python//python:defs.bzl", "py_library", "py_test")
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

TESTS = [
    "aes_nangate45",
    "countdown_asap7",
    "ibex_sky130hd",
]

filegroup(
    name = "regression_resources",
    # Dependencies could be specified more narrowly per test case,
    # but at least it is not a glob of everything and there are
    # per test glob patterns below.
    srcs = [
        "helpers.py",
        "helpers.tcl",
    ],
)

[
    filegroup(
        name = test_name + "_resources",
        srcs = [":regression_resources"] + glob(
            [
                test_name + "*",
            ] + {
                "aes_nangate45": [
                    "Nangate45/Nangate45.lef",
                    "Nangate45/Nangate45_stdcell.lef",
                    "Nangate45/Nangate45_typ.lib",
                ],
                "countdown_asap7": [
                    "asap7/asap7_tech_1x_201209.lef",
                    "asap7/asap7sc7p5t_28_R_1x_220121a.lef",
                    "asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz",
                    "asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz",
                    "asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz",
                    "asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib",
                    "asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz",
                ],
                "ibex_sky130hd": [
                    "sky130hd/sky130_fd_sc_hd__ss_n40C_1v40.lib",
                    "sky130hd/sky130hd.tlef",
                    "sky130hd/sky130hd_std_cell.lef",
                ],
            }.get(test_name, []),
        ),
    )
    for test_name in TESTS
]

[
    regression_test(
        name = test_name,
        data = [":" + test_name + "_resources"],
        tags = [],
        visibility = ["//visibility:public"],
    )
    for test_name in TESTS
]

py_library(
    name = "helpers",
    srcs = ["helpers.py"],
    imports = ["."],
    deps = [
        "//python/openroad:openroadpy",
    ],
)

PY_TESTS = [
    "aes_nangate45",
    "countdown_asap7",
    "ibex_sky130hd",
]

[
    py_test(
        name = test_name,
        srcs = [test_name + ".py"],
        data = [":" + test_name + "_resources"],
        deps = [
            ":helpers",
            "//python/openroad:openroadpy",
        ],
    )
    for test_name in PY_TESTS
]
