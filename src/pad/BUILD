# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "pad",
    srcs = [
        "src/ICeWall.cpp",
        "src/MakeICeWall.cpp",
        "src/PadPlacer.cpp",
        "src/PadPlacer.h",
        "src/RDLGui.cpp",
        "src/RDLGui.h",
        "src/RDLRoute.cpp",
        "src/RDLRoute.h",
        "src/RDLRouter.cpp",
        "src/RDLRouter.h",
        "src/Utilities.cpp",
        "src/Utilities.h",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/pad/ICeWall.h",
        "include/pad/MakeICeWall.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//:ord",
        "//src/gui",
        "//src/odb",
        "//src/utl",
        "@boost.geometry",
        "@boost.graph",
        "@boost.icl",
        "@boost.polygon",
        "@boost.stacktrace",
        "@tk_tcl//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/pad.tcl",
    ],
    char_array_name = "pad_tcl_inits",
    namespace = "pad",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/pad.i",
        "//:error_swig",
    ],
    module = "pad",
    namespace_prefix = "pad",
    root_swig_src = "src/pad.i",
    swig_includes = [
        "src/pad/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)
