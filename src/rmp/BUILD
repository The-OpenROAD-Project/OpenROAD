# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2026, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "rmp",
    srcs = [
        "src/Restructure.cpp",
        "src/annealing_strategy.cpp",
        "src/annealing_strategy.h",
        "src/delay_optimization_strategy.cpp",
        "src/delay_optimization_strategy.h",
        "src/genetic_strategy.cpp",
        "src/genetic_strategy.h",
        "src/gia.cpp",
        "src/gia.h",
        "src/logic_optimization_strategy.h",
        "src/resynthesis_strategy.h",
        "src/slack_tuning_strategy.cpp",
        "src/slack_tuning_strategy.h",
        "src/utils.cpp",
        "src/utils.h",
        "src/zero_slack_strategy.cpp",
        "src/zero_slack_strategy.h",
    ],
    hdrs = [
        "include/rmp/Restructure.h",
    ],
    copts = [
        "-Isrc/rmp/src",
    ],
    defines = ["ABC_NAMESPACE=abc"],
    includes = [
        "include",
    ],
    deps = [
        "//:ord",
        "//src/cut",
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/odb",
        "//src/rsz",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@abc",
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/hash",
        "@abseil-cpp//absl/random:distributions",
        "@abseil-cpp//absl/synchronization",
        "@boost.bind",
        "@boost.config",
        "@boost.fusion",
        "@boost.lambda",
        "@boost.optional",
        "@boost.phoenix",
        "@boost.spirit",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "src/MakeRestructure.cpp",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/rmp/MakeRestructure.h",
    ],
    copts = [
        "-Isrc/rmp/src",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":rmp",
        "//:ord",
        "//src/cut",
        "//src/dbSta",
        "//src/odb",
        "//src/rsz",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@boost.stacktrace",
        "@tcl_lang//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/rmp.tcl",
    ],
    char_array_name = "rmp_tcl_inits",
    namespace = "rmp",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/rmp.i",
        "//:error_swig",
        "//src/sta:sta_swig_files",
    ],
    module = "rmp",
    namespace_prefix = "rmp",
    root_swig_src = "src/rmp.i",
    swig_includes = [
        "../sta",
        "src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)
