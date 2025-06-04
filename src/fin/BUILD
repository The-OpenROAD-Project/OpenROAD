# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "fin",
    srcs = [
        "src/DensityFill.cpp",
        "src/DensityFill.h",
        "src/Finale.cpp",
        "src/MakeFinale.cpp",
        "src/graphics.cpp",
        "src/graphics.h",
        "src/polygon.h",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/fin/Finale.h",
        "include/fin/MakeFinale.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//:ord",
        "//src/gui",
        "//src/odb",
        "//src/utl",
        "@boost.lexical_cast",
        "@boost.polygon",
        "@boost.property_tree",
        "@tk_tcl//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/finale.tcl",
    ],
    char_array_name = "fin_tcl_inits",
    namespace = "fin",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/finale.i",
        "//:error_swig",
    ],
    module = "fin",
    namespace_prefix = "fin",
    root_swig_src = "src/finale.i",
    swig_includes = [
        "src/fin/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)
