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

# The .cpp sources that make up :core.  Named so :gui_qt can exclude them
# from its glob; the matching headers are listed in :core's srcs.
CORE_SRCS = [
    "src/bufferTreeDescriptor.cpp",
    "src/colorGenerator.cpp",
    "src/dbDescriptors.cpp",
    "src/descriptor_registry.cpp",
    "src/heatMapCore.cpp",
    "src/heatMapPinDensity.cpp",
    "src/heatMapPlacementDensity.cpp",
    "src/init_descriptors.cpp",
    "src/renderer.cpp",
    "src/staDescriptors.cpp",
]

# The Qt-free half of the gui: the data model every other module draws
# through -- Painter, Renderer, Descriptor/Selected, HeatMapDataSource --
# published through gui/core.h.  Compiled once and linked by both :gui and
# :gui_qt, so the Qt build no longer carries a second, separately compiled
# copy of these objects.
#
# Not yet complete on its own: the Gui dispatch it calls into still lives in
# gui.cpp (Qt) and stub.cpp (CLI), so a binary has to link one of those too.
cc_library(
    name = "core",
    srcs = CORE_SRCS + [
        "src/bufferTreeDescriptor.h",
        "src/colorGenerator.h",
        "src/dbDescriptors.h",
        "src/heatMapPinDensity.h",
        "src/heatMapPlacementDensity.h",
        "src/staDescriptors.h",
    ],
    hdrs = [
        "include/gui/core.h",
        "include/gui/descriptor_registry.h",
        "include/gui/heatMap.h",
    ],
    includes = [
        "include",
        "src",
    ],
    deps = [
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/odb/src/db",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@abseil-cpp//absl/synchronization",
        "@boost.algorithm",
        "@boost.multi_array",
    ],
)

cc_library(
    name = "gui",
    srcs = [
        ":tcl",
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
        ":core",
        "@tcl_lang//:tcl",
    ],
)

# CLI-only stub implementations of GUI dispatch symbols (Gui::enabled,
# startGui, initGui, Painter::getOptions, ...). Split out of :gui so
# that :openroad-qt — which links //src/gui:gui_qt for the real Qt
# implementations and pulls :gui transitively via :ui modules — does
# NOT also link the stub. With --allow-multiple-definition in effect
# (LLVM toolchain default), having both stub and gui_qt definitions
# present at link time causes the stub to silently win for symbols
# like Gui::enabled, and the GUI never opens at runtime.
#
# Only the CLI :openroad binary (and :_openroadpy.so) need to dep
# :gui_stub explicitly to satisfy those undefined references in :core's
# heatMapCore.cpp et al.
cc_library(
    name = "gui_stub",
    srcs = [
        "src/stub.cpp",
    ],
    deps = [
        ":core",
        ":gui",
        "//src/odb/src/db",
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
            # The Qt-free sources are compiled once, by :core, which this
            # target deps on.  Globbing them here too would put a second,
            # separately compiled copy of every descriptor and heatmap
            # object in the same link as :core's.
            exclude = CORE_SRCS + [
                "src/stub.cpp",
                "src/stub_heatMap.cpp",
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
            ":core",
            ":qt_resources",
            "//:ord",
            "//src/dbSta",
            "//src/dbSta:dbNetwork",
            "//src/odb/src/db",
            "//src/odb/src/defout",
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
