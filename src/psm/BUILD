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
    name = "psm",
    srcs = [
        "src/MakePDNSim.cpp",
        "src/connection.cpp",
        "src/connection.h",
        "src/debug_gui.cpp",
        "src/debug_gui.h",
        "src/heatMap.cpp",
        "src/heatMap.h",
        "src/ir_network.cpp",
        "src/ir_network.h",
        "src/ir_solver.cpp",
        "src/ir_solver.h",
        "src/node.cpp",
        "src/node.h",
        "src/pdnsim.cpp",
        "src/shape.cpp",
        "src/shape.h",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/psm/MakePDNSim.hh",
        "include/psm/pdnsim.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//:ord",
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/dpl",
        "//src/est",
        "//src/gui",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@boost.geometry",
        "@boost.polygon",
        "@boost.stacktrace",
        "@eigen",
        "@tcl_lang//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/pdnsim.tcl",
    ],
    char_array_name = "psm_tcl_inits",
    namespace = "psm",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/pdnsim.i",
        "//:error_swig",
        "//src/sta:sta_swig_files",
    ],
    module = "psm",
    namespace_prefix = "psm",
    root_swig_src = "src/pdnsim.i",
    swig_includes = [
        "../sta",
        "src/psm/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)

python_wrap_cc(
    name = "swig-py",
    srcs = [
        "include/psm/pdnsim.h",
        "src/pdnsim-py.i",
        "//:error_swig-py",
    ],
    module = "psm_py",
    root_swig_src = "src/pdnsim-py.i",
    swig_includes = [
        "include",
    ],
    deps = [
        "//src/odb:swig-py",
    ],
)
