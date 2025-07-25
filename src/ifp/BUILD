# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")
load("//bazel:python_wrap_cc.bzl", "python_wrap_cc")

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
        "//src/dbSta",
        "//src/odb",
        "//src/sta:opensta_lib",
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
        ":swig-py",
        ":tcl",
        ":python",
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
        "//:ord",
        "//src/dbSta",
        "//src/gui",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@boost.stacktrace",
        "@tk_tcl//:tcl",
        "@openroad_rules_python//python/cc:current_py_cc_headers",
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

python_wrap_cc(
    name = "swig-py",
    srcs = [
        "include/ifp/InitFloorplan.hh",
        "src/InitFloorplan-py.i",
        "//:error_swig-py",
    ],
    module = "ifp_py",
    root_swig_src = "src/InitFloorplan-py.i",
    swig_includes = [
        "include",
        "src",
    ],
    deps = [
        "//src/odb:swig-py",
    ],
)

tcl_encode(
    name = "python",
    srcs = [
        ":swig-py",
    ],
    char_array_name = "ifp_py_python_inits",
    namespace = "ifp",
)
