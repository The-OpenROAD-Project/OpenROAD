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
    name = "gpl",
    srcs = [
        "src/MakeReplace.cpp",
        "src/fft.cpp",
        "src/fft.h",
        "src/fftsg.cpp",
        "src/fftsg2d.cpp",
        "src/graphics.cpp",
        "src/graphics.h",
        "src/initialPlace.cpp",
        "src/initialPlace.h",
        "src/mbff.cpp",
        "src/mbff.h",
        "src/nesterovBase.cpp",
        "src/nesterovBase.h",
        "src/nesterovPlace.cpp",
        "src/nesterovPlace.h",
        "src/placerBase.cpp",
        "src/placerBase.h",
        "src/point.h",
        "src/replace.cpp",
        "src/routeBase.cpp",
        "src/routeBase.h",
        "src/solver.cpp",
        "src/solver.h",
        "src/timingBase.cpp",
        "src/timingBase.h",
        ":swig",
        ":swig-py",
        ":python",
        ":tcl",
    ],
    hdrs = [
        "include/gpl/MakeReplace.h",
        "include/gpl/Replace.h",
    ],
    copts = [
        "-fopenmp",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//:ord",
        "//src/dbSta",
        "//src/grt",
        "//src/gui",
        "//src/odb",
        "//src/rsz",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@boost.format",
        "@boost.geometry",
        "@boost.polygon",
        "@boost.property_tree",
        "@boost.random",
        "@boost.stacktrace",
        "@boost.tokenizer",
        "@boost.utility",
        "@com_github_quantamhd_lemon//:lemon",
        "@eigen",
        "@openmp",
        "@or-tools//ortools/linear_solver",
        "@or-tools//ortools/sat:cp_model",
        "@tk_tcl//:tcl",
        "@openroad_rules_python//python/cc:current_py_cc_headers",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/replace.tcl",
    ],
    char_array_name = "gpl_tcl_inits",
    namespace = "gpl",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/replace.i",
        "//:error_swig",
    ],
    module = "gpl",
    namespace_prefix = "gpl",
    root_swig_src = "src/replace.i",
    swig_includes = [
        "src/gpl/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)

python_wrap_cc(
    name = "swig-py",
    srcs = [
        "include/gpl/Replace.h",
        "src/replace-py.i",
        "//:error_swig-py",
    ],
    module = "gpl_py",
    root_swig_src = "src/replace-py.i",
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
    char_array_name = "gpl_py_python_inits",
    namespace = "gpl",
)
