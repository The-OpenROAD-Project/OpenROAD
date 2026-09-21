# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_python//python:defs.bzl", "py_library")
load("//bazel:python_wrap_cc.bzl", "python_wrap_cc")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")
load("//test:regression.bzl", "messages_txt")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "hmac_sha256",
    srcs = ["src/HmacSha256.cpp"],
    hdrs = ["src/HmacSha256.h"],
    includes = ["src"],
)

cc_library(
    name = "marks",
    srcs = ["src/Marks.cpp"],
    hdrs = ["src/Marks.h"],
    includes = ["src"],
    deps = [":hmac_sha256"],
)

cc_library(
    name = "claim_file",
    srcs = ["src/ClaimFile.cpp"],
    hdrs = ["src/ClaimFile.h"],
    includes = ["src"],
)

cc_library(
    name = "verify_result",
    srcs = ["src/VerifyResult.cpp"],
    hdrs = ["include/wmk/VerifyResult.h"],
    includes = ["include"],
    deps = ["@boost.math"],
)

cc_library(
    name = "claims",
    srcs = ["src/Claims.cpp"],
    hdrs = ["src/Claims.h"],
    includes = ["src"],
    deps = ["@boost.algorithm"],
)

cc_library(
    name = "timing",
    srcs = ["src/Timing.cpp"],
    hdrs = ["src/Timing.h"],
    includes = ["src"],
    deps = [
        "//src/dbSta",
        "//src/sta:opensta_lib",
    ],
)

cc_library(
    name = "clock_tree",
    srcs = ["src/ClockTree.cpp"],
    hdrs = ["src/ClockTree.h"],
    includes = ["src"],
    deps = [
        "//src/dbSta:dbNetwork",
        "//src/odb/src/db",
        "//src/sta:opensta_lib",
    ],
)

cc_library(
    name = "wmk_private_hdrs",
    hdrs = [
        "src/Keys.h",
        "src/Options.h",
        "src/RoutingStat.h",
        "src/Wirelength.h",
    ],
    includes = [
        "src",
    ],
    visibility = ["//src/wmk:__subpackages__"],
    deps = ["//src/odb/src/db"],
)

cc_library(
    name = "wmk",
    srcs = [
        "src/CtsWatermark.cpp",
        "src/Keys.cpp",
        "src/Options.cpp",
        "src/PlacementWatermark.cpp",
        "src/RoutingStat.cpp",
        "src/Verify.cpp",
        "src/Watermark.cpp",
        "src/Wirelength.cpp",
    ],
    hdrs = [
        "include/wmk/Watermark.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":wmk_private_hdrs",
        ":hmac_sha256",
        ":marks",
        ":claims",
        ":verify_result",
        ":claim_file",
        ":timing",
        ":clock_tree",
        # Slack screens the placement candidates and skew screens the clock
        # tree ones, and moving a cell needs its parasitics re-estimated before
        # either means anything.
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
        "src/MakeWatermark.cpp",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/wmk/MakeWatermark.h",
    ],
    includes = [
        "include",
        "src",
    ],
    deps = [
        ":hmac_sha256",
        ":marks",
        ":wmk",
        ":wmk_private_hdrs",
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
        "src/watermark.tcl",
    ],
    char_array_name = "wmk_tcl_inits",
    namespace = "wmk",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/watermark.i",
        "//:error_swig",
    ],
    module = "wmk",
    namespace_prefix = "wmk",
    root_swig_src = "src/watermark.i",
    swig_includes = [
        "src/wmk/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)

python_wrap_cc(
    name = "swig-py",
    srcs = [
        "include/wmk/VerifyResult.h",
        "include/wmk/Watermark.h",
        "src/watermark-py.i",
        "//:error_swig-py",
    ],
    module = "wmk",
    root_swig_src = "src/watermark-py.i",
    swig_includes = [
        "include",
        "src",
    ],
)

filegroup(
    name = "doc_files",
    srcs = [
        "README.md",
        "src/watermark.tcl",
    ],
    visibility = ["//src/wmk/test:__pkg__"],
)

messages_txt(
    visibility = [
        "//docs:__pkg__",
        "//src/wmk/test:__pkg__",
    ],
)

# Keep timing-dependent bindings in _openroadpy.so. A separate WMK extension
# would link another copy of OpenSTA's singleton state (such as port directions)
# and interpret the Design's Liberty objects using that uninitialized copy.
py_library(
    name = "wmk_py",
    srcs = ["wmk.py"],
    imports = ["."],
    visibility = ["//visibility:public"],
    deps = ["//:openroad_py"],
)
