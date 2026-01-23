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
    name = "rsz",
    srcs = [
        "src/BaseMove.cc",
        "src/BaseMove.hh",
        "src/BufferMove.cc",
        "src/BufferMove.hh",
        "src/BufferedNet.cc",
        "src/BufferedNet.hh",
        "src/CloneMove.cc",
        "src/CloneMove.hh",
        "src/ConcreteSwapArithModules.cc",
        "src/ConcreteSwapArithModules.hh",
        "src/OdbCallBack.cc",
        "src/PreChecks.cc",
        "src/PreChecks.hh",
        "src/Rebuffer.cc",
        "src/Rebuffer.hh",
        "src/RecoverPower.cc",
        "src/RecoverPower.hh",
        "src/RepairDesign.cc",
        "src/RepairDesign.hh",
        "src/RepairHold.cc",
        "src/RepairHold.hh",
        "src/RepairSetup.cc",
        "src/RepairSetup.hh",
        "src/Resizer.cc",
        "src/ResizerObserver.hh",
        "src/SizeDownMove.cc",
        "src/SizeDownMove.hh",
        "src/SizeUpMove.cc",
        "src/SizeUpMove.hh",
        "src/SplitLoadMove.cc",
        "src/SplitLoadMove.hh",
        "src/SwapArithModules.hh",
        "src/SwapPinsMove.cc",
        "src/SwapPinsMove.hh",
        "src/UnbufferMove.cc",
        "src/UnbufferMove.hh",
        "src/VTSwapMove.cc",
        "src/VTSwapMove.hh",
    ],
    hdrs = [
        "include/rsz/OdbCallBack.hh",
        "include/rsz/Resizer.hh",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//src/dbSta",
        "//src/dbSta:SpefWriter",
        "//src/dbSta:dbNetwork",
        "//src/dpl",
        "//src/est",
        "//src/grt",
        "//src/grt:groute",
        "//src/gui",
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
        "src/BufferedNet.hh",
        "src/Graphics.cc",
        "src/Graphics.hh",
        "src/MakeResizer.cc",
        "src/ResizerObserver.hh",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/rsz/MakeResizer.hh",
    ],
    copts = [
        "-Isrc/rsz/src",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":rsz",
        "//:ord",
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/dpl",
        "//src/est",
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
        "@tk_tcl//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/Resizer.tcl",
    ],
    char_array_name = "rsz_tcl_inits",
    namespace = "rsz",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/Resizer.i",
        "//:error_swig",
        "//src/sta:sta_swig_files",
    ],
    module = "rsz",
    namespace_prefix = "rsz",
    root_swig_src = "src/Resizer.i",
    swig_includes = [
        "../sta",
        "src",
    ],
)
