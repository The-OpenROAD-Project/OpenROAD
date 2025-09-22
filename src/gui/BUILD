# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

# This file only currently supports the gui disabled.  More work
# is needed to handle Qt and build a working gui.

load("@qt-bazel//:build_defs.bzl", "qt6_library", "qt6_resource_library")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "gui",
    srcs = [
        "src/stub.cpp",
        "src/stub_heatMap.cpp",
        ":tcl",
    ],
    hdrs = [
        "include/gui/MakeGui.h",
        "include/gui/gui.h",
        "include/gui/heatMap.h",
    ],
    includes = ["include"],
    deps = [
        "//src/odb",
        "@boost.multi_array",
        "@tk_tcl//:tcl",
    ],
)

qt6_library(
    name = "gui_qt",
    srcs = glob(
        include = [
            "src/*.cpp",
        ],
        exclude = [
            "src/stub.cpp",
            "src/stub_heatMap.cpp",
        ],
    ) + [
        ":gui_swig",
        ":tcl",
    ],
    hdrs = [
        "include/gui/MakeGui.h",
        "include/gui/gui.h",
        "include/gui/heatMap.h",
    ],
    defines = [
        "STATIC_QPA_PLUGIN_XCB=1",
    ],
    includes = [
        "include",
        "src",
        "ui",
    ],
    moc_hdrs = glob(["src/*.h"]),
    uic_srcs = glob(["ui/*.ui"]),
    deps = [
        ":qt_resources",
        "//:ord",
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/utl",
        "//third-party/gif-h:gif_h",
        "@boost.algorithm",
        "@boost.geometry",
        "@boost.multi_array",
        "@boost.stacktrace",
        "@spdlog",
        "@tk_tcl//:tcl",
    ],
)

qt6_resource_library(
    name = "qt_resources",
    data_srcs = glob([
        "resources/**/*.png",
    ]),
    external_init_name = "resource",
    qrc_srcs = [
        "resources/resource.qrc",
    ],
)

tcl_wrap_cc(
    name = "gui_swig",
    srcs = [
        "src/gui.i",
        "//:error_swig",
    ],
    module = "gui",
    namespace_prefix = "gui",
    root_swig_src = "src/gui.i",
    runtime_header = "src/tclSwig.h",
    swig_includes = [
        "src",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/gui.tcl",
    ],
    char_array_name = "gui_tcl_inits",
    namespace = "gui",
)
