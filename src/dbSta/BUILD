# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_python//python:defs.bzl", "py_binary")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")
load("//test:regression.bzl", "messages_txt")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "lambertw_delay_calc",
    srcs = ["src/DmpCeffLambertWDelayCalc.cc"],
    hdrs = ["src/DmpCeffLambertWDelayCalc.hh"],
    deps = [
        "//src/sta:opensta_lib",
        "@boost.math",
    ],
    features = ["-layering_check"]
)

cc_library(
    name = "dbSta",
    srcs = [
        "src/dbSta.cc",
    ],
    hdrs = [
        "include/db_sta/DelayFmt.hh",
        "include/db_sta/dbSta.hh",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":dbNetwork",
        ":dbSdcNetwork",
        ":lambertw_delay_calc",
        "//src/odb/src/db",
        "//src/sta:opensta_lib",
        "//src/utl",
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
        "//src/odb/src/db",
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
        "//src/odb/src/db",
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
        "src",
    ],
    visibility = ["//visibility:private"],
    deps = [
        "//src/sta:opensta_lib",
        "@spdlog",
    ],
)

cc_library(
    name = "IpChecker",
    srcs = [
        "src/IpChecker.cc",
    ],
    hdrs = [
        "include/db_sta/IpChecker.hh",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":dbSta",
        "//src/odb/src/db",
        "//src/utl",
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
        "//src/odb/src/db",
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
        "-Wno-missing-braces",  # from TCL swigging
    ],
    includes = [
        "include",
        "src",
    ],
    deps = [
        ":IpChecker",
        ":dbNetwork",
        ":dbSta",
        "//:ord",
        "//src/odb/src/db",
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

messages_txt(
    visibility = ["//docs:__pkg__"],
)

cc_binary(
    name = "ceff_training_data_generator",
    srcs = ["src/ceff_training_data_generator.cc"],
    data = [
        "//src/dbSta/test:synthetic_linear.lib",
    ],
    deps = [
        "//src/sta:opensta_lib",
    ],
    features = ["-layering_check"],
)

py_binary(
    name = "fit_pade",
    srcs = ["src/fit_pade.py"],
    deps = [
        "@openroad-pip//numpy",
        "@openroad-pip//pandas",
        "@openroad-pip//scipy",
    ],
)

genrule(
    name = "generate_training_data",
    srcs = ["//src/dbSta/test:synthetic_linear.lib"],
    outs = ["dmp_training_data.csv"],
    cmd = "$(location :ceff_training_data_generator) $(location //src/dbSta/test:synthetic_linear.lib) > $@",
    tools = [":ceff_training_data_generator"],
)

genrule(
    name = "generate_pade_coefficients",
    srcs = [":dmp_training_data.csv"],
    outs = ["PadeCoefficients.h"],
    cmd = "$(location :fit_pade) --csv_path=$< --output=$@",
    tools = [":fit_pade"],
)
