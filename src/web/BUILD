# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_python//python:defs.bzl", "py_binary")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

py_binary(
    name = "embed_report_assets",
    srcs = ["src/embed_report_assets.py"],
    main = "src/embed_report_assets.py",
)

genrule(
    name = "report_assets",
    srcs = [
        "src/style.css",
        "src/theme.js",
        "src/coordinates.js",
        "src/ui-utils.js",
        "src/checkbox-tree-model.js",
        "src/vis-tree.js",
        "src/websocket-manager.js",
        "src/websocket-tile-layer.js",
        "src/display-controls.js",
        "src/inspector.js",
        "src/ruler.js",
        "src/tcl-completer.js",
        "src/hierarchy-browser.js",
        "src/menu-bar.js",
        "src/clock-tree-widget.js",
        "src/schematic-widget.js",
        "src/charts-widget.js",
        "src/timing-widget.js",
        "src/main.js",
    ],
    outs = ["src/report_assets.cpp"],
    cmd = "$(execpath :embed_report_assets)" +
          " --output $@" +
          " --css $(location src/style.css)" +
          " --js $(location src/theme.js)" +
          " $(location src/coordinates.js)" +
          " $(location src/ui-utils.js)" +
          " $(location src/checkbox-tree-model.js)" +
          " $(location src/vis-tree.js)" +
          " $(location src/websocket-manager.js)" +
          " $(location src/websocket-tile-layer.js)" +
          " $(location src/display-controls.js)" +
          " $(location src/inspector.js)" +
          " $(location src/ruler.js)" +
          " $(location src/tcl-completer.js)" +
          " $(location src/hierarchy-browser.js)" +
          " $(location src/menu-bar.js)" +
          " $(location src/clock-tree-widget.js)" +
          " $(location src/schematic-widget.js)" +
          " $(location src/charts-widget.js)" +
          " $(location src/timing-widget.js)" +
          " $(location src/main.js)",
    tools = [":embed_report_assets"],
)

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
        ":report_assets",
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

filegroup(
    name = "js_files",
    srcs = glob(["src/*.js"]),
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
