# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "ppl",
    srcs = [
        "src/AbstractIOPlacerRenderer.h",
        "src/Core.cpp",
        "src/Core.h",
        "src/HungarianMatching.cpp",
        "src/HungarianMatching.h",
        "src/IOPlacer.cpp",
        "src/IOPlacerRenderer.cpp",
        "src/IOPlacerRenderer.h",
        "src/MakeIoplacer.cpp",
        "src/Netlist.cpp",
        "src/Netlist.h",
        "src/SimulatedAnnealing.cpp",
        "src/SimulatedAnnealing.h",
        "src/Slots.cpp",
        "src/Slots.h",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/ppl/IOPlacer.h",
        "include/ppl/MakeIoplacer.h",
        "include/ppl/Parameters.h",
    ],
    copts = [
        "-Isrc/ppl/src",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":munkres",
        "//src/sta:opensta_lib",
        "//:ord",
        "//src/dbSta",
        "//src/gui",
        "//src/odb",
        "//src/utl",
        "@boost.container_hash",
        "@boost.random",
        "@tk_tcl//:tcl",
    ],
)

cc_library(
    name = "munkres",
    srcs = glob([
        "src/munkres/src/*.cpp",
    ]),
    hdrs = glob([
        "src/munkres/src/*.h",
    ]),
    includes = [
        "src/munkres/src",
    ],
    visibility = ["//:__subpackages__"],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/IOPlacer.tcl",
    ],
    char_array_name = "ppl_tcl_inits",
    namespace = "ppl",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/IOPlacer.i",
        "//:error_swig",
    ],
    module = "ppl",
    namespace_prefix = "ppl",
    root_swig_src = "src/IOPlacer.i",
    swig_includes = [
        "src/ppl/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)
