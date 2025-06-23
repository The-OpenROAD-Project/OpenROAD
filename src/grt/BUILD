# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "types",
    hdrs = [
        "include/grt/GRoute.h",
        "include/grt/GlobalRouter.h",
        "include/grt/PinGridLocation.h",
        "include/grt/RoutePt.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//src/odb",
    ],
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
    name = "fastroute",
    srcs = [
        "src/fastroute/src/FastRoute.cpp",
        "src/fastroute/src/FastRouteRenderer.cpp",
        "src/fastroute/src/RSMT.cpp",
        "src/fastroute/src/RipUp.cpp",
        "src/fastroute/src/maze.cpp",
        "src/fastroute/src/maze3D.cpp",
        "src/fastroute/src/route.cpp",
        "src/fastroute/src/utility.cpp",
    ],
    hdrs = [
        "src/fastroute/include/AbstractMakeWireParasitics.h",
        "src/fastroute/include/FastRoute.h",
        "src/fastroute/include/FastRouteRenderer.h",
    ],
    includes = [
        "src/fastroute/include",
    ],
    deps = [
        ":abstract-fastroute",
        ":types",
        "//src/gui",
        "//src/odb",
        "//src/stt",
        "//src/utl",
        "@boost.container_hash",
        "@boost.icl",
        "@boost.multi_array",
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
        "src/MakeWireParasitics.cpp",
        "src/MakeWireParasitics.h",
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
        ":fastroute",
        ":types",
        "//src/ant:types",
        "//src/sta:opensta_lib",
        "//src/ant",
        "//src/dbSta",
        "//src/dpl",
        "//src/gui",
        "//src/odb",
        "//src/rsz",
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
        "include/grt/Rudy.h",
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
        ":fastroute",
        ":types",
        "//src/sta:opensta_lib",
        "//:ord",
        "//src/ant",
        "//src/dbSta",
        "//src/dpl",
        "//src/gui",
        "//src/odb",
        "//src/rsz",
        "//src/stt",
        "//src/utl",
        "@boost.geometry",
        "@boost.graph",
        "@boost.icl",
        "@boost.iterator",
        "@openmp",
        "@tk_tcl//:tcl",
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
