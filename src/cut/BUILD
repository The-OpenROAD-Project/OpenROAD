# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
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
        "//src/dbSta:dbNetwork",
        "//src/odb",
        "//src/rsz",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@abc",
        "@boost.bind",
        "@boost.config",
        "@boost.fusion",
        "@boost.lambda",
        "@boost.optional",
        "@boost.phoenix",
        "@boost.spirit",
    ],
)
