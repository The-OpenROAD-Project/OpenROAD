# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2026, The OpenROAD Authors
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@aspect_rules_js//js:defs.bzl", "js_test")
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

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

js_test(
    name = "coordinates_test",
    entry_point = "js/test-coordinates.js",
    data = ["//src/web:js_sources"],
    no_copy_to_bin = ["//src/web:js_sources"],
)

js_test(
    name = "vis_tree_test",
    entry_point = "js/test-vis-tree.js",
    data = [
        "//src/web:js_sources",
        "js/setup-dom.js",
        "//src/web:node_modules/jsdom",
    ],
    no_copy_to_bin = ["//src/web:js_sources"],
)

js_test(
    name = "websocket_manager_test",
    entry_point = "js/test-websocket-manager.js",
    data = ["//src/web:js_sources"],
    no_copy_to_bin = ["//src/web:js_sources"],
)

js_test(
    name = "display_controls_test",
    entry_point = "js/test-display-controls.js",
    data = ["//src/web:js_sources"],
    no_copy_to_bin = ["//src/web:js_sources"],
)

js_test(
    name = "clock_tree_widget_test",
    entry_point = "js/test-clock-tree-widget.js",
    data = ["//src/web:js_sources"],
    no_copy_to_bin = ["//src/web:js_sources"],
)

cc_test(
    name = "clock_tree_report_test",
    srcs = ["cpp/TestClockTreeReport.cpp"],
    features = ["-layering_check"],
    copts = ["-Isrc/web/src"],
    data = [
        "//test:nangate45_data",
    ],
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
    features = ["-layering_check"],
    copts = ["-Isrc/web/src"],
    data = [
        "//test:nangate45_data",
    ],
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
    features = ["-layering_check"],
    copts = ["-Isrc/web/src"],
    data = [
        "//test:nangate45_data",
    ],
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
