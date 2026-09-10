# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_python//python:defs.bzl", "py_library")
load("//bazel:python_wrap_cc.bzl", "PYTHON_EXTENSION_LINKOPTS", "PYTHON_STABLE_API_DEFINE", "python_wrap_cc")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

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
        "src/Wirelength.h",
    ],
    includes = [
        "src",
    ],
    visibility = ["//visibility:private"],
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
        ":claims",
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

# The Python extension and the wrapper that loads it.  Without these the module
# is reachable from Tcl only: openroad -python would import openroad fine and
# then fail on "import wmk", because the generated shim and the .so it needs
# would never be packaged together.
cc_binary(
    name = "_wmk.so",
    srcs = [
        ":swig-py",
    ],
    defines = [PYTHON_STABLE_API_DEFINE],
    includes = ["src"],
    linkopts = PYTHON_EXTENSION_LINKOPTS,
    linkshared = True,
    deps = [
        ":wmk",
        # The interface file pulls in the key parser for its typemap, and
        # OpenRoad.hh for the accessor the wrapper is generated against.
        ":wmk_private_hdrs",
        ":hmac_sha256",
        "//:ord",
        "//src/odb/src/db",
        "//src/utl",
        "@boost.stacktrace",
        "@rules_python//python/cc:current_py_cc_headers",
    ],
)

py_library(
    name = "wmk_py",
    srcs = [":swig-py"],
    data = [":_wmk.so"],
    imports = ["."],
    visibility = ["//visibility:public"],
)
