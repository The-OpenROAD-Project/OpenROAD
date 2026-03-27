# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@aspect_rules_js//js:defs.bzl", "js_library")
load("@npm//:defs.bzl", "npm_link_all_packages")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

npm_link_all_packages(name = "node_modules")

cc_library(
    name = "web",
    srcs = [
        "src/clock_tree_report.cpp",
        "src/clock_tree_report.h",
        "src/color.cpp",
        "src/color.h",
        "src/hierarchy_report.cpp",
        "src/hierarchy_report.h",
        "src/json_builder.h",
        "src/request_handler.cpp",
        "src/request_handler.h",
        "src/search.cpp",
        "src/search.h",
        "src/tile_generator.cpp",
        "src/tile_generator.h",
        "src/timing_report.cpp",
        "src/timing_report.h",
        "src/web.cpp",
    ],
    hdrs = [
        "include/web/web.h",
    ],
    copts = [
        "-DBOOST_ASIO_NO_DEPRECATED",
    ],
    includes = [
        "include",
        "src",
    ],
    deps = [
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/gui",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/utl",
        "//third-party/lodepng",
        "@boost.asio",
        "@boost.beast",
        "@boost.geometry",
        "@tcl_lang//:tcl",
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
    includes = [
        "include",
        "src",
    ],
    deps = [
        ":web",
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

js_library(
    name = "js_sources",
    srcs = glob(["src/*.js"]),
    visibility = ["//src/web:__subpackages__"],
)
