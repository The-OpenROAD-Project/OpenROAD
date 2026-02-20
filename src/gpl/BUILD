# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@openroad_rules_python//python:defs.bzl", "py_library")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:python_wrap_cc.bzl", "PYTHON_STABLE_API_DEFINE", "python_wrap_cc")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "gpl",
    srcs = [
        "src/AbstractGraphics.cpp",
        "src/MakeReplace.cpp",
        "src/fft.cpp",
        "src/fft.h",
        "src/fftsg.cpp",
        "src/fftsg2d.cpp",
        "src/graphicsImpl.cpp",
        "src/graphicsImpl.h",
        "src/graphicsNone.cpp",
        "src/initialPlace.cpp",
        "src/initialPlace.h",
        "src/mbff.cpp",
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
        ":tcl",
    ],
    hdrs = [
        "include/gpl/MakeReplace.h",
        "include/gpl/Replace.h",
        "src/AbstractGraphics.h",
        "src/graphicsNone.h",
        "src/mbff.h",
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
        "//src/dbSta:dbNetwork",
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
        "@boost.unordered",
        "@boost.utility",
        "@coin-or-lemon//:lemon",
        "@eigen",
        "@openmp",
        "@or-tools//ortools/linear_solver",
        "@or-tools//ortools/sat:cp_model",
        "@tk_tcl//:tcl",
    ],
)

# This target compiles the SWIG C++ wrapper into a shared library (.so)
# that Python can load dynamically.
cc_binary(
    name = "_gpl.so",
    srcs = [
        ":swig-py",
    ],
    defines = [PYTHON_STABLE_API_DEFINE],
    includes = ["src"],
    linkshared = True,
    deps = [
        "//src/gpl",
        "//src/odb",
        "//src/utl",
        "@boost.stacktrace",
        "@openroad_rules_python//python/cc:current_py_cc_headers",
    ],
)

# This packages the SWIG-generated Python wrapper (gpl.py) and the
# compiled C++ extension (_gpl.so) together.
py_library(
    name = "gpl_py",
    srcs = [":swig-py"],  # Use the .py output from the swig-py rule
    # The data attribute makes the .so file available at runtime.
    data = [":_gpl.so"],
    # This allows imports relative to the workspace root.
    imports = ["."],
    visibility = ["//visibility:public"],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/replace.tcl",
    ],
    char_array_name = "gpl_tcl_inits",
    namespace = "gpl",
    visibility = ["//visibility:public"],
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/replace.i",
        "//:error_swig",
        "//:options_swig",
    ],
    module = "gpl",
    namespace_prefix = "gpl",
    root_swig_src = "src/replace.i",
    swig_includes = [
        "src/gpl/src",
    ],
    visibility = ["//visibility:public"],
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
    module = "gpl",
    root_swig_src = "src/replace-py.i",
    swig_includes = [
        "include",
        "src",
    ],
    visibility = ["//:__subpackages__"],
)
