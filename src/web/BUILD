# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@aspect_rules_esbuild//esbuild:defs.bzl", "esbuild")
load("@aspect_rules_js//js:defs.bzl", "js_library")
load("@npm_web//:defs.bzl", "npm_link_all_packages")
load("@npm_web//src/web:html-minifier-terser/package_json.bzl", html_minifier = "bin")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_python//python:defs.bzl", "py_binary")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")
load("//test:regression.bzl", "messages_txt")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

py_binary(
    name = "embed_web_assets",
    srcs = ["src/embed_web_assets.py"],
    main = "src/embed_web_assets.py",
)

py_binary(
    name = "inline_styles",
    srcs = ["src/inline_styles.py"],
    main = "src/inline_styles.py",
)

py_binary(
    name = "collect_licenses",
    srcs = ["src/collect_licenses.py"],
    main = "src/collect_licenses.py",
)

# ── The browser bundles ─────────────────────────────────────────────────────
#
# The viewer's npm libraries, bundled and minified for the embed rules below.
# The CMake build cannot run the bundler and embeds dist/ (see dist/README.md).

npm_link_all_packages(name = "node_modules")

# What the bundles carry, and so whose licences ship with them.
_BUNDLED_PACKAGES = [
    "elkjs",
    "golden-layout",
    "leaflet",
    "netlistsvg",
    "three",
]

_APP_SRCS = glob([
    "src/*.js",
    "src/*.css",
]) + [
    "esbuild.config.mjs",
    "src/openroad_skin.svg",
]

js_library(
    name = "app_js",
    srcs = _APP_SRCS,
    deps = [":node_modules/" + p for p in _BUNDLED_PACKAGES],
)

genrule(
    name = "third_party_licenses",
    srcs = [":node_modules/%s/dir" % p for p in _BUNDLED_PACKAGES] + [
        "licenses/netlistsvg-bundle.txt",
    ],
    outs = ["THIRD_PARTY_LICENSES.txt"],
    cmd = "$(execpath :collect_licenses) --output $@" +
          " --extra $(location licenses/netlistsvg-bundle.txt) " +
          " ".join(["$(execpath :node_modules/%s/dir)" % p for p in _BUNDLED_PACKAGES]),
    tools = [":collect_licenses"],
)

[esbuild(
    name = name,
    config = "esbuild.config.mjs",
    entry_point = "src/%s" % entry,
    minify = True,
    output = output,
    # Off, not the rule's "linked": no .map is served, so its comment would 404.
    # esbuild.config.mjs cannot set this key.
    sourcemap = "",
    deps = [":app_js"],
) for name, entry, output in [
    ("app_bundle", "entry-app.js", "app.min.js"),
    # Without elk, netlistsvg and three, whose panels need a live server.
    ("report_bundle", "entry-report.js", "report.min.js"),
    # index.html orders the four stylesheets around the two themes, and the
    # cascade depends on that order (see src/vendor.css).
    ("vendor_css", "vendor.css", "vendor.min.css"),
    ("app_css", "style.css", "app.min.css"),
    ("gl_theme_dark_bundle", "gl-theme-dark.css", "gl-dark.min.css"),
    ("gl_theme_light_bundle", "gl-theme-light.css", "gl-light.min.css"),
]]

# Minified while still a source file; the stylesheets, which esbuild has
# minified already, are stitched in afterwards.
html_minifier.html_minifier_terser(
    name = "minify_html",
    srcs = ["src/index.html"],
    outs = ["index.min.html.in"],
    args = [
        "src/index.html",
        "--collapse-whitespace",
        "--remove-comments",
        # index.html keeps one inline script, the line that disables the light
        # theme before theme.js runs; everything else is in the bundle.
        "--minify-js",
        "-o",
        "index.min.html.in",
    ],
    # js_run_binary then runs inside the package's bindir, so the paths above
    # are package-relative.
    chdir = package_name(),
)

genrule(
    name = "index_html",
    srcs = [
        ":minify_html",
        "vendor.min.css",
        "gl-dark.min.css",
        "gl-light.min.css",
        "app.min.css",
    ],
    outs = ["index.min.html"],
    cmd = "$(execpath :inline_styles) --html $(location :minify_html)" +
          " --output $@" +
          " --style $(location vendor.min.css)" +
          " --style $(location gl-dark.min.css)" +
          " --style $(location gl-light.min.css)" +
          " --style $(location app.min.css)",
    tools = [":inline_styles"],
)

# The two blobs the server hands out, their licences, and the saved report's
# script, gzipped by the script itself so that no build needs a host gzip.
genrule(
    name = "web_assets",
    srcs = [
        "THIRD_PARTY_LICENSES.txt",
        "app.min.js",
        "index.min.html",
        "report.min.js",
    ],
    outs = ["src/web_assets.cpp"],
    cmd = "$(execpath :embed_web_assets) --output $@" +
          " --extern kReportJS=$(location report.min.js)" +
          " /index.html=$(location index.min.html)" +
          " /app.min.js=$(location app.min.js)" +
          " /THIRD_PARTY_LICENSES.txt=$(location THIRD_PARTY_LICENSES.txt)",
    tools = [":embed_web_assets"],
)

# The Qt-free half of what used to be the gui: the data model every other
# module draws through -- Painter, Renderer, Descriptor/Selected,
# HeatMapDataSource -- published through gui/core.h.  It lives here because
# the web viewer is its main consumer and nothing in it needs Qt; the Qt gui
# links it too, so a binary carries one copy either way.
#
# Still the gui:: namespace and the gui/ include prefix.  Renaming those is a
# much wider change and is not part of moving the code.
cc_library(
    name = "core",
    srcs = [
        "src/bufferTreeDescriptor.cpp",
        "src/colorGenerator.cpp",
        "src/dbDescriptors.cpp",
        "src/descriptor_registry.cpp",
        "src/gifWriter.cpp",
        "src/guiDispatch.cpp",
        "src/heatMapCore.cpp",
        "src/heatMapPinDensity.cpp",
        "src/heatMapPinDensity.h",
        "src/heatMapPlacementDensity.cpp",
        "src/heatMapPlacementDensity.h",
        "src/heatMapRenderer.h",
        "src/init_descriptors.cpp",
        "src/renderer.cpp",
        "src/staDescriptors.cpp",
    ],
    hdrs = [
        "include/web/core.h",
        "include/web/descriptor_registry.h",
        "include/web/heatMap.h",
        # The Qt gui's widgets build on these: its windows and views name
        # the descriptors, and its layout tabs take colours from the same
        # palette.  Private while this code and the gui shared a package;
        # part of the interface now that they do not.
        "src/bufferTreeDescriptor.h",
        "src/colorGenerator.h",
        "src/dbDescriptors.h",
        "src/staDescriptors.h",
    ],
    includes = [
        "include",
        "src",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/odb/src/db",
        "//src/sta:opensta_lib",
        "//src/utl",
        "//third-party/gif-h:gif_h",
        "@abseil-cpp//absl/synchronization",
        "@boost.algorithm",
        "@boost.multi_array",
    ],
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
        "src/asset_gzip.cpp",
        "src/clock_tree_report.cpp",
        "src/font_atlas.h",
        "src/font_data.cpp",
        "src/font_data.h",
        "src/glyph_cache.cpp",
        "src/hierarchy_report.cpp",
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
        "src/hierarchy_report.h",
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
    local_defines = ["HAVE_LIBDEFLATE"],
    deps = [
        ":color",
        ":core",
        "//:cli_completer",
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
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
        "@libdeflate//:deflate",
        "@spdlog",
        "@tcl_lang//:tcl",
        "@zlib",
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

exports_files(
    [
        "package.json",
        "pnpm-lock.yaml",
        "src/openroad_skin.svg",
    ],
    visibility = ["//src/web/test:__pkg__"],
)

messages_txt(
    visibility = ["//docs:__pkg__"],
)
