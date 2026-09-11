# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_python//python:defs.bzl", "py_binary", "py_test")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")
load("//test:regression.bzl", "messages_txt")

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

# The vendored libraries are generated files (see third-party/README.md); this
# is what catches one being edited in place instead of through the script.
py_test(
    name = "vendor_lock_test",
    srcs = ["third-party/update_vendor.py"],
    args = ["--check"],
    data = glob(
        ["third-party/**"],
        exclude = ["third-party/update_vendor.py"],
    ),
    legacy_create_init = 0,
    main = "third-party/update_vendor.py",
)

# The report inlines these into one <script type="module">, so the order is
# what runs.  Listed once and used for both srcs and the command line.
_REPORT_JS_FILES = [
    "src/theme.js",
    "src/display-state.js",
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
    "src/label-manager.js",
    "src/tcl-completer.js",
    "src/hierarchy-browser.js",
    "src/menu-bar.js",
    "src/context-menu.js",
    "src/search-nav.js",
    "src/capture.js",
    "src/toolbar.js",
    "src/image-export.js",
    "src/edit-dialogs.js",
    "src/title.js",
    "src/clock-tree-widget.js",
    "src/schematic-widget.js",
    "src/charts-widget.js",
    "src/timing-widget.js",
    "src/drc-widget.js",
    "src/selection-browser.js",
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
    "src/display-state.js",
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
    "src/label-manager.js",
    "src/tcl-completer.js",
    "src/hierarchy-browser.js",
    "src/menu-bar.js",
    "src/context-menu.js",
    "src/search-nav.js",
    "src/capture.js",
    "src/toolbar.js",
    "src/image-export.js",
    "src/edit-dialogs.js",
    "src/title.js",
    "src/clock-tree-widget.js",
    "src/schematic-widget.js",
    "src/openroad_skin.svg",
    "src/charts-widget.js",
    "src/timing-widget.js",
    "src/drc-widget.js",
    "src/selection-browser.js",
    "src/main.js",
    # The vendored browser libraries.  They are served from the binary like any
    # other asset so the viewer never loads code from a CDN (issue #11065); see
    # third-party/README.md.  Their directory layout is part of the contract:
    # the stylesheets reach their icons through relative urls.
    "third-party/leaflet/leaflet.js",
    "third-party/leaflet/leaflet.css",
    "third-party/leaflet/images/layers.png",
    "third-party/leaflet/images/layers-2x.png",
    "third-party/leaflet/images/marker-icon.png",
    "third-party/leaflet/images/marker-icon-2x.png",
    "third-party/leaflet/images/marker-shadow.png",
    "third-party/golden-layout/golden-layout.esm.js",
    "third-party/golden-layout/css/goldenlayout-base.css",
    "third-party/golden-layout/css/themes/goldenlayout-dark-theme.css",
    "third-party/golden-layout/css/themes/goldenlayout-light-theme.css",
    "third-party/golden-layout/img/lm_close_black.png",
    "third-party/golden-layout/img/lm_close_tab_white.png",
    "third-party/golden-layout/img/lm_close_white.png",
    "third-party/golden-layout/img/lm_maximise_black.png",
    "third-party/golden-layout/img/lm_maximise_white.png",
    "third-party/golden-layout/img/lm_minimize_black.png",
    "third-party/golden-layout/img/lm_minimize_white.png",
    "third-party/golden-layout/img/lm_popin_black.png",
    "third-party/golden-layout/img/lm_popin_white.png",
    "third-party/golden-layout/img/lm_popout_black.png",
    "third-party/golden-layout/img/lm_popout_white.png",
    "third-party/three/three.module.min.js",
    "third-party/elkjs/elk.bundled.js",
    "third-party/netlistsvg/netlistsvg.bundle.js",
]

# Assets under src/ are served flat ("/main.js"); everything else keeps its
# path ("/third-party/leaflet/images/layers.png").
_WEB_ASSET_ARGS = [
    "/%s=$(location %s)" % (
        f[len("src/"):] if f.startswith("src/") else f,
        f,
    )
    for f in _WEB_ASSET_FILES
]

genrule(
    name = "web_assets",
    srcs = _WEB_ASSET_FILES,
    outs = ["src/web_assets.cpp"],
    cmd = "$(execpath :embed_web_assets) --output $@ " +
          " ".join(_WEB_ASSET_ARGS),
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
        "src/web_gif.cpp",
        "src/web_gif.h",
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
        "//third-party/gif-h:gif_h",
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

exports_files(["README.md"])

messages_txt(
    visibility = ["//docs:__pkg__"],
)
