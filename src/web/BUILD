# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "web",
    srcs = [
        "src/color.cpp",
        "src/color.h",
        "src/search.cpp",
        "src/search.h",
        "src/tile_generator.h",
        "src/web.cpp",
    ],
    hdrs = [
        "include/web/web.h",
    ],
    includes = [
        "include",
    ],
    copts = [
        "-Isrc/web/src",
    ],
    deps = [
        "//src/dbSta",
        "//src/gui",
        "//src/odb",
        "//src/utl",
        "//third-party/lodepng",
        "@boost.asio",
        "@boost.beast",
        "@boost.geometry",
        "@tk_tcl//:tcl",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "src/MakeWeb.cpp",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/web/MakeWeb.h",
    ],
    copts = [
        "-Isrc/web/src",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":web",
        "//:ord",
        "//src/odb",
        "//src/utl",
        "@tk_tcl//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/web.tcl",
    ],
    char_array_name = "web_tcl_inits",
    namespace = "web",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/web.i",
        "//:error_swig",
    ],
    module = "web",
    namespace_prefix = "web",
    root_swig_src = "src/web.i",
    swig_includes = [
        "src/web/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)
