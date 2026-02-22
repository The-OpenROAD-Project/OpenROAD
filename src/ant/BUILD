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
    name = "ant",
    srcs = [
        "src/AntennaChecker.cc",
        "src/AntennaCheckerImpl.hh",
        "src/PinType.hh",
        "src/Polygon.cc",
        "src/Polygon.hh",
        "src/WireBuilder.cc",
        "src/WireBuilder.hh",
    ],
    hdrs = [
        "include/ant/AntennaChecker.hh",
    ],
    copts = [
        "-fopenmp",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//src/odb",
        "//src/utl",
        "@abseil-cpp//absl/synchronization",
        "@boost.container_hash",
        "@boost.graph",
        "@boost.polygon",
        "@openmp",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "src/MakeAntennaChecker.cc",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/ant/MakeAntennaChecker.hh",
    ],
    includes = ["include"],
    deps = [
        ":ant",
        "//:ord",
        "//src/odb",
        "//src/utl",
        "@boost.stacktrace",
        "@tk_tcl//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/AntennaChecker.tcl",
    ],
    char_array_name = "ant_tcl_inits",
    namespace = "ant",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/AntennaChecker.i",
        "//:error_swig",
    ],
    module = "ant",
    namespace_prefix = "ant",
    root_swig_src = "src/AntennaChecker.i",
    swig_includes = [
        "src",
    ],
)

python_wrap_cc(
    name = "swig-py",
    srcs = [
        "include/ant/AntennaChecker.hh",
        "src/AntennaChecker.i",
        "//:error_swig-py",
    ],
    module = "ant_py",
    root_swig_src = "src/AntennaChecker-py.i",
    swig_includes = [
        "include",
        "src",
    ],
    deps = [
        "//src/odb:swig-py",
    ],
)
