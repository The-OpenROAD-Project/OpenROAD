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
    name = "dbSta",
    srcs = [
        "src/dbSta.cc",
    ],
    hdrs = [
        "include/db_sta/dbSta.hh",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":dbNetwork",
        ":dbSdcNetwork",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@abseil-cpp//absl/synchronization",
        "@boost.json",
        "@spdlog",
        "@tcl_lang//:tcl",
    ],
)

cc_library(
    name = "dbReadVerilog",
    srcs = [
        "src/dbReadVerilog.cc",
    ],
    hdrs = [
        "include/db_sta/dbReadVerilog.hh",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":dbNetwork",
        ":dbSta",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/utl",
    ],
)

cc_library(
    name = "SpefWriter",
    srcs = [
        "src/SpefWriter.cc",
    ],
    hdrs = [
        "include/db_sta/SpefWriter.hh",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":dbNetwork",
        ":dbSta",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/utl",
    ],
)

cc_library(
    name = "dbSdcNetwork",
    srcs = [
        "src/dbSdcNetwork.cc",
    ],
    hdrs = [
        "src/dbSdcNetwork.hh",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//src/sta:opensta_lib",
        "@spdlog",
    ],
)

cc_library(
    name = "dbNetwork",
    srcs = [
        "src/dbEditHierarchy.cc",
        "src/dbEditHierarchy.hh",
        "src/dbNetwork.cc",
    ],
    hdrs = [
        "include/db_sta/dbNetwork.hh",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/utl",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "src/MakeDbSta.cc",
        ":swig",
        ":tcl",
        "//src/sta:StaConfig",
    ],
    hdrs = [
        "include/db_sta/MakeDbSta.hh",
    ],
    copts = [
        "-Isrc/dbSta/src",
        "-Wno-missing-braces",  # from TCL swigging
    ],
    includes = [
        "include",
    ],
    deps = [
        ":dbNetwork",
        ":dbSta",
        "//:ord",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@boost.json",
        "@boost.stacktrace",
        "@tcl_lang//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    # Encode sta:tcl_scripts before dbSta.tcl, as dbSta.tcl will
    # override sta_error and sta_warn.
    srcs = ["//src/sta:tcl_scripts"] + [
        "src/dbReadVerilog.tcl",
        "src/dbSta.tcl",
    ],
    char_array_name = "dbSta_tcl_inits",
    namespace = "sta",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/dbSta.i",
        "//:error_swig",
        "//src/sta:sta_swig_files",
    ],
    module = "dbSta",
    namespace_prefix = "sta",
    root_swig_src = "src/dbSta.i",
    swig_includes = [
        "../../src/sta",
        "src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)
