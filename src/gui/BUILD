# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

# This file only currently supports the gui disabled.  More work
# is needed to handle Qt and build a working gui.

load("//:bazel/tcl_encode_or.bzl", "tcl_encode")
load("//:bazel/tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "gui",
    srcs = [
        "src/stub.cpp",
        "src/stub_heatMap.cpp",
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

tcl_encode(
    name = "tcl",
    srcs = [
        "src/gui.tcl",
    ],
    char_array_name = "gui_tcl_inits",
    namespace = "gui",
)
