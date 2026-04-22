# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, Precision Innovations Inc.

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

exports_files([
    "common/odb.i",
    "tcl/odb.tcl",
])

filegroup(
    name = "swig_imports_py",
    srcs = glob([
        "python/*.i",
        "common/*.i",
    ]),
)

SWIG_WARNINGS = "-w509,503,501,472,467,402,401,317,325,378,383,389,365,362,314,258,240,203,201"

cc_library(
    name = "swig_common",
    srcs = ["common/swig_common.cpp"],
    hdrs = ["common/swig_common.h"],
    deps = [
        "//src/odb/src/db",
        "//src/odb/src/defin",
        "//src/odb/src/defout",
        "//src/odb/src/lefin",
        "//src/odb/src/lefout",
        "//src/utl",
        "@boost.polygon",
    ],
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "//:src/Design.i",
        "//:src/Exception.i",
        "//src/odb:odb_public_headers",
    ] + glob([
        "tcl/*.i",
        "common/*.i",
    ]),
    module = "odbtcl",
    namespace_prefix = "odb",
    root_swig_src = "common/odb.i",
    swig_includes = [
        "../../include",
        "common",
        "tcl",
    ],
    swig_options = [SWIG_WARNINGS],
)

tcl_encode(
    name = "tcl",
    srcs = ["tcl/odb.tcl"],
    char_array_name = "odbtcl_tcl_inits",
    namespace = "odb",
)

filegroup(
    name = "message_srcs",
    srcs = glob([
        "**/*.cpp",
        "**/*.h",
        "**/*.tcl",
    ]),
    visibility = ["//src/odb:__pkg__"],
)

cc_library(
    name = "ui",
    srcs = [
        "common/swig_common.h",
        "tcl/MakeOdb.cpp",
        ":swig",
        ":tcl",
        "//src/odb:include/odb/geom.h",
    ],
    hdrs = ["//src/odb:include/odb/MakeOdb.h"],
    copts = [
        "-Wno-missing-braces",  # from TCL swigging
    ],
    includes = [
        "../../include",
        "common",
    ],
    deps = [
        "//:ord",
        "//src/odb",
        "//src/utl",
        "@abseil-cpp//absl/synchronization",
        "@boost.container",
        "@boost.geometry",
        "@boost.multi_array",
        "@boost.polygon",
        "@boost.stacktrace",
        "@tcl_lang//:tcl",
    ],
)
