# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

# This package loads @qt-bazel and defines Qt-specific GUI targets.
# By keeping these in a separate Bazel package from //src/gui, the
# @qt-bazel dependency is only fetched and analyzed when this package
# is actually depended upon (i.e. --//:platform=gui builds only).

load("@qt-bazel//:build_defs.bzl", "qt6_library", "qt6_resource_library")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

# A Qt 'headless' build is added for environments that require GUI features
# but do not have an attached display. This requires setting 'qt_gui_platform'
# to `headless`, and substituting the dependency in the top-level BUILD.

QT_GUI_PLATFORMS = {
    "headless": "_headless",
    "native": "",
}

[
    qt6_library(
        name = "gui_qt" + suffix,
        srcs = glob(["src/*.cpp"]) + [
            "//src/gui:gui_swig",
            "//src/gui:tcl",
        ],
        hdrs = [
            "src/colorGenerator.h",
            "src/gui_utils.h",
            "src/heatMapGui.h",
            "src/label.h",
            "src/options.h",
            "src/painter.h",
            "src/staGuiInterface.h",
            "//src/gui:include/gui/MakeGui.h",
            "//src/gui:include/gui/descriptor_registry.h",
            "//src/gui:include/gui/gui.h",
            "//src/gui:include/gui/heatMap.h",
        ],
        defines = select({
            "@platforms//os:linux": ["STATIC_QPA_PLUGIN_XCB=1"],
            "//conditions:default": [],
        }),
        includes = ["src"],
        moc_hdrs = [
            "src/GUIProgress.h",
            "src/browserWidget.h",
            "src/chartsWidget.h",
            "src/chiplet3DWidget.h",
            "src/clockWidget.h",
            "src/cmdInputWidget.h",
            "src/displayControls.h",
            "src/drcWidget.h",
            "src/dropdownCheckboxes.h",
            "src/findDialog.h",
            "src/globalConnectDialog.h",
            "src/gotoDialog.h",
            "src/heatMapSetup.h",
            "src/helpWidget.h",
            "src/highlightGroupDialog.h",
            "src/insertBufferDialog.h",
            "src/inspector.h",
            "src/layoutTabs.h",
            "src/layoutViewer.h",
            "src/mainWindow.h",
            "src/renderThread.h",
            "src/scriptWidget.h",
            "src/search.h",
            "src/selectHighlightWindow.h",
            "src/staGui.h",
            "src/tclCmdHighlighter.h",
            "src/tclCmdInputWidget.h",
            "src/timingWidget.h",
        ],
        qt_gui_platform = platform,
        uic_srcs = [
            "ui/findDlg.ui",
            "ui/gotoDlg.ui",
            "ui/highlightGroupDlg.ui",
            "ui/selectedWidget.ui",
        ] if platform == "native" else [],
        deps = [
            ":qt_resources",
            "//:ord",
            "//src/dbSta",
            "//src/dbSta:dbNetwork",
            "//src/gui:gui_descriptors",
            "//src/gui:gui_heatmap_core",
            "//src/odb",
            "//src/sta:opensta_lib",
            "//src/utl",
            "//third-party/gif-h:gif_h",
            "@abseil-cpp//absl/synchronization",
            "@boost.algorithm",
            "@boost.geometry",
            "@boost.multi_array",
            "@boost.stacktrace",
            "@spdlog",
            "@tcl_lang//:tcl",
        ] + [":uic_lib"],
    )
    for platform, suffix in QT_GUI_PLATFORMS.items()
]

# This is used to work around the limitation of the `qt6_library` rule which
# only supports use of one `qt6_library` rule using `uic_srcs` per package.
cc_library(
    name = "uic_lib",
    hdrs = [
        ":ui_findDlg.h",
        ":ui_gotoDlg.h",
        ":ui_highlightGroupDlg.h",
        ":ui_selectedWidget.h",
    ],
)

qt6_resource_library(
    name = "qt_resources",
    data_srcs = glob([
        "resources/*.png",
        "resources/google_icons/*.png",
    ]),
    external_init_name = "resource",
    qrc_srcs = ["resources/resource.qrc"],
)
