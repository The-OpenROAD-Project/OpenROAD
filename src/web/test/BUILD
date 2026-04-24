load("@aspect_rules_js//js:defs.bzl", "js_test")
load("@npm//:defs.bzl", "npm_link_all_packages")

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2026, The OpenROAD Authors
load("@rules_cc//cc:cc_test.bzl", "cc_test")
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

JS_FILES = ["//src/web:js_files"]

DOM_TEST_DATA = [
    "js/setup-dom.js",
    ":node_modules/jsdom",
] + JS_FILES

js_test(
    name = "coordinates_test",
    data = JS_FILES,
    entry_point = "js/test-coordinates.js",
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
    name = "checkbox_tree_model_test",
    data = JS_FILES,
    entry_point = "js/test-checkbox-tree-model.js",
    no_copy_to_bin = JS_FILES,
)

js_test(
    name = "display_controls_test",
    data = JS_FILES,
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
    name = "status_indicator_test",
    data = DOM_TEST_DATA,
    entry_point = "js/test-status-indicator.js",
    no_copy_to_bin = JS_FILES,
)

cc_test(
    name = "clock_tree_report_test",
    srcs = ["cpp/TestClockTreeReport.cpp"],
    copts = ["-Isrc/web/src"],
    data = [
        "//test:nangate45_data",
    ],
    features = ["-layering_check"],
    deps = [
        "//src/gui",
        "//src/odb",
        "//src/tst",
        "//src/tst:nangate45_fixture",
        "//src/utl",
        "//src/web",
        "//third-party/lodepng",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "request_handler_test",
    srcs = ["cpp/TestRequestHandler.cpp"],
    copts = ["-Isrc/web/src"],
    data = [
        "//test:nangate45_data",
    ],
    features = ["-layering_check"],
    deps = [
        "//src/gui",
        "//src/odb",
        "//src/tst",
        "//src/tst:nangate45_fixture",
        "//src/utl",
        "//src/web",
        "//third-party/lodepng",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "tile_generator_test",
    srcs = ["cpp/TestTileGenerator.cpp"],
    copts = ["-Isrc/web/src"],
    data = [
        "//test:nangate45_data",
    ],
    features = ["-layering_check"],
    deps = [
        "//src/gui",
        "//src/odb",
        "//src/tst",
        "//src/tst:nangate45_fixture",
        "//src/utl",
        "//src/web",
        "//third-party/lodepng",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "save_image_test",
    srcs = ["cpp/TestSaveImage.cpp"],
    copts = ["-Isrc/web/src"],
    data = [
        "//test:nangate45_data",
    ],
    features = ["-layering_check"],
    deps = [
        "//src/gui",
        "//src/odb",
        "//src/tst",
        "//src/tst:nangate45_fixture",
        "//src/utl",
        "//src/web",
        "//third-party/lodepng",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "snap_test",
    srcs = ["cpp/TestSnap.cpp"],
    copts = ["-Isrc/web/src"],
    data = [
        "//test:nangate45_data",
    ],
    features = ["-layering_check"],
    deps = [
        "//src/gui",
        "//src/odb",
        "//src/tst",
        "//src/tst:nangate45_fixture",
        "//src/utl",
        "//src/web",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "debug_graphics_test",
    srcs = ["cpp/TestDebugGraphics.cpp"],
    copts = ["-Isrc/web/src"],
    features = ["-layering_check"],
    deps = [
        "//src/gui",
        "//src/odb",
        "//src/web",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "save_report_test",
    srcs = ["cpp/TestSaveReport.cpp"],
    copts = ["-Isrc/web/src"],
    data = [
        "//test:nangate45_data",
    ],
    features = ["-layering_check"],
    deps = [
        "//src/gui",
        "//src/odb",
        "//src/tst",
        "//src/tst:nangate45_fixture",
        "//src/utl",
        "//src/web",
        "//third-party/lodepng",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
