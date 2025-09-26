# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
)

cc_library(
    name = "cut",
    srcs = [
        "src/abc_library_factory.cpp",
        "src/blif.cpp",
        "src/blifParser.cpp",
        "src/logic_cut.cpp",
        "src/logic_extractor.cpp",
    ],
    hdrs = [
        "include/cut/abc_init.h",
        "include/cut/abc_library_factory.h",
        "include/cut/blif.h",
        "include/cut/blifParser.h",
        "include/cut/logic_cut.h",
        "include/cut/logic_extractor.h",
    ],
    defines = [
        "_GLIBCXX_ASSERTIONS",
        "ABC_NAMESPACE=abc",
    ],
    includes = ["include"],
    deps = [
        "//src/dbSta",
        "//src/odb",
        "//src/rsz",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@boost.bind",
        "@boost.config",
        "@boost.fusion",
        "@boost.lambda",
        "@boost.optional",
        "@boost.phoenix",
        "@boost.spirit",
        "@edu_berkeley_abc//:abc-lib",
    ],
)
