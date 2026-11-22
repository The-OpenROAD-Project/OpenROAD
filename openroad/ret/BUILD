# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")
load("//test:regression.bzl", "messages_txt")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "ret",
    srcs = [
        "src/RegisterTiming.cpp",
        "src/RegisterTiming.h",
        "src/RegisterUtils.cpp",
        "src/RegisterUtils.h",
        "src/Retime.cpp",
        "src/Retime.h",
        "src/RetimeModel.cpp",
        "src/Retimer.cpp",
        "src/RetimerPlacer.cpp",
        "src/RetimerPlacer.h",
        "src/RetimingGraph.cpp",
        "src/RetimingGraph.h",
        "src/SequentialUnmapper.cpp",
        "src/SequentialUnmapper.h",
        "src/StaCheck.cpp",
        "src/StaCheck.h",
    ],
    hdrs = [
        "include/ret/Retimer.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//openroad/dbSta",
        "//openroad/dbSta:dbNetwork",
        "//openroad/est",
        "//openroad/odb/src/db",
        "//openroad/rsz",
        "//openroad/sta:opensta_lib",
        "//openroad/utl",
    ],
)

# The private register unmapper, for the unit tests in test.
cc_library(
    name = "ret_internal",
    testonly = True,
    hdrs = [
        "src/SequentialUnmapper.h",
    ],
    strip_include_prefix = "src",
    visibility = ["//openroad/ret/test:__pkg__"],
    deps = [
        ":ret",
        "//openroad/odb/src/db",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "src/MakeRet.cpp",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/ret/MakeRet.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":ret",
        "//:ord",
        "//openroad/odb/src/db",
        "//openroad/utl",
        "@tcl_lang//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/ret.tcl",
    ],
    char_array_name = "ret_tcl_inits",
    namespace = "ret",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/ret.i",
        "//:error_swig",
    ],
    module = "ret",
    namespace_prefix = "ret",
    root_swig_src = "src/ret.i",
    swig_includes = [
        "openroad/ret/src",
    ],
    deps = [
        "//openroad/odb:swig",
    ],
)

exports_files(["README.md"])

messages_txt(
    visibility = ["//docs:__pkg__"],
)
