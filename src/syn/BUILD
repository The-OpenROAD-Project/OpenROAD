# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:python_wrap_cc.bzl", "python_wrap_cc")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "syn",
    srcs = [
        "src/MakeSynthesis.cpp",
        "src/synthesis.cpp",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/syn/MakeSynthesis.h",
        "include/syn/synthesis.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//src/dbSta",
        "//src/odb/src/db",
        "//src/utl",
        "@tcl_lang//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/synthesis.tcl",
    ],
    char_array_name = "syn_tcl_inits",
    namespace = "syn",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/synthesis.i",
        "//:error_swig",
    ],
    module = "syn",
    namespace_prefix = "syn",
    root_swig_src = "src/synthesis.i",
    swig_includes = [
        "src/syn/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)
