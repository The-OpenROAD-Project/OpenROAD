# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "ifp",
    srcs = [
        "src/InitFloorplan.cc",
    ],
    hdrs = [
        "include/ifp/InitFloorplan.hh",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//:opensta_lib",
        "//src/dbSta",
        "//src/odb",
        "//src/upf",
        "//src/utl",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "include/ifp/InitFloorplan.hh",
        "src/MakeInitFloorplan.cc",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/ifp/MakeInitFloorplan.hh",
    ],
    copts = [
        "-Isrc/ifp/src",
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
        "//src/utl",
        "@tk_tcl//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/InitFloorplan.tcl",
    ],
    char_array_name = "ifp_tcl_inits",
    namespace = "ifp",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/InitFloorplan.i",
        "//:error_swig",
    ],
    module = "ifp",
    namespace_prefix = "ifp",
    root_swig_src = "src/InitFloorplan.i",
    swig_includes = [
        "src/ifp/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)
