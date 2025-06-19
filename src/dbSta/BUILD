# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "dbSta",
    srcs = [
        "src/SpefWriter.cc",
        "src/dbNetwork.cc",
        "src/dbReadVerilog.cc",
        "src/dbSdcNetwork.cc",
        "src/dbSdcNetwork.hh",
        "src/dbSta.cc",
    ],
    hdrs = [
        "include/db_sta/SpefWriter.hh",
        "include/db_sta/dbNetwork.hh",
        "include/db_sta/dbReadVerilog.hh",
        "include/db_sta/dbSta.hh",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//src/sta:opensta_lib",
        "//src/odb",
        "//src/utl",
        "@boost.json",
        "@tk_tcl//:tcl",
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
        "include/db_sta/SpefWriter.hh",
        "include/db_sta/dbNetwork.hh",
        "include/db_sta/dbReadVerilog.hh",
        "include/db_sta/dbSta.hh",
    ],
    copts = [
        "-Isrc/dbSta/src",
        "-Wno-missing-braces",  # from TCL swigging
    ],
    includes = [
        "include",
    ],
    deps = [
        "//src/sta:opensta_lib",
        "//:ord",
        "//src/odb",
        "//src/utl",
        "@boost.json",
        "@tk_tcl//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/dbReadVerilog.tcl",
        "src/dbSta.tcl",
        "//src/sta:tcl_scripts",
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
