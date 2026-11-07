# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")
load("//test:regression.bzl", "messages_txt")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "ret",
    srcs = [
        "src/RegisterUtils.cpp",
        "src/RegisterUtils.h",
        "src/Retime.cpp",
        "src/Retime.h",
        "src/Retimer.cpp",
        "src/RetimingGraph.cpp",
        "src/RetimingGraph.h",
        "src/SequentialUnmapper.cpp",
    ],
    hdrs = [
        "include/ret/Retimer.h",
        "include/ret/SequentialUnmapper.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/dpl",
        "//src/est",
        "//src/odb/src/db",
        "//src/sta:opensta_lib",
        "//src/utl",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "src/MakeRet.cpp",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/ret/MakeRet.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":ret",
        "//:ord",
        "//src/odb/src/db",
        "//src/utl",
        "@boost.stacktrace",
        "@tcl_lang//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/ret.tcl",
    ],
    char_array_name = "ret_tcl_inits",
    namespace = "ret",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/ret.i",
        "//:error_swig",
    ],
    module = "ret",
    namespace_prefix = "ret",
    root_swig_src = "src/ret.i",
    swig_includes = [
        "src/ret/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)

exports_files(["README.md"])

messages_txt(
    visibility = ["//docs:__pkg__"],
)
