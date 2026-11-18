# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2026, The OpenROAD Authors

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
    name = "utils",
    srcs = ["src/utils.cpp"],
    hdrs = ["src/utils.h"],
    defines = ["ABC_NAMESPACE=abc"],
    includes = ["src"],
    visibility = ["//visibility:private"],
    deps = [
        "//openroad/cut",
        "//openroad/dbSta",
        "//openroad/dbSta:dbNetwork",
        "//openroad/rsz",
        "//openroad/sta:opensta_lib",
        "//openroad/utl",
        "@abc",
    ],
)

cc_library(
    name = "optimization_strategies",
    srcs = [
        "src/delay_optimization_strategy.cpp",
        "src/zero_slack_strategy.cpp",
    ],
    hdrs = [
        "src/delay_optimization_strategy.h",
        "src/logic_optimization_strategy.h",
        "src/resynthesis_strategy.h",
        "src/zero_slack_strategy.h",
    ],
    defines = ["ABC_NAMESPACE=abc"],
    includes = ["src"],
    visibility = ["//openroad/rmp/test:__pkg__"],
    deps = [
        ":utils",
        "//openroad/cut",
        "//openroad/dbSta",
        "//openroad/dbSta:dbNetwork",
        "//openroad/rsz",
        "//openroad/sta:opensta_lib",
        "//openroad/utl",
        "@abc",
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/synchronization",
    ],
)

cc_library(
    name = "rmp",
    srcs = [
        "src/Restructure.cpp",
        "src/annealing_strategy.cpp",
        "src/annealing_strategy.h",
        "src/genetic_strategy.cpp",
        "src/genetic_strategy.h",
        "src/gia.cpp",
        "src/gia.h",
        "src/slack_tuning_strategy.cpp",
        "src/slack_tuning_strategy.h",
    ],
    hdrs = [
        "include/rmp/Restructure.h",
    ],
    defines = ["ABC_NAMESPACE=abc"],
    includes = [
        "include",
        "src",
    ],
    deps = [
        ":optimization_strategies",
        ":utils",
        "//openroad/cut",
        "//openroad/dbSta",
        "//openroad/dbSta:dbNetwork",
        "//openroad/odb/src/db",
        "//openroad/rsz",
        "//openroad/sta:opensta_lib",
        "//openroad/utl",
        "@abc",
        "@abseil-cpp//absl/hash",
        "@abseil-cpp//absl/random:distributions",
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
    includes = [
        "include",
        "src",
    ],
    deps = [
        ":rmp",
        "//:ord",
        "//openroad/cut",
        "//openroad/odb/src/db",
        "//openroad/sta:opensta_lib",
        "//openroad/utl",
        "@tcl_lang//:tcl",
    ],
)

# The rmp module is part of //:openroad_py.
alias(
    name = "rmp_py",
    actual = "//:openroad_py",
    visibility = ["//visibility:public"],
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
        "//openroad/sta:sta_swig_files",
    ],
    module = "rmp",
    namespace_prefix = "rmp",
    root_swig_src = "src/rmp.i",
    swig_includes = [
        "../sta",
        "src",
    ],
    deps = [
        "//openroad/odb:swig",
    ],
)

python_wrap_cc(
    name = "swig-py",
    srcs = [
        "src/rmp-py.i",
        "//:error_swig-py",
    ],
    module = "rmp_py",
    root_swig_src = "src/rmp-py.i",
    swig_includes = [
        "../../include",
        "../cut/include",
        "../sta/include",
        "include",
        "src",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//openroad/odb:swig-py",
    ],
)

filegroup(
    name = "doc_files",
    srcs = [
        "README.md",
        "src/rmp.tcl",
    ],
    visibility = ["//openroad/rmp/test:__pkg__"],
)

messages_txt(
    visibility = [
        "//docs:__pkg__",
        "//openroad/rmp/test:__pkg__",
    ],
)

python_wrap_library(
    name = "py_wrap",
    includes = ["src"],
    deps = [
        ":rmp",
        "//openroad/cut",
        "//openroad/dbSta",
        "//openroad/sta:opensta_lib",
    ],
)
