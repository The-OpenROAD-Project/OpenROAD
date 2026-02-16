# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:python_wrap_cc.bzl", "python_wrap_cc")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "abstract-fastroute",
    hdrs = [
        "src/fastroute/include/AbstractFastRouteRenderer.h",
        "src/fastroute/include/DataType.h",
    ],
    visibility = ["//visibility:private"],
    deps = [
        "//src/stt",
    ],
)

cc_library(
    name = "groute",
    hdrs = ["include/grt/GRoute.h"],
)

cc_library(
    name = "fastroute",
    srcs = [
        "src/fastroute/src/FastRoute.cpp",
        "src/fastroute/src/FastRouteRenderer.cpp",
        "src/fastroute/src/RSMT.cpp",
        "src/fastroute/src/RipUp.cpp",
        "src/fastroute/src/graph2d.cpp",
        "src/fastroute/src/maze.cpp",
        "src/fastroute/src/maze3D.cpp",
        "src/fastroute/src/route.cpp",
        "src/fastroute/src/utility.cpp",
    ],
    hdrs = [
        "src/fastroute/include/FastRoute.h",
        "src/fastroute/include/FastRouteRenderer.h",
        "src/fastroute/include/Graph2D.h",
    ],
    includes = [
        "include",
        "src/fastroute/include",
    ],
    deps = [
        ":abstract-fastroute",
        ":groute",
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/gui",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/stt",
        "//src/utl",
        "@boost.container_hash",
        "@boost.icl",
        "@boost.multi_array",
    ],
)

cc_library(
    name = "cugr",
    srcs = [
        "src/cugr/src/CUGR.cpp",
        "src/cugr/src/Design.cpp",
        "src/cugr/src/Design.h",
        "src/cugr/src/GRNet.cpp",
        "src/cugr/src/GRNet.h",
        "src/cugr/src/GRTree.cpp",
        "src/cugr/src/GRTree.h",
        "src/cugr/src/GeoTypes.cpp",
        "src/cugr/src/GeoTypes.h",
        "src/cugr/src/GridGraph.cpp",
        "src/cugr/src/GridGraph.h",
        "src/cugr/src/Layers.cpp",
        "src/cugr/src/Layers.h",
        "src/cugr/src/MazeRoute.cpp",
        "src/cugr/src/MazeRoute.h",
        "src/cugr/src/Netlist.cpp",
        "src/cugr/src/Netlist.h",
        "src/cugr/src/PatternRoute.cpp",
        "src/cugr/src/PatternRoute.h",
        "src/cugr/src/geo.h",
        "src/cugr/src/robin_hood.h",
    ],
    hdrs = [
        "src/cugr/include/CUGR.h",
    ],
    includes = [
        "include",
        "src/cugr/include",
    ],
    deps = [
        ":groute",
        "//:ord",
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/stt",
        "//src/utl",
    ],
)

cc_library(
    name = "grt",
    srcs = [
        "src/AbstractGrouteRenderer.h",
        "src/AbstractRoutingCongestionDataSource.h",
        "src/GlobalRouter.cpp",
        "src/Grid.cpp",
        "src/Grid.h",
        "src/Net.cpp",
        "src/Net.h",
        "src/Pin.cpp",
        "src/Pin.h",
        "src/RepairAntennas.cpp",
        "src/RepairAntennas.h",
        "src/RoutingTracks.h",
        "src/Rudy.cpp",
    ],
    hdrs = [
        "include/grt/GlobalRouter.h",
        "include/grt/PinGridLocation.h",
        "include/grt/RoutePt.h",
        "include/grt/Rudy.h",
    ],
    copts = [
        "-fopenmp",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":abstract-fastroute",
        ":cugr",
        ":fastroute",
        ":groute",
        "//src/ant",
        "//src/dbSta",
        "//src/dbSta:SpefWriter",
        "//src/dbSta:dbNetwork",
        "//src/dpl",
        "//src/gui",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/stt",
        "//src/utl",
        "@boost.geometry",
        "@boost.graph",
        "@boost.icl",
        "@boost.iterator",
        "@boost.polygon",
        "@openmp",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "src/AbstractGrouteRenderer.h",
        "src/AbstractRoutingCongestionDataSource.h",
        "src/GrouteRenderer.cpp",
        "src/GrouteRenderer.h",
        "src/MakeGlobalRouter.cpp",
        "src/Net.h",
        "src/Pin.h",
        "src/heatMap.cpp",
        "src/heatMap.h",
        "src/heatMapRudy.cpp",
        "src/heatMapRudy.h",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/grt/MakeGlobalRouter.h",
    ],
    copts = [
        "-Isrc/grt/src",
        "-Wno-missing-braces",  # from TCL swigging
        "-fopenmp",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":abstract-fastroute",
        ":cugr",
        ":fastroute",
        ":groute",
        ":grt",
        "//:ord",
        "//src/ant",
        "//src/dbSta",
        "//src/dpl",
        "//src/gui",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/stt",
        "//src/utl",
        "@boost.geometry",
        "@boost.graph",
        "@boost.icl",
        "@boost.iterator",
        "@boost.stacktrace",
        "@openmp",
        "@tcl_lang//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/GlobalRouter.tcl",
    ],
    char_array_name = "grt_tcl_inits",
    namespace = "grt",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/GlobalRouter.i",
        "//:error_swig",
    ],
    module = "grt",
    namespace_prefix = "grt",
    root_swig_src = "src/GlobalRouter.i",
    swig_includes = [
        "src",
    ],
)

python_wrap_cc(
    name = "swig-py",
    srcs = [
        "include/grt/GlobalRouter.h",
        "src/GlobalRouter-py.i",
        "//:error_swig-py",
    ],
    module = "grt_py",
    root_swig_src = "src/GlobalRouter-py.i",
    swig_includes = [
        "include",
        "src",
    ],
)
