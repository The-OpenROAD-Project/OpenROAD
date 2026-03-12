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
    name = "private_hdrs",
    hdrs = [
        "src/AbstractSteinerRenderer.h",
        "src/EstimateParasiticsCallBack.h",
        "src/MakeWireParasitics.h",
        "src/OdbCallBack.h",
        "src/SteinerRenderer.h",
    ],
    visibility = ["//visibility:private"],
    deps = [
        "//src/grt:groute",
        "//src/sta:opensta_lib",
    ],
)

cc_library(
    name = "est",
    srcs = [
        "src/EstimateParasitics.cpp",
        "src/EstimateParasiticsCallBack.cpp",
        "src/MakeWireParasitics.cpp",
        "src/OdbCallBack.cpp",
        "src/SteinerTree.cpp",
    ],
    hdrs = [
        "include/est/EstimateParasitics.h",
        "include/est/SteinerTree.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":private_hdrs",
        "//src/dbSta",
        "//src/dbSta:SpefWriter",
        "//src/dbSta:dbNetwork",
        "//src/grt",
        "//src/grt:groute",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/stt",
        "//src/utl",
        "@boost.container_hash",
        "@boost.multi_array",
        "@spdlog",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "src/MakeEstimateParasitics.cpp",
        "src/SteinerRenderer.cpp",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/est/MakeEstimateParasitics.h",
    ],
    copts = [
        "-Isrc/est/src",
        "-Wno-missing-braces",  # from TCL swigging
        "-fopenmp",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":est",
        ":private_hdrs",
        "//:ord",
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/grt",
        "//src/gui",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/stt",
        "//src/utl",
        "@boost.container_hash",
        "@boost.multi_array",
        "@boost.stacktrace",
        "@spdlog",
        "@tcl_lang//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/EstimateParasitics.tcl",
    ],
    char_array_name = "est_tcl_inits",
    namespace = "est",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/EstimateParasitics.i",
        "//:error_swig",
        "//src/sta:sta_swig_files",
    ],
    module = "est",
    namespace_prefix = "est",
    root_swig_src = "src/EstimateParasitics.i",
    swig_includes = [
        "../sta",
        "src",
    ],
)
