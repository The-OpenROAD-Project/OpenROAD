# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")
load("//test:regression.bzl", "messages_txt")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "snapper",
    srcs = [
        "src/snapper.cpp",
    ],
    hdrs = [
        "src/snapper.h",
    ],
    includes = [
        "src",
    ],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/utl",
    ],
)

cc_library(
    name = "pusher",
    srcs = [
        "src/object.cpp",
        "src/pusher.cpp",
        "src/shapes.cpp",
    ],
    hdrs = [
        "src/mpl-util.h",
        "src/object.h",
        "src/pusher.h",
        "src/shapes.h",
    ],
    includes = [
        "src",
    ],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/utl",
        "@boost.random",
    ],
)

cc_library(
    name = "mpl_impl",
    srcs = [
        "src/SACoreHardMacro.cpp",
        "src/SACoreHardMacro.h",
        "src/SACoreSoftMacro.cpp",
        "src/SACoreSoftMacro.h",
        "src/SimulatedAnnealingCore.cpp",
        "src/SimulatedAnnealingCore.h",
        "src/clusterEngine.cpp",
        "src/feasibilityChecker.cpp",
        "src/feasibilityChecker.h",
        "src/hardMacroFactory.cpp",
        "src/hardMacroFactory.h",
        "src/hier_rtlmp.cpp",
        "src/hier_rtlmp.h",
        "src/rtl_mp.cpp",
    ],
    hdrs = [
        "include/mpl/rtl_mp.h",
        "src/MplObserver.h",
        "src/clusterEngine.h",
    ],
    includes = [
        "include",
        "src",
    ],
    visibility = [
        "//openroad/mpl:__subpackages__",
    ],
    deps = [
        ":pusher",
        ":snapper",
        "//openroad/dbSta:dbNetwork",
        "//openroad/odb/src/db",
        "//openroad/par",
        "//openroad/sta:opensta_lib",
        "//openroad/utl",
        "@boost.polygon",
        "@boost.random",
        "@fmt",
    ],
)

cc_library(
    name = "mpl",
    hdrs = [
        "include/mpl/rtl_mp.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":mpl_impl",
        "//openroad/odb/src/db",
        "//openroad/utl",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "src/MakeMacroPlacer.cpp",
        "src/graphics.cpp",
        "src/graphics.h",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/mpl/MakeMacroPlacer.h",
    ],
    includes = [
        "include",
        "src",
    ],
    deps = [
        ":mpl_impl",
        ":pusher",
        "//:ord",
        "//openroad/odb/src/db",
        "//openroad/utl",
        "//openroad/web:core",
        "@tcl_lang//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/mpl.tcl",
    ],
    char_array_name = "mpl_tcl_inits",
    namespace = "mpl",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/mpl.i",
        "//:error_swig",
        "//:options_swig",
    ],
    module = "mpl",
    namespace_prefix = "mpl",
    root_swig_src = "src/mpl.i",
    swig_includes = [
        "src",
    ],
    deps = [
        "//openroad/odb:swig",
    ],
)

filegroup(
    name = "doc_files",
    srcs = [
        "README.md",
        "src/mpl.tcl",
    ],
    visibility = ["//openroad/mpl/test:__pkg__"],
)

messages_txt(
    visibility = [
        "//docs:__pkg__",
        "//openroad/mpl/test:__pkg__",
    ],
)
