# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_shell//shell:sh_test.bzl", "sh_test")

# End-to-end repro for a SIGSEGV in syn::map_combinationals (the post-ABC
# mapper introduced in #10473). Runs the openroad binary against a 2-input
# AND Tcl repro and expects exit 0 with "OK" on stdout. Currently fails;
# tagged manual so CI does not turn red on the unfixed bug.
sh_test(
    name = "synthesize_minimal_and_sigsegv_test",
    srcs = ["run_openroad_repro.sh"],
    args = [
        "$(rootpath //:openroad)",
        "$(rootpath synthesize_minimal_and.tcl)",
    ],
    data = [
        "synthesize_minimal_and.sv",
        "synthesize_minimal_and.tcl",
        "//:openroad",
        "//test:regression_resources",
    ],
    tags = ["manual"],
)

# No gtest_main on ir/graph/opt tests: ABC (via //src/syn/src/ir →
# TritModel) ships its own main() that wins over gtest_main; each TU
# defines its own.
cc_test(
    name = "ir_test",
    srcs = ["ir_test.cc"],
    features = ["-layering_check"],
    deps = [
        "//src/syn/src/ir",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "graph_test",
    srcs = ["graph_test.cc"],
    features = ["-layering_check"],
    deps = [
        "//src/syn/src/ir",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "opt_test",
    srcs = ["opt_test.cc"],
    features = ["-layering_check"],
    deps = [
        "//src/syn",
        "//src/syn/src/ir",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "liveness_test",
    srcs = ["liveness_test.cc"],
    features = ["-layering_check"],
    # No gtest_main: ABC ships its own main() that wins over gtest_main;
    # the TU defines its own.
    deps = [
        "//src/syn",
        "//src/syn/src/ir",
        "//src/utl",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "parse_test",
    srcs = ["parse_test.cc"],
    features = ["-layering_check"],
    # No gtest_main: ABC (via //src/syn/src/ir → TritModel) ships its own
    # main() that wins over gtest_main; parse_test.cc defines its own.
    deps = [
        "//src/syn/src/ir",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "sm_test",
    srcs = ["sm_test.cc"],
    data = ["sm_test_cells.lib"],
    features = ["-layering_check"],
    deps = [
        "//src/syn",
        "//src/syn/src/ir",
        "//src/tst",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "abc_test",
    srcs = ["abc_test.cc"],
    features = ["-layering_check"],
    deps = [
        "//src/syn",
        "//src/syn/src/ir",
        "//src/tst",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "bitblast_test",
    srcs = ["bitblast_test.cc"],
    features = ["-layering_check"],
    deps = [
        "//src/syn",
        "//src/syn/src/ir",
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
    features = ["-layering_check"],
    deps = [
        "//src/syn/src/elab",
        "//src/syn/src/ir",
        "@googletest//:gtest",
    ],
)
