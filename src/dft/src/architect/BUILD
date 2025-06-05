# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "architect",
    srcs = [
        "Opt.cpp",
        "ScanArchitect.cpp",
        "ScanArchitectHeuristic.cpp",
        "ScanChain.cpp",
    ],
    hdrs = [
        "Opt.hh",
        "ScanArchitect.hh",
        "ScanArchitectHeuristic.hh",
        "ScanChain.hh",
    ],
    includes = [
        ".",
    ],
    deps = [
        "//src/dft/src/cells",
        "//src/dft/src/clock_domain",
        "//src/dft/src/config",
        "//src/dft/src/utils",
        "//src/odb",
        "//src/utl",
        "@boost.geometry",
    ],
)
