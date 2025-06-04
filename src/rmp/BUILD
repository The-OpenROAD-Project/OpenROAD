# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "rmp",
    srcs = [
        "src/MakeRestructure.cpp",
        "src/Restructure.cpp",
        "src/abc_library_factory.cpp",
        "src/abc_library_factory.h",
        "src/blif.cpp",
        "src/blifParser.cpp",
        "src/delay_optimization_strategy.cpp",
        "src/delay_optimization_strategy.h",
        "src/logic_cut.cpp",
        "src/logic_cut.h",
        "src/logic_extractor.cpp",
        "src/logic_extractor.h",
        "src/logic_optimization_strategy.h",
        "src/resynthesis_strategy.h",
        "src/zero_slack_strategy.cpp",
        "src/zero_slack_strategy.h",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/rmp/MakeRestructure.h",
        "include/rmp/Restructure.h",
        "include/rmp/blif.h",
        "include/rmp/blifParser.h",
        "include/rmp/unique_name.h",
    ],
    copts = [
        "-Isrc/rmp/src",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//:opensta_lib",
        "//:ord",
        "//src/dbSta",
        "//src/gui",
        "//src/odb",
        "//src/rsz",
        "//src/utl",
        "@edu_berkeley_abc//:abc-lib",
        "@tk_tcl//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/rmp.tcl",
    ],
    char_array_name = "rmp_tcl_inits",
    namespace = "rmp",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/rmp.i",
        "//:error_swig",
        "//:sta_swig_files",
    ],
    module = "rmp",
    namespace_prefix = "rmp",
    root_swig_src = "src/rmp.i",
    swig_includes = [
        "../../src/sta",
        "src/rmp/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)
