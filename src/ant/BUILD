# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

# Broken out due to cyclic dependency between grt and ant
cc_library(
    name = "types",
    hdrs = ["include/ant/GlobalRouteSource.hh"],
    includes = ["include"],
)

cc_library(
    name = "make-antenna-checker",
    srcs = [
        "src/MakeAntennaChecker.cc",
    ],
    hdrs = [
        "include/ant/MakeAntennaChecker.hh",
    ],
    includes = ["include"],
    deps = [
        ":ant",
        "//:ord",
        "//src/grt:types",
        "//src/utl",
        "@tk_tcl//:tcl",
    ],
)

cc_library(
    name = "ant",
    srcs = [
        "src/AntennaChecker.cc",
        "src/Polygon.cc",
    ],
    hdrs = [
        "include/ant/AntennaChecker.hh",
        "src/PinType.hh",
        "src/Polygon.hh",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":types",
        "//:ord",
        "//src/odb",
        "//src/utl",
        "@boost.container_hash",
        "@boost.graph",
        "@boost.polygon",
        "@org_llvm_openmp//:openmp",
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
