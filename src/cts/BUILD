# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:python_wrap_cc.bzl", "python_wrap_cc")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "private_hdrs",
    hdrs = [
        "src/Clock.h",
        "src/Clustering.h",
        "src/CtsObserver.h",
        "src/HTreeBuilder.h",
        "src/LatencyBalancer.h",
        "src/SinkClustering.h",
        "src/TreeBuilder.h",
        "src/Util.h",
    ],
    visibility = ["//visibility:private"],
    deps = [
        "//src/sta:opensta_lib",
        "@boost.container_hash",
        "@boost.unordered",
    ],
)

cc_library(
    name = "cts",
    srcs = [
        "src/Clock.cpp",
        "src/Clock.h",
        "src/Clustering.cpp",
        "src/CtsOptions.cpp",
        "src/HTreeBuilder.cpp",
        "src/LatencyBalancer.cpp",
        "src/SinkClustering.cpp",
        "src/TechChar.cpp",
        "src/TreeBuilder.cpp",
        "src/TritonCTS.cpp",
    ],
    hdrs = [
        "include/cts/TritonCTS.h",
        "src/CtsOptions.h",
        "src/TechChar.h",
    ],
    includes = [
        "include",
        "src",
    ],
    deps = [
        ":private_hdrs",
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/est",
        "//src/odb",
        "//src/rsz",
        "//src/sta:opensta_lib",
        "//src/stt",
        "//src/utl",
        "@boost.container_hash",
        "@boost.polygon",
        "@boost.unordered",
        "@coin-or-lemon//:lemon",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "src/CtsGraphics.cpp",
        "src/CtsGraphics.h",
        "src/MakeTritoncts.cpp",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/cts/MakeTritoncts.h",
    ],
    copts = [
        "-Isrc/cts/src",
    ],
    includes = ["include"],
    deps = [
        ":cts",
        ":private_hdrs",
        "//:ord",
        "//src/dbSta",
        "//src/est",
        "//src/gui",
        "//src/odb",
        "//src/rsz",
        "//src/utl",
        "@boost.stacktrace",
        "@tk_tcl//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/TritonCTS.tcl",
    ],
    char_array_name = "cts_tcl_inits",
    namespace = "cts",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/TritonCTS.i",
        "//:error_swig",
    ],
    module = "cts",
    namespace_prefix = "cts",
    root_swig_src = "src/TritonCTS.i",
    swig_includes = [
        "src/cts/src",
    ],
)

python_wrap_cc(
    name = "swig-py",
    srcs = [
        "include/cts/TritonCTS.h",
        "src/CtsOptions.h",
        "src/TritonCTS-py.i",
        "//:error_swig-py",
    ],
    module = "cts_py",
    root_swig_src = "src/TritonCTS-py.i",
    swig_includes = [
        "include",
    ],
)
