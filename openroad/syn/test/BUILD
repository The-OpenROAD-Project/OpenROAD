# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

regression_test(
    name = "acd_resynth",
    data = [
        "Nangate45/Nangate45.lef",
        "Nangate45/Nangate45_typ.lib",
        "acd_resynth.ok",
        "gcd.v",
    ],
)

regression_test(
    name = "comb_loop",
    data = [
        "Nangate45/Nangate45.lef",
        "Nangate45/Nangate45_typ.lib",
        "Nangate45/fakeram45_64x7.lef",
        "Nangate45/fakeram45_64x7.lib",
        "comb_loop.ok",
        "comb_loop.v",
        "comb_loop_cells.lef",
        "comb_loop_cells.lib",
    ],
)

regression_test(
    name = "def_units",
    data = [
        "Nangate45/Nangate45.lef",
        "Nangate45/Nangate45_typ.lib",
        "comb_loop.v",
        "def_units.ok",
    ],
)

# No gtest_main on ir/graph/opt tests: ABC (via //openroad/syn/src/ir →
# TritModel) ships its own main() that wins over gtest_main; each TU
# defines its own.
cc_test(
    name = "ir_test",
    srcs = ["ir_test.cc"],
    deps = [
        "//openroad/syn/src/ir",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "graph_test",
    srcs = ["graph_test.cc"],
    deps = [
        "//openroad/syn/src/ir",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "opt_test",
    srcs = [
        "equivalence_check.h",
        "opt_test.cc",
    ],
    deps = [
        "//openroad/syn",
        "//openroad/syn/src/ir",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "liveness_test",
    srcs = ["liveness_test.cc"],
    # No gtest_main: ABC ships its own main() that wins over gtest_main;
    # the TU defines its own.
    deps = [
        "//openroad/syn",  # buildcleaner: keep: needs fixing syn layering first.
        "//openroad/syn/src/ir",
        "//openroad/utl",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "npn_test",
    srcs = ["npn_test.cc"],
    # No gtest_main: ABC ships its own main() that wins over gtest_main;
    # the TU defines its own.
    deps = [
        "//openroad/syn",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "parse_test",
    srcs = ["parse_test.cc"],
    # No gtest_main: ABC (via //openroad/syn/src/ir → TritModel) ships its own
    # main() that wins over gtest_main; parse_test.cc defines its own.
    deps = [
        "//openroad/syn/src/ir",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "sm_test",
    srcs = ["sm_test.cc"],
    data = ["sm_test_cells.lib"],
    deps = [
        "//openroad/syn",
        "//openroad/syn/src/ir",
        "//openroad/tst",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "cm_test",
    srcs = [
        "cm_test.cc",
        "equivalence_check.h",
    ],
    data = ["cm_test_cells.lib"],
    deps = [
        "//openroad/syn",
        "//openroad/syn/src/ir",
        "//openroad/tst",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "abc_test",
    srcs = ["abc_test.cc"],
    deps = [
        "//openroad/syn",
        "//openroad/syn/src/ir",
        "//openroad/tst",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "acd_test",
    srcs = ["acd_test.cc"],
    data = ["cm_test_cells.lib"],
    deps = [
        "//openroad/dbSta",
        "//openroad/sta:opensta_lib",
        "//openroad/syn",
        "//openroad/tst",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "bitblast_test",
    srcs = ["bitblast_test.cc"],
    deps = [
        "//openroad/syn",
        "//openroad/syn/src/ir",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "elab_diag_test",
    srcs = ["elab_diag_test.cc"],
    data = [
        "elab_diag_expr.sv",
        "elab_diag_hier.sv",
        "elab_diag_tops.sv",
    ],
    deps = [
        "//openroad/syn/src/elab",
        "//openroad/tst:db_fixture",
        "//openroad/utl",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "elab_test",
    srcs = [
        "elab_test.cc",
        "elab_testcases.h",
        "equivalence_check.h",
    ],
    data = ["macro_test_cells.lib"],
    deps = [
        "//openroad/syn",
        "//openroad/syn/src/elab",
        "//openroad/syn/src/ir",
        "//openroad/tst",
        "@googletest//:gtest",
    ],
)
