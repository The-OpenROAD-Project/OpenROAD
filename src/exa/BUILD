# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "exa",
    srcs = [
        "src/MakeExample.cpp",
        "src/example.cpp",
        "src/observer.h",
    ],
    hdrs = [
        "include/exa/MakeExample.h",
        "include/exa/example.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//src/odb",
        "//src/utl",
        "@tk_tcl//:tcl",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "src/MakeExample.cpp",
        "src/graphics.cpp",
        "src/graphics.h",
        "src/observer.h",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/exa/MakeExample.h",
        "include/exa/example.h",
    ],
    copts = [
        "-Isrc/exa/src",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//:ord",
        "//src/gui",
        "//src/odb",
        "//src/utl",
        "@tk_tcl//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/example.tcl",
    ],
    char_array_name = "exa_tcl_inits",
    namespace = "exa",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/example.i",
        "//:error_swig",
    ],
    module = "exa",
    namespace_prefix = "exa",
    root_swig_src = "src/example.i",
    swig_includes = [
        "src/exa/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)
