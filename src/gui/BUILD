# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

# This file only currently supports the gui disabled.  More work
# is needed to handle Qt and build a working gui.

load("@bazel_skylib//rules:expand_template.bzl", "expand_template")
load("@qt-bazel//:build_defs.bzl", "qt6_library", "qt6_resource_library")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_pkg//pkg:mappings.bzl", "pkg_files")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")
load("//test:regression.bzl", "messages_txt")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

# The gui as a binary with no Qt has it: the Qt-free core, plus the entry
# points answering that there is no gui to open.  A Qt build links :gui_qt
# instead, which carries Qt's own versions of those, so nothing reachable
# from :openroad-qt depends on this target.
cc_library(
    name = "gui",
    srcs = [
        "src/makeGui.cpp",
    ],
    hdrs = [
        "include/gui/MakeGui.h",
        "include/gui/gui.h",
    ],
    includes = [
        "include",
        "src",
    ],
    deps = [
        "//src/odb/src/db",
        "//src/web:core",
        "@tcl_lang//:tcl",
    ],
)

# A Qt 'headless' build is added for environments that require GUI features
# but do not have an attached display. This requires setting 'qt_gui_platform'
# to `headless`, and subsituting the dependency in the top-level BUILD.
QT_GUI_PLATFORMS = {
    "headless": "_headless",
    "native": "",
}

[
    qt6_library(
        name = "gui_qt" + suffix,
        srcs = glob(
            include = [
                "src/*.cpp",
            ],
            # makeGui.cpp answers the entry points for a build with no Qt;
            # this target has gui.cpp's versions of them.
            exclude = [
                "src/makeGui.cpp",
            ],
        ) + [
            ":gui_swig",
            ":tcl",
        ],
        hdrs = [
            "include/gui/MakeGui.h",
            "include/gui/gui.h",
        ],
        defines = select({
            "@platforms//os:linux": ["STATIC_QPA_PLUGIN_XCB=1"],
            "//conditions:default": [],
        }),
        includes = [
            "include",
            "src",
            "ui",
        ],
        moc_hdrs = glob(["src/*.h"]),
        qt_gui_platform = platform,
        uic_srcs = glob(["ui/*.ui"]) if platform == "native" else [],
        deps = [
            ":qt_resources",
            "//:ord",
            "//src/dbSta",
            "//src/dbSta:dbNetwork",
            "//src/odb/src/db",
            "//src/odb/src/defout",
            "//src/sta:opensta_lib",
            "//src/utl",
            "//src/web:core",
            "@abseil-cpp//absl/synchronization",
            "@boost.algorithm",
            "@boost.geometry",
            "@boost.multi_array",
            "@boost.stacktrace",
            "@spdlog",
            "@tcl_lang//:tcl",
        ] + ([":uic_lib"] if platform == "headless" else []),
    )
    for platform, suffix in QT_GUI_PLATFORMS.items()
]

# This is used to work around the limitation of the `qt6_library` rule which
# only supports use of one `qt6_library` rule using `uic_srcs` per package.
cc_library(
    name = "uic_lib",
    hdrs = [
        ":ui/ui_findDlg.h",
        ":ui/ui_gotoDlg.h",
        ":ui/ui_highlightGroupDlg.h",
        ":ui/ui_selectedWidget.h",
    ],
    includes = [
        "ui",
    ],
    deps = [
        "@qt-bazel//qt_source:core",
        "@qt-bazel//qt_source:widgets",
    ],
)

qt6_resource_library(
    name = "qt_resources",
    data_srcs = glob([
        "resources/**/*.png",
    ]),
    external_init_name = "resource",
    qrc_srcs = [
        "resources/resource.qrc",
    ],
)

tcl_wrap_cc(
    name = "gui_swig",
    srcs = [
        "src/gui.i",
        "//:error_swig",
    ],
    module = "gui",
    namespace_prefix = "gui",
    root_swig_src = "src/gui.i",
    runtime_header = "src/tclSwig.h",
    swig_includes = [
        "src",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/gui.tcl",
    ],
    char_array_name = "gui_tcl_inits",
    namespace = "gui",
)

filegroup(
    name = "doc_files",
    srcs = [
        "README.md",
        "src/gui.tcl",
    ],
    visibility = ["//src/gui/test:__pkg__"],
)

messages_txt(
    visibility = [
        "//docs:__pkg__",
        "//src/gui/test:__pkg__",
    ],
)

# Desktop entry for GUI builds. Generated from the same template that the
# CMake build configures (see src/gui/CMakeLists.txt). The install prefix is
# not known at build time, so the icon path is left as a @OPENROAD_PREFIX@
# placeholder that //bazel:install.sh substitutes at install time.
expand_template(
    name = "desktop_entry",
    out = "openroad.desktop",
    substitutions = {
        "${OPENROAD_SHARE_GUI_DIR}": "@OPENROAD_PREFIX@/share/openroad/gui",
    },
    template = "resources/openroad.desktop.cmake",
)

# The desktop entry and its icon, laid out at the paths install.sh expects
# inside the extracted tar. Pulled into //packaging:tarfile only when
# --//:platform=gui is set.
pkg_files(
    name = "desktop_files",
    srcs = [
        "resources/icon.png",
        ":openroad.desktop",
    ],
    renames = {
        ":openroad.desktop": "share/applications/openroad.desktop",
        "resources/icon.png": "share/openroad/gui/icon.png",
    },
    visibility = ["//packaging:__pkg__"],
)
