# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "cts",
    srcs = [
        "src/Clock.cpp",
        "src/Clock.h",
        "src/Clustering.cpp",
        "src/Clustering.h",
        "src/CtsGraphics.cpp",
        "src/CtsGraphics.h",
        "src/CtsObserver.h",
        "src/CtsOptions.cpp",
        "src/CtsOptions.h",
        "src/HTreeBuilder.cpp",
        "src/HTreeBuilder.h",
        "src/LevelBalancer.cpp",
        "src/LevelBalancer.h",
        "src/MakeTritoncts.cpp",
        "src/SinkClustering.cpp",
        "src/SinkClustering.h",
        "src/TechChar.cpp",
        "src/TechChar.h",
        "src/TreeBuilder.cpp",
        "src/TreeBuilder.h",
        "src/TritonCTS.cpp",
        "src/Util.h",
    ],
    hdrs = [
        "include/cts/MakeTritoncts.h",
        "include/cts/TritonCTS.h",
    ],
    includes = ["include"],
    deps = [
        "//:opensta_lib",
        "//:ord",
        "//src/dbSta",
        "//src/gui",
        "//src/odb",
        "//src/rsz",
        "//src/stt",
        "//src/utl",
        "@boost.container_hash",
        "@boost.polygon",
        "@boost.unordered",
        "@com_github_quantamhd_lemon//:lemon",
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
