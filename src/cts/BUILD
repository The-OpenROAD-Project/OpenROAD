# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

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
        "src/CtsOptions.h",
        "src/HTreeBuilder.h",
        "src/LevelBalancer.h",
        "src/SinkClustering.h",
        "src/TechChar.h",
        "src/TreeBuilder.h",
        "src/Util.h",
    ],
    visibility = ["//visibility:private"],
)

cc_library(
    name = "cts",
    srcs = [
        "src/Clock.cpp",
        "src/Clock.h",
        "src/Clustering.cpp",
        "src/CtsOptions.cpp",
        "src/HTreeBuilder.cpp",
        "src/LevelBalancer.cpp",
        "src/SinkClustering.cpp",
        "src/TechChar.cpp",
        "src/TreeBuilder.cpp",
        "src/TritonCTS.cpp",
    ],
    hdrs = [
        "include/cts/TritonCTS.h",
    ],
    includes = ["include"],
    deps = [
        ":private_hdrs",
        "//src/sta:opensta_lib",
        "//src/dbSta",
        "//src/odb",
        "//src/rsz",
        "//src/stt",
        "//src/utl",
        "@boost.container_hash",
        "@boost.polygon",
        "@boost.unordered",
        "@com_github_quantamhd_lemon//:lemon",
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
        "include/cts/TritonCTS.h",
    ],
    copts = [
        "-Isrc/cts/src",
    ],
    includes = ["include"],
    deps = [
        ":private_hdrs",
        "//:ord",
        "//src/dbSta",
        "//src/gui",
        "//src/odb",
        "//src/rsz",
        "//src/utl",
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
