load("@aspect_rules_js//js:defs.bzl", "js_test")
load("@npm//:defs.bzl", "npm_link_all_packages")

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2026, The OpenROAD Authors
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_python//python:defs.bzl", "py_test")
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

npm_link_all_packages(name = "node_modules")

TESTS = [
]

filegroup(
    name = "test_resources",
    # overly broad glob, could be refined later, but
    # symlinks are cheap and OpenROAD binary changes, the common
    # use case is that all tests have to be re-run.
    srcs = glob(
        ["**/*"],
        exclude = [
            test + "." + ext
            for test in TESTS
            for ext in [
                "tcl",
                "py",
            ]
        ],
    ),
)

[regression_test(
    name = test_name,
    data = [":test_resources"],
) for test_name in TESTS]

# The report bundler strips module syntax with regexes; this pins the shapes.
py_test(
    name = "embed_report_assets_test",
    srcs = ["embed_report_assets_test.py"],
    data = ["//src/web:src/embed_report_assets.py"],
    legacy_create_init = 0,
)

# The CSP names the inline scripts by hash; a hash over anything but the served
# bytes blocks them with no build-time failure.
py_test(
    name = "embed_web_assets_test",
    srcs = ["embed_web_assets_test.py"],
    data = ["//src/web:src/embed_web_assets.py"],
    legacy_create_init = 0,
)

# The fetch empties --dest and short-circuits on a stamp; both are quiet when
# wrong.  Needs no network.
py_test(
    name = "fetch_packages_test",
    srcs = ["fetch_packages_test.py"],
    data = [
        "//src/web:third-party/fetch_packages.py",
        "//src/web:third-party/packages.json",
    ],
    legacy_create_init = 0,
)

# The asset lists live once per build system; this is what keeps them equal.
py_test(
    name = "asset_list_parity_test",
    srcs = ["asset_list_parity.py"],
    data = [
        "//src/web:BUILD",
        "//src/web:CMakeLists.txt",
        "//src/web:third-party/fetch_packages.py",
        "//src/web:third-party/packages.json",
    ],
    legacy_create_init = 0,
    main = "asset_list_parity.py",
)

JS_FILES = ["//src/web:js_files"]

DOM_TEST_DATA = [
    "js/setup-dom.js",
    ":node_modules/jsdom",
] + JS_FILES

test_suite(
    name = "cpp_tests",
    tests = [
        ":css_inliner_test",
        ":request_handler_test",
        ":tile_generator_test",
        ":web_assets_test",
    ],
)

cc_test(
    name = "css_inliner_test",
    srcs = ["cpp/TestCssInliner.cpp"],
    deps = [
        "//src/gui:gui_stub",
        "//src/utl",
        "//src/web",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "web_assets_test",
    srcs = ["cpp/TestWebAssets.cpp"],
    deps = [
        "//src/gui:gui_stub",
        "//src/web",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

# The bundle is a build output, not a source file, so it is not in JS_FILES and
# needs no no_copy_to_bin.
js_test(
    name = "golden_layout_bundle_test",
    data = ["//src/web:third-party/golden-layout/golden-layout.esm.js"],
    entry_point = "js/test-golden-layout-bundle.js",
)

js_test(
    name = "coordinates_test",
    data = JS_FILES,
    entry_point = "js/test-coordinates.js",
    no_copy_to_bin = JS_FILES,
)

js_test(
    name = "schematic_widget_test",
    data = JS_FILES,
    entry_point = "js/test-schematic-widget.js",
    no_copy_to_bin = JS_FILES,
)

js_test(
    name = "vis_tree_test",
    data = DOM_TEST_DATA,
    entry_point = "js/test-vis-tree.js",
    no_copy_to_bin = JS_FILES,
)

js_test(
    name = "websocket_manager_test",
    data = JS_FILES,
    entry_point = "js/test-websocket-manager.js",
    no_copy_to_bin = JS_FILES,
)

js_test(
    name = "websocket_tile_layer_test",
    data = JS_FILES,
    entry_point = "js/test-websocket-tile-layer.js",
    no_copy_to_bin = JS_FILES,
)

js_test(
    name = "checkbox_tree_model_test",
    data = JS_FILES,
    entry_point = "js/test-checkbox-tree-model.js",
    no_copy_to_bin = JS_FILES,
)

js_test(
    name = "display_controls_test",
    data = DOM_TEST_DATA,
    entry_point = "js/test-display-controls.js",
    no_copy_to_bin = JS_FILES,
)

js_test(
    name = "clock_tree_widget_test",
    data = JS_FILES,
    entry_point = "js/test-clock-tree-widget.js",
    no_copy_to_bin = JS_FILES,
)

js_test(
    name = "charts_widget_test",
    data = DOM_TEST_DATA,
    entry_point = "js/test-charts-widget.js",
    no_copy_to_bin = JS_FILES,
)

js_test(
    name = "timing_widget_test",
    data = DOM_TEST_DATA,
    entry_point = "js/test-timing-widget.js",
    no_copy_to_bin = JS_FILES,
)

js_test(
    name = "inspector_test",
    data = DOM_TEST_DATA,
    entry_point = "js/test-inspector.js",
    no_copy_to_bin = JS_FILES,
)

js_test(
    name = "ruler_test",
    data = JS_FILES,
    entry_point = "js/test-ruler.js",
    no_copy_to_bin = JS_FILES,
)

js_test(
    name = "tcl_completer_test",
    data = DOM_TEST_DATA,
    entry_point = "js/test-tcl-completer.js",
    no_copy_to_bin = JS_FILES,
)

js_test(
    name = "drc_widget_test",
    data = DOM_TEST_DATA,
    entry_point = "js/test-drc-widget.js",
    no_copy_to_bin = JS_FILES,
)

js_test(
    name = "menu_bar_test",
    data = DOM_TEST_DATA,
    entry_point = "js/test-menu-bar.js",
    no_copy_to_bin = JS_FILES,
)

js_test(
    name = "status_indicator_test",
    data = DOM_TEST_DATA,
    entry_point = "js/test-status-indicator.js",
    no_copy_to_bin = JS_FILES,
)

js_test(
    name = "merged_tile_layer_test",
    data = DOM_TEST_DATA,
    entry_point = "js/test-merged-tile-layer.js",
    no_copy_to_bin = JS_FILES,
)

js_test(
    name = "tile_merge_test",
    data = JS_FILES,
    entry_point = "js/test-tile-merge.js",
    no_copy_to_bin = JS_FILES,
)

js_test(
    name = "dpr_test",
    data = JS_FILES,
    entry_point = "js/test-dpr.js",
    no_copy_to_bin = JS_FILES,
)

js_test(
    name = "device_pixels_test",
    data = JS_FILES,
    entry_point = "js/test-device-pixels.js",
    no_copy_to_bin = JS_FILES,
)

js_test(
    name = "theme_test",
    data = DOM_TEST_DATA,
    entry_point = "js/test-theme.js",
    no_copy_to_bin = JS_FILES,
)

js_test(
    name = "title_test",
    data = JS_FILES,
    entry_point = "js/test-title.js",
    no_copy_to_bin = JS_FILES,
)

cc_test(
    name = "clock_tree_report_test",
    srcs = ["cpp/TestClockTreeReport.cpp"],
    data = [
        "//test:nangate45_data",
    ],
    deps = [
        "//src/gui:gui_stub",
        "//src/odb/src/db",
        "//src/tst:nangate45_fixture",
        "//src/web",
        "@boost.json",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "request_handler_test",
    srcs = ["cpp/TestRequestHandler.cpp"],
    data = [
        "//test:nangate45_data",
    ],
    deps = [
        "//src/gui",
        "//src/gui:gui_stub",
        "//src/odb/src/db",
        "//src/tst:nangate45_fixture",
        "//src/web",
        "@boost.json",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "tile_generator_test",
    srcs = ["cpp/TestTileGenerator.cpp"],
    data = [
        "//test:nangate45_data",
    ],
    deps = [
        "//src/gui",
        "//src/gui:gui_stub",
        "//src/odb/src/db",
        "//src/tst:nangate45_fixture",
        "//src/web",
        "//src/web:color",
        "//third-party/lodepng",
        "@boost.json",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "save_image_test",
    srcs = ["cpp/TestSaveImage.cpp"],
    data = [
        "//test:nangate45_data",
    ],
    deps = [
        "//src/gui:gui_stub",
        "//src/odb/src/db",
        "//src/tst:nangate45_fixture",
        "//src/web",
        "//third-party/lodepng",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "snap_test",
    srcs = ["cpp/TestSnap.cpp"],
    data = [
        "//test:nangate45_data",
    ],
    deps = [
        "//src/gui:gui_stub",
        "//src/odb/src/db",
        "//src/tst:nangate45_fixture",
        "//src/web",
        "@boost.json",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "glyph_cache_test",
    srcs = ["cpp/TestGlyphCache.cpp"],
    deps = [
        "//src/gui:gui_stub",
        "//src/web",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "debug_graphics_test",
    srcs = ["cpp/TestDebugGraphics.cpp"],
    deps = [
        "//src/gui",
        "//src/gui:gui_stub",
        "//src/odb/src/db",
        "//src/web",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "save_report_test",
    srcs = ["cpp/TestSaveReport.cpp"],
    data = [
        "//test:nangate45_data",
    ],
    deps = [
        "//src/gui:gui_stub",
        "//src/odb/src/db",
        "//src/tst:nangate45_fixture",
        "//src/utl",
        "//src/web",
        "@boost.json",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
