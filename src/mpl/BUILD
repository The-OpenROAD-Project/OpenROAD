# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "mpl",
    srcs = [
        "src/MplObserver.h",
        "src/SACoreHardMacro.cpp",
        "src/SACoreHardMacro.h",
        "src/SACoreSoftMacro.cpp",
        "src/SACoreSoftMacro.h",
        "src/SimulatedAnnealingCore.cpp",
        "src/SimulatedAnnealingCore.h",
        "src/clusterEngine.cpp",
        "src/clusterEngine.h",
        "src/hier_rtlmp.cpp",
        "src/hier_rtlmp.h",
        "src/object.cpp",
        "src/object.h",
        "src/rtl_mp.cpp",
        "src/shapes.cpp",
        "src/shapes.h",
        "src/util.h",
    ],
    hdrs = [
        "include/mpl/rtl_mp.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//src/sta:opensta_lib",
        "//src/dbSta",
        "//src/odb",
        "//src/par",
        "//src/utl",
        "@boost.random",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "include/mpl/rtl_mp.h",
        "src/MakeMacroPlacer.cpp",
        "src/MplObserver.h",
        "src/clusterEngine.h",
        "src/graphics.cpp",
        "src/graphics.h",
        "src/object.h",
        "src/shapes.h",
        "src/util.h",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/mpl/MakeMacroPlacer.h",
    ],
    copts = [
        "-Isrc/mpl/src",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//:ord",
        "//src/gui",
        "//src/odb",
        "//src/utl",
        "@tk_tcl//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/mpl.tcl",
    ],
    char_array_name = "mpl_tcl_inits",
    namespace = "mpl",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/mpl.i",
        "//:error_swig",
    ],
    module = "mpl",
    namespace_prefix = "mpl",
    root_swig_src = "src/mpl.i",
    swig_includes = [
        "src/mpl/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)
