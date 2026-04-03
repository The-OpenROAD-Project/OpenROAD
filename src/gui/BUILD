# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

# Qt-specific targets (gui_qt, qt_resources) live in //src/gui/qt so that
# @qt-bazel is only loaded when building with --//:platform=gui.

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")
load("//test:regression.bzl", "messages_txt")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

# Shared descriptor code, compiled without Qt.  Used by the CLI stub build
# and (via deps) by the Qt gui_qt target in //src/gui/qt.
cc_library(
    name = "gui_descriptors",
    srcs = [
        "src/dbDescriptors.cpp",
        "src/descriptor_registry.cpp",
        "src/init_descriptors.cpp",
        "src/ruler.cpp",
        "src/staDescriptors.cpp",
    ],
    hdrs = [
        "include/gui/MakeGui.h",
        "include/gui/descriptor_registry.h",
        "include/gui/gui.h",
        "src/bufferTreeDescriptor.h",
        "src/dbDescriptors.h",
        "src/ruler.h",
        "src/staDescriptors.h",
    ],
    includes = [
        ".",
        "include",
        "src",
    ],
    visibility = [
        "//:__subpackages__",
        "//src/gui/qt:__pkg__",
    ],
    deps = [
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@abseil-cpp//absl/synchronization",
        "@boost.algorithm",
        "@boost.geometry",
        "@boost.multi_array",
        "@spdlog",
        "@tcl_lang//:tcl",
    ],
)

# Shared heat-map core, compiled without Qt.  Used by the CLI stub build
# and (via deps) by the Qt gui_qt target in //src/gui/qt.
cc_library(
    name = "gui_heatmap_core",
    srcs = [
        "src/heatMapCore.cpp",
        "src/heatMapPinDensity.cpp",
        "src/heatMapPlacementDensity.cpp",
    ],
    hdrs = [
        "include/gui/gui.h",
        "include/gui/heatMap.h",
        "src/heatMapPinDensity.h",
        "src/heatMapPlacementDensity.h",
    ],
    includes = [
        ".",
        "include",
        "src",
    ],
    visibility = [
        "//:__subpackages__",
        "//src/gui/qt:__pkg__",
    ],
    deps = [
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@abseil-cpp//absl/synchronization",
        "@boost.algorithm",
        "@boost.multi_array",
        "@spdlog",
    ],
)

cc_library(
    name = "gui",
    srcs = [
        "src/stub.cpp",
        ":tcl",
    ],
    hdrs = [
        "include/gui/MakeGui.h",
        "include/gui/descriptor_registry.h",
        "include/gui/gui.h",
        "include/gui/heatMap.h",
    ],
    includes = [
        ".",
        "include",
        "src",
    ],
    deps = [
        ":gui_descriptors",
        ":gui_heatmap_core",
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@abseil-cpp//absl/synchronization",
        "@boost.algorithm",
        "@boost.multi_array",
        "@spdlog",
        "@tcl_lang//:tcl",
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
    visibility = ["//src/gui/test:__pkg__"],
)
