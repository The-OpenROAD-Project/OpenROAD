# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:python_wrap_cc.bzl", "python_wrap_cc")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")
load("//test:regression.bzl", "messages_txt")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "syn",
    srcs = [
        "src/flow/abc.cc",
        "src/flow/acd.cc",
        "src/flow/acd_resynth.cc",
        "src/flow/acd_resynth.h",
        "src/flow/acd_timing.cc",
        "src/flow/bitblast.cc",
        "src/flow/combinational_mapper.cc",
        "src/flow/combinational_mapper_npn.cc",
        "src/flow/constant_fold.cc",
        "src/flow/export.cc",
        "src/flow/export.h",
        "src/flow/import.cc",
        "src/flow/liveness.cc",
        "src/flow/loop_check.cc",
        "src/flow/opt_gatefusion.cc",
        "src/flow/opt_gatefusion.h",
        "src/flow/sequential_mapper.cc",
        "src/flow/target_index.cc",
        "src/synthesis.cpp",
    ],
    hdrs = [
        "include/syn/synthesis.h",
        "src/flow/acd.h",
        "src/flow/combinational_mapper_npn.h",
        "src/flow/constant_fold.h",
        "src/flow/import.h",
        "src/flow/target_index.h",
    ],
    includes = [
        "include",
        "src",
    ],
    deps = [
        "//openroad/dbSta",
        "//openroad/dbSta:dbNetwork",
        "//openroad/odb/src/db",
        "//openroad/rsz",
        "//openroad/sta:opensta_lib",
        "//openroad/syn/src/elab",
        "//openroad/syn/src/ir",
        "//openroad/utl",
        "@abc",
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
        "//openroad/utl",
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
        "src",
    ],
    deps = [
        "//openroad/odb:swig",
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
        "//openroad/odb:swig-py",
    ],
)

exports_files(["README.md"])

# Synthesis keeps most of its logger calls below src/: flow/ is covered by the
# macro's recursive glob, elab/ and ir/ are their own packages.
messages_txt(
    extra_srcs = [
        "//openroad/syn/src/elab:message_srcs",
        "//openroad/syn/src/ir:message_srcs",
        "//openroad/syn/src/lir:message_srcs",
    ],
    visibility = ["//docs:__pkg__"],
)
