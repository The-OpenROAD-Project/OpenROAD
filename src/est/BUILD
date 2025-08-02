# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "est",
    srcs = [
        "src/EstimateParasitics.cpp",
        "src/MakeWireParasitics.cpp",
        "src/MakeWireParasitics.h",
        "src/OdbCallBack.cpp",
        "src/SteinerRenderer.cpp",
        "src/SteinerRenderer.h",
        "src/SteinerTree.cpp",
    ],
    hdrs = [
        "include/est/AbstractSteinerRenderer.h",
        "include/est/EstimateParasitics.h",
        "include/est/OdbCallBack.h",
        "include/est/SteinerTree.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//src/dbSta",
        "//src/grt:types",
        "//src/gui",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/stt",
        "//src/utl",
        "@boost.container_hash",
        "@boost.multi_array",
        "@spdlog",
        "@tk_tcl//:tcl",
    ],
)
cc_library(
    name = "ui",
    srcs = [
        "src/EstimateParasitics.cpp",
        "src/MakeEstimateParasitics.cpp",
        "src/MakeWireParasitics.cpp",
        "src/MakeWireParasitics.h",
        "src/SteinerRenderer.cpp",
        "src/SteinerRenderer.h",
        "src/SteinerTree.cpp",
    ],
    hdrs = [
        "include/est/AbstractSteinerRenderer.h",
        "include/est/EstimateParasitics.h",
        "include/est/MakeEstimateParasitics.h",
        "include/est/OdbCallBack.h",
        "include/est/SteinerTree.h",
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
        "//src/dbSta",
        "//src/grt:types",
        "//src/gui",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/stt",
        "//src/utl",
        "@boost.container_hash",
        "@boost.multi_array",
        "@spdlog",
        "@tk_tcl//:tcl",
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
