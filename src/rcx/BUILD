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
    name = "rcx",
    srcs = [
        "src/dbUtil.cpp",
        "src/ext.cpp",
        "src/extBench.cpp",
        "src/extBenchDB.cpp",
        "src/extCC.cpp",
        "src/extCoords.cpp",
        "src/extDebugPrint.cpp",
        "src/extDefPatterns.cpp",
        "src/extDefPatternsVia.cpp",
        "src/extFlow.cpp",
        "src/extFlow_v2.cpp",
        "src/extModelGen.cpp",
        "src/extParse.h",
        "src/extPatterns.cpp",
        "src/extRCmodel.cpp",
        "src/extRCmodel_process.cpp",
        "src/extRCmodel_solver.cpp",
        "src/extRulesVia.cpp",
        "src/extSegment.cpp",
        "src/extSolverGen.cpp",
        "src/extSpef.cpp",
        "src/extSpefIn.cpp",
        "src/extmain.cpp",
        "src/extmain_v2.cpp",
        "src/extmeasure.cpp",
        "src/extmeasure_calc.cpp",
        "src/extmeasure_couple.cpp",
        "src/extmeasure_diag.cpp",
        "src/extmeasure_diag_opt.cpp",
        "src/extmeasure_dist.cpp",
        "src/extmeasure_flow.cpp",
        "src/extmeasure_print.cpp",
        "src/extmeasure_rc_flow.cpp",
        "src/extmeasure_res.cpp",
        "src/extmeasure_res_v2.cpp",
        "src/extprocess.cpp",
        "src/extstats.cpp",
        "src/find_some_net.cpp",
        "src/find_some_net.h",
        "src/findsegments.cpp",
        "src/grids.cpp",
        "src/gs.cpp",
        "src/gseq.h",
        "src/name.cpp",
        "src/name.h",
        "src/netRC.cpp",
        "src/parse.cpp",
        "src/parse.h",
        "src/process_ext.cpp",
    ],
    hdrs = [
        "include/rcx/array1.h",
        "include/rcx/box.h",
        "include/rcx/dbUtil.h",
        "include/rcx/ext.h",
        "include/rcx/ext2dBox.h",
        "include/rcx/extMeasureRC.h",
        "include/rcx/extModelGen.h",
        "include/rcx/extPattern.h",
        "include/rcx/extProgressTracker.hpp",
        "include/rcx/extRCap.h",
        "include/rcx/extRulesPattern.h",
        "include/rcx/extSegment.h",
        "include/rcx/extSolverGen.h",
        "include/rcx/extSpef.h",
        "include/rcx/extViaModel.h",
        "include/rcx/ext_options.h",
        "include/rcx/extprocess.h",
        "include/rcx/grids.h",
        "include/rcx/rcx.h",
        "include/rcx/util.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//src/odb",
        "//src/utl",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "src/MakeOpenRCX.cpp",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/rcx/MakeOpenRCX.h",
    ],
    copts = [
        "-Isrc/rcx/src",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":rcx",
        "//:ord",
        "//src/odb",
        "//src/utl",
        "@boost.stacktrace",
        "@tcl_lang//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/OpenRCX.tcl",
    ],
    char_array_name = "rcx_tcl_inits",
    namespace = "rcx",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/ext.i",
        "//:error_swig",
    ],
    module = "rcx",
    namespace_prefix = "rcx",
    root_swig_src = "src/ext.i",
    swig_includes = [
        "src/rcx/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)

python_wrap_cc(
    name = "swig-py",
    srcs = [
        "include/rcx/ext.h",
        "include/rcx/ext_options.h",
        "src/ext-py.i",
        "//:error_swig-py",
    ],
    module = "rcx_py",
    root_swig_src = "src/ext-py.i",
    swig_includes = [
        "include",
    ],
    deps = [
        "//src/odb:swig-py",
    ],
)
