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
        "//src/odb/src/db",
        "//src/utl",
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
        "//src/odb/src/db",
        "//src/utl",
        "@boost.random",
    ],
)

# Headers owned by :mpl but also consumed by :ui. Kept package-private so
# nothing outside //src/mpl can depend on them.
cc_library(
    name = "mpl_private_hdrs",
    hdrs = [
        "src/MplObserver.h",
        "src/clusterEngine.h",
    ],
    includes = [
        "src",
    ],
    visibility = ["//visibility:private"],
    deps = [
        ":pusher",
        "//src/odb/src/db",
    ],
)

cc_library(
    name = "mpl",
    srcs = [
        "src/SACoreHardMacro.cpp",
        "src/SACoreHardMacro.h",
        "src/SACoreSoftMacro.cpp",
        "src/SACoreSoftMacro.h",
        "src/SimulatedAnnealingCore.cpp",
        "src/SimulatedAnnealingCore.h",
        "src/clusterEngine.cpp",
        "src/hier_rtlmp.cpp",
        "src/hier_rtlmp.h",
        "src/rtl_mp.cpp",
    ],
    hdrs = [
        "include/mpl/rtl_mp.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":mpl_private_hdrs",
        ":pusher",
        ":snapper",
        "//src/dbSta:dbNetwork",
        "//src/odb/src/db",
        "//src/par",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@boost.polygon",
        "@boost.random",
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
        ":mpl",
        ":mpl_private_hdrs",
        ":pusher",
        "//:ord",
        "//src/gui",
        "//src/odb/src/db",
        "//src/utl",
        "@boost.stacktrace",
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
    ],
    module = "mpl",
    namespace_prefix = "mpl",
    root_swig_src = "src/mpl.i",
    swig_includes = [
        "src/mpl/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)

filegroup(
    name = "doc_files",
    srcs = [
        "README.md",
        "src/mpl.tcl",
    ],
    visibility = ["//src/mpl/test:__pkg__"],
)

messages_txt(
    visibility = [
        "//docs:__pkg__",
        "//src/mpl/test:__pkg__",
    ],
)
