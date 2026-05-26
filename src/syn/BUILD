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
        "src/flow/abc.cc",
        "src/flow/bitblast.cc",
        "src/flow/combinational_mapper.cc",
        "src/flow/combinational_mapper_npn.cc",
        "src/flow/combinational_mapper_npn.h",
        "src/flow/constant_fold.cc",
        "src/flow/constant_fold.h",
        "src/flow/export.cc",
        "src/flow/export.h",
        "src/flow/import.cc",
        "src/flow/import.h",
        "src/flow/liveness.cc",
        "src/flow/opt_gatefusion.cc",
        "src/flow/opt_gatefusion.h",
        "src/flow/sequential_mapper.cc",
        "src/synthesis.cpp",
    ],
    hdrs = [
        "include/syn/synthesis.h",
    ],
    includes = [
        "include",
        "src",
    ],
    deps = [
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/odb/src/db",
        "//src/rsz",
        "//src/sta:opensta_lib",
        "//src/syn/src/elab",
        "//src/syn/src/ir",
        "//src/utl",
        "@abc",
        "@boost.stacktrace",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "src/MakeSynthesis.cpp",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/syn/MakeSynthesis.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":syn",
        "//:ord",
        "//src/dbSta",
        "//src/odb/src/db",
        "//src/utl",
        "@boost.stacktrace",
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

python_wrap_cc(
    name = "swig-py",
    srcs = [
        "include/syn/synthesis.h",
        "src/synthesis-py.i",
        "//:error_swig-py",
    ],
    module = "syn_py",
    root_swig_src = "src/synthesis-py.i",
    swig_includes = [
        "include",
        "src",
    ],
    deps = [
        "//src/odb:swig-py",
    ],
)
