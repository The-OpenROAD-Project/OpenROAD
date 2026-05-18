# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_test.bzl", "cc_test")

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
        "@googletest//:gtest_main",
    ],
)
