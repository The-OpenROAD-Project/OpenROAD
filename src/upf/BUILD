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
    name = "upf",
    srcs = [
        "src/upf.cpp",
        "src/writer.cpp",
        "src/writer.h",
    ],
    hdrs = [
        "include/upf/upf.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/utl",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "include/upf/upf.h",
        "src/MakeUpf.cpp",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/upf/MakeUpf.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//:ord",
        "//src/odb",
        "//src/utl",
        "@tcl_lang//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/upf.tcl",
    ],
    char_array_name = "upf_tcl_inits",
    namespace = "upf",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/upf.i",
        "//:error_swig",
    ],
    module = "upf",
    namespace_prefix = "upf",
    root_swig_src = "src/upf.i",
    swig_includes = [
        "src/upf/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)
