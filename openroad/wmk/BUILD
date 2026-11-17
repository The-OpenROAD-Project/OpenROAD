# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:python_wrap_cc.bzl", "python_wrap_cc", "python_wrap_library")
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
        "//openroad/dbSta",
        "//openroad/sta:opensta_lib",
    ],
)

cc_library(
    name = "clock_tree",
    srcs = ["src/ClockTree.cpp"],
    hdrs = ["src/ClockTree.h"],
    includes = ["src"],
    deps = [
        "//openroad/dbSta:dbNetwork",
        "//openroad/odb/src/db",
        "//openroad/sta:opensta_lib",
    ],
)

cc_library(
    name = "wmk_impl",
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
        "src/Keys.h",
        "src/Options.h",
        "src/RoutingStat.h",
        "src/Wirelength.h",
    ],
    includes = [
        "include",
        "src",
    ],
    visibility = [
        "//openroad/wmk:__subpackages__",
    ],
    deps = [
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
        "//openroad/dbSta",
        "//openroad/dbSta:dbNetwork",
        "//openroad/dpl",
        "//openroad/est",
        "//openroad/odb/src/db",
        "//openroad/sta:opensta_lib",
        "//openroad/utl",
    ],
)

cc_library(
    name = "wmk",
    hdrs = [
        "include/wmk/Watermark.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":verify_result",
        ":wmk_impl",
        "//openroad/odb/src/db",
        "//openroad/utl",
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
        ":wmk_impl",
        "//:ord",
        "//openroad/odb/src/db",
        "//openroad/utl",
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
        "src",
    ],
    deps = [
        "//openroad/odb:swig",
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
    module = "wmk_py",
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
    visibility = ["//openroad/wmk/test:__pkg__"],
)

messages_txt(
    visibility = [
        "//docs:__pkg__",
        "//openroad/wmk/test:__pkg__",
    ],
)

# Compiled into //:_openroad.so with the other Python modules rather than a
# separate extension: that would link another copy of OpenSTA's singleton
# state (such as port directions) and interpret the Design's Liberty objects
# using that uninitialized copy.
python_wrap_library(
    name = "py_wrap",
    includes = [
        "include",
        "src",
    ],
    deps = [
        ":hmac_sha256",
        ":wmk",
    ],
)
