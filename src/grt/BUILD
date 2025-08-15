# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")
load("//bazel:python_wrap_cc.bzl", "python_wrap_cc")

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
    name = "fastroute",
    srcs = [
        "src/fastroute/src/FastRoute.cpp",
        "src/fastroute/src/FastRouteRenderer.cpp",
        "src/fastroute/src/graph2d.cpp",
        "src/fastroute/src/RSMT.cpp",
        "src/fastroute/src/RipUp.cpp",
        "src/fastroute/src/maze.cpp",
        "src/fastroute/src/maze3D.cpp",
        "src/fastroute/src/route.cpp",
        "src/fastroute/src/utility.cpp",
    ],
    hdrs = [
        "include/grt/GRoute.h",
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
        "//src/dbSta",
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
        "include/grt/GRoute.h",
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
        ":fastroute",
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
        ":swig-py",
        ":python",
        ":tcl",
    ],
    hdrs = [
        "include/grt/GRoute.h",
        "include/grt/GlobalRouter.h",
        "include/grt/MakeGlobalRouter.h",
        "include/grt/PinGridLocation.h",
        "include/grt/RoutePt.h",
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
        "@tk_tcl//:tcl",
        "@openroad_rules_python//python/cc:current_py_cc_headers",
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

tcl_encode(
    name = "python",
    srcs = [
        ":swig-py",
    ],
    char_array_name = "grt_py_python_inits",
    namespace = "grt",
)
