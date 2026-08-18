# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_python//python:defs.bzl", "py_binary")
load("@web_third_party//:assets.bzl", "WEB_THIRD_PARTY_ASSETS", "WEB_THIRD_PARTY_BUNDLES")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

# Read by //src/web/test:asset_list_parity_test, which holds the two build
# systems' asset lists to each other, and by the test of the report bundler.
exports_files([
    "BUILD",
    "CMakeLists.txt",
    "src/embed_report_assets.py",
    "third-party/fetch_packages.py",
    "third-party/packages.json",
])

py_binary(
    name = "embed_report_assets",
    srcs = ["src/embed_report_assets.py"],
    main = "src/embed_report_assets.py",
)

py_binary(
    name = "embed_web_assets",
    srcs = ["src/embed_web_assets.py"],
    main = "src/embed_web_assets.py",
)

# The CMake build fetches with this; here it is only the esbuild invocation, so
# that both build systems bundle golden-layout the same way down to the bytes.
py_binary(
    name = "fetch_packages",
    srcs = ["third-party/fetch_packages.py"],
    data = ["third-party/packages.json"],
    main = "third-party/fetch_packages.py",
)

# Which prebuilt esbuild the bundle genrule runs.  A host outside this list has
# no pinned binary, and adding one means an entry here, in packages.json and in
# fetch_packages.py's ESBUILD_PLATFORMS.
config_setting(
    name = "linux_x64",
    constraint_values = [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
    ],
)

config_setting(
    name = "linux_arm64",
    constraint_values = [
        "@platforms//cpu:aarch64",
        "@platforms//os:linux",
    ],
)

config_setting(
    name = "darwin_x64",
    constraint_values = [
        "@platforms//cpu:x86_64",
        "@platforms//os:macos",
    ],
)

config_setting(
    name = "darwin_arm64",
    constraint_values = [
        "@platforms//cpu:arm64",
        "@platforms//os:macos",
    ],
)

alias(
    name = "esbuild",
    actual = select({
        ":darwin_arm64": "@esbuild_darwin_arm64//:bin/esbuild",
        ":darwin_x64": "@esbuild_darwin_x64//:bin/esbuild",
        ":linux_arm64": "@esbuild_linux_arm64//:bin/esbuild",
        ":linux_x64": "@esbuild_linux_x64//:bin/esbuild",
    }),
)

# golden-layout publishes no browser build, and the saved report needs one file:
# a relative import does not resolve from a data: URI.
[genrule(
    name = "bundle_" + bundle.package.replace("-", "_"),
    srcs = [
        bundle.tree,
        bundle.entry,
    ],
    outs = [bundle.out],
    cmd = "$(execpath :fetch_packages)" +
          " --bundle " + bundle.package +
          " --esbuild $(execpath :esbuild)" +
          " --entry $(execpath " + bundle.entry + ")" +
          " --output $@",
    tools = [
        ":esbuild",
        ":fetch_packages",
    ],
) for bundle in WEB_THIRD_PARTY_BUNDLES]

# The report inlines these into one <script type="module">, so the order is
# what runs.  Listed once and used for both srcs and the command line.
_REPORT_JS_FILES = [
    "src/theme.js",
    "src/3d-viewer-widget.js",
    "src/coordinates.js",
    "src/ui-utils.js",
    "src/checkbox-tree-model.js",
    "src/vis-tree.js",
    "src/websocket-manager.js",
    "src/tile-merge.js",
    "src/tile-request.js",
    "src/device-pixels.js",
    "src/websocket-tile-layer.js",
    "src/merged-tile-layer.js",
    "src/display-controls.js",
    "src/inspector.js",
    "src/ruler.js",
    "src/tcl-completer.js",
    "src/hierarchy-browser.js",
    "src/menu-bar.js",
    "src/title.js",
    "src/clock-tree-widget.js",
    "src/schematic-widget.js",
    "src/charts-widget.js",
    "src/timing-widget.js",
    "src/drc-widget.js",
    "src/main.js",
]

genrule(
    name = "report_assets",
    srcs = ["src/style.css"] + _REPORT_JS_FILES,
    outs = ["src/report_assets.cpp"],
    cmd = "$(execpath :embed_report_assets)" +
          " --output $@" +
          " --css $(location src/style.css)" +
          " --js" +
          "".join([" $(location %s)" % f for f in _REPORT_JS_FILES]),
    tools = [":embed_report_assets"],
)

_WEB_ASSET_FILES = [
    "src/index.html",
    "src/style.css",
    "src/theme.js",
    "src/tile-merge.js",
    "src/tile-request.js",
    "src/device-pixels.js",
    "src/merged-tile-layer.js",
    "src/3d-viewer-widget.js",
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
    "src/title.js",
    "src/clock-tree-widget.js",
    "src/schematic-widget.js",
    "src/openroad_skin.svg",
    "src/charts-widget.js",
    "src/timing-widget.js",
    "src/drc-widget.js",
    "src/main.js",
]

# (served path, the file to serve there).  The viewer's own assets are served
# flat ("/main.js").  The browser libraries come from the manifest, keeping the
# layout their packages ship, because the stylesheets reach their icons through
# relative urls; the bundled ones are outputs of the genrules above.
_ASSETS = [
    (
        "/" + f[len("src/"):],
        f,
    )
    for f in _WEB_ASSET_FILES
] + WEB_THIRD_PARTY_ASSETS + [
    (
        bundle.served,
        bundle.out,
    )
    for bundle in WEB_THIRD_PARTY_BUNDLES
]

genrule(
    name = "web_assets",
    srcs = [target for _, target in _ASSETS],
    outs = ["src/web_assets.cpp"],
    cmd = "$(execpath :embed_web_assets) --output $@ " +
          " ".join([
              "%s=$(location %s)" % (served, target)
              for served, target in _ASSETS
          ]),
    tools = [":embed_web_assets"],
)

cc_library(
    name = "color",
    srcs = ["src/color.cpp"],
    hdrs = ["src/color.h"],
    includes = ["src"],
    visibility = ["//src/web/test:__pkg__"],
)

cc_library(
    name = "web",
    srcs = [
        "src/clock_tree_report.cpp",
        "src/font_atlas.h",
        "src/font_data.cpp",
        "src/font_data.h",
        "src/glyph_cache.cpp",
        "src/hierarchy_report.cpp",
        "src/hierarchy_report.h",
        "src/module_color_palette.h",
        "src/request_dispatcher.h",
        "src/request_handler.cpp",
        "src/search.cpp",
        "src/search.h",
        "src/tile_generator.cpp",
        "src/timing_report.cpp",
        "src/web.cpp",
        "src/web_chart.cpp",
        "src/web_painter.cpp",
        "src/web_serve.cpp",
        "src/web_viewer_hook.cpp",
        ":report_assets",
        ":web_assets",
    ],
    hdrs = [
        "include/web/web.h",
        # The following are used by tests, but they are not broken
        # out as library. So TODO: make them visible for the tests
        # to properly depend on this library and see the includes.
        # If these are meant to be used in tests with specific dependencies,
        # these should be put (together with their *.cpp files) into separate
        # libraries, with visibility for the //src/web/test path (see :color).
        "src/glyph_cache.h",
        "src/tile_generator.h",
        "src/timing_report.h",
        "src/web_chart.h",
        "src/web_painter.h",
        "src/web_viewer_hook.h",
        "src/request_handler.h",
        "src/clock_tree_report.h",
        "src/css_inliner.h",
        "src/web_assets.h",
    ],
    copts = [
        "-DBOOST_ASIO_NO_DEPRECATED",
        "-DBOOST_JSON_HEADER_ONLY",
    ],
    includes = [
        "include",
        "src",
    ],
    deps = [
        ":color",
        "//:cli_completer",
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/gui",
        "//src/odb/src/db",
        "//src/sta:opensta_lib",
        "//src/utl",
        "//third-party/lodepng",
        "//third-party/stb_truetype",
        "@boost.asio",
        "@boost.beast",
        "@boost.geometry",
        "@boost.json",
        "@boost.system",
        "@spdlog",
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
