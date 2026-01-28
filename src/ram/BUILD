# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:python_wrap_cc.bzl", "python_wrap_cc")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "ram",
    srcs = [
        "src/MakeRam.cpp",
        "src/layout.cpp",
        "src/layout.h",
        "src/ram.cpp",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/ram/MakeRam.h",
        "include/ram/ram.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//:ord",
        "//src/dbSta:dbNetwork",
        "//src/dpl",
        "//src/drt",
        "//src/grt",
        "//src/odb",
        "//src/pdn",
        "//src/ppl",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@boost.stacktrace",
        "@tk_tcl//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/ram.tcl",
    ],
    char_array_name = "ram_tcl_inits",
    namespace = "ram",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/ram.i",
        "//:error_swig",
    ],
    module = "ram",
    namespace_prefix = "ram",
    root_swig_src = "src/ram.i",
    swig_includes = [
        "src/ram/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)

python_wrap_cc(
    name = "swig-py",
    srcs = [
        "include/ram/ram.h",
        "src/ram-py.i",
        "//:error_swig-py",
    ],
    module = "ram_py",
    root_swig_src = "src/ram-py.i",
    swig_includes = [
        "include",
    ],
    deps = [
        "//src/odb:swig-py",
    ],
)
