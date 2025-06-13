# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "par",
    srcs = [
        "src/Coarsener.cpp",
        "src/Coarsener.h",
        "src/Evaluator.cpp",
        "src/Evaluator.h",
        "src/GreedyRefine.cpp",
        "src/GreedyRefine.h",
        "src/Hypergraph.cpp",
        "src/Hypergraph.h",
        "src/ILPRefine.cpp",
        "src/ILPRefine.h",
        "src/KWayFMRefine.cpp",
        "src/KWayFMRefine.h",
        "src/KWayPMRefine.cpp",
        "src/KWayPMRefine.h",
        "src/Multilevel.cpp",
        "src/Multilevel.h",
        "src/PartitionMgr.cpp",
        "src/Partitioner.cpp",
        "src/Partitioner.h",
        "src/PriorityQueue.cpp",
        "src/PriorityQueue.h",
        "src/Refiner.cpp",
        "src/Refiner.h",
        "src/TritonPart.cpp",
        "src/TritonPart.h",
        "src/Utilities.cpp",
        "src/Utilities.h",
    ],
    hdrs = [
        "include/par/PartitionMgr.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//:opensta_lib",
        "//src/dbSta",
        "//src/gui",
        "//src/odb",
        "//src/utl",
        "@boost.format",
        "@boost.geometry",
        "@boost.polygon",
        "@boost.property_tree",
        "@boost.random",
        "@boost.range",
        "@boost.tokenizer",
        "@boost.utility",
        "@or-tools//ortools/linear_solver:linear_solver",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "src/MakePartitionMgr.cpp",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/par/MakePartitionMgr.h",
        "include/par/PartitionMgr.h",
    ],
    copts = [
        "-Wno-missing-braces",  # from TCL swigging
    ],
    includes = [
        "include",
    ],
    deps = [
        "//:opensta_lib",
        "//:ord",
        "//src/dbSta",
        "//src/gui",
        "//src/odb",
        "//src/utl",
        "@boost.format",
        "@boost.geometry",
        "@boost.polygon",
        "@boost.property_tree",
        "@boost.random",
        "@boost.range",
        "@boost.tokenizer",
        "@boost.utility",
        "@tk_tcl//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/partitionmgr.tcl",
    ],
    char_array_name = "par_tcl_inits",
    namespace = "par",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/partitionmgr.i",
        "//:error_swig",
    ],
    module = "par",
    namespace_prefix = "par",
    root_swig_src = "src/partitionmgr.i",
    swig_includes = [
        "src/par/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)
