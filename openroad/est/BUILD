# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")
load("//test:regression.bzl", "messages_txt")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

# Abstract service interface published through utl::ServiceRegistry.
# Consumers depend only on this target, not on //openroad/est, which keeps
# them free of the full est implementation surface.
cc_library(
    name = "parasitics_service",
    hdrs = ["include/est/ParasiticsService.h"],
    includes = ["include"],
    deps = ["//openroad/grt:groute"],
)

cc_library(
    name = "est_impl",
    srcs = [
        "src/EstimateParasitics.cpp",
        "src/MakeWireParasitics.cpp",
        "src/OdbCallBack.cpp",
        "src/SteinerTree.cpp",
    ],
    hdrs = [
        "include/est/EstimateParasitics.h",
        "include/est/SteinerTree.h",
        "src/AbstractSteinerRenderer.h",
        "src/MakeWireParasitics.h",
        "src/OdbCallBack.h",
    ],
    copts = [
        "-fopenmp",
    ],
    includes = [
        "include",
        "src",
    ],
    visibility = [
        "//openroad/est:__subpackages__",
    ],
    deps = [
        ":parasitics_service",
        "//openroad/dbSta",
        "//openroad/dbSta:SpefWriter",
        "//openroad/dbSta:dbNetwork",
        "//openroad/grt",
        "//openroad/grt:groute",
        "//openroad/grt:route_pt",
        "//openroad/odb/src/db",
        "//openroad/sta:opensta_lib",
        "//openroad/stt",
        "//openroad/utl",
        "@openmp",
    ],
)

cc_library(
    name = "est",
    hdrs = [
        "include/est/EstimateParasitics.h",
        "include/est/SteinerTree.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":est_impl",
        ":parasitics_service",
        "//openroad/dbSta",
        "//openroad/dbSta:SpefWriter",
        "//openroad/dbSta:dbNetwork",
        "//openroad/grt",
        "//openroad/grt:groute",
        "//openroad/odb/src/db",
        "//openroad/sta:opensta_lib",
        "//openroad/stt",
        "//openroad/utl",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "src/MakeEstimateParasitics.cpp",
        "src/SteinerRenderer.cpp",
        "src/SteinerRenderer.h",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/est/MakeEstimateParasitics.h",
    ],
    copts = [
        "-Wno-missing-braces",  # from TCL swigging
        "-fopenmp",
    ],
    includes = [
        "include",
        "src",
    ],
    deps = [
        ":est_impl",
        "//:ord",
        "//openroad/dbSta:dbNetwork",
        "//openroad/odb/src/db",
        "//openroad/sta:opensta_lib",
        "//openroad/utl",
        "//openroad/web:core",
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
        "//openroad/sta:sta_swig_files",
    ],
    module = "est",
    namespace_prefix = "est",
    root_swig_src = "src/EstimateParasitics.i",
    swig_includes = [
        "../sta",
        "src",
    ],
)

exports_files(["README.md"])

messages_txt(
    visibility = ["//docs:__pkg__"],
)
