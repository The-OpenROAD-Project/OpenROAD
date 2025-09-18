# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    # ABC headers have duplicate declarations which cause warnings/compile errors.
    # This was solved by including more specific individual headers, but those are private, so the layering check is disabled.
    # features = ["layering_check"],
)

cc_library(
    name = "cgt",
    srcs = [
        "src/ClockGating.cpp",
        "src/MakeClockGating.cpp",
        "src/NetworkBuilder.cpp",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/cgt/ClockGating.h",
        "include/cgt/MakeClockGating.h",
        "include/cgt/NetworkBuilder.h",
        "include/cgt/RandomBits.h",
    ],
    defines = [
        "ABC_NAMESPACE=abc",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//:ord",
        "//src/cut",
        "//src/dbSta",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@boost.stacktrace",
        "@edu_berkeley_abc//:abc-lib",
        "@tk_tcl//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/cgt.tcl",
    ],
    char_array_name = "cgt_tcl_inits",
    namespace = "cgt",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/cgt.i",
        "//:error_swig",
    ],
    module = "cgt",
    namespace_prefix = "cgt",
    root_swig_src = "src/cgt.i",
)
