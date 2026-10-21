# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")
load("//test:regression.bzl", "messages_txt")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "fsv",
    srcs = [
        "src/Scene.cpp",
        "src/Solver.cpp",
        "src/WorkerPool.cpp",
        "src/WorkerPool.h",
    ],
    hdrs = [
        "include/fsv/Scene.h",
        "include/fsv/Solver.h",
    ],
    includes = ["include"],
    deps = [
        ":core",
        ":geometry",
        ":stack",
    ],
)

cc_library(
    name = "geometry",
    srcs = ["src/Geometry.cpp"],
    hdrs = ["include/fsv/Geometry.h"],
    includes = ["include"],
)

# Internals, exposed to the unit tests.
cc_library(
    name = "core",
    srcs = [
        "src/AliasTable.cpp",
        "src/CubeGreens.cpp",
        "src/CubePanels.cpp",
        "src/Dielectric.cpp",
        "src/LayeredCubeGreens.cpp",
        "src/SpatialIndex.cpp",
    ],
    hdrs = [
        "src/AliasTable.h",
        "src/BoxFace.h",
        "src/CubeGreens.h",
        "src/CubePanels.h",
        "src/Dielectric.h",
        "src/LayeredCubeGreens.h",
        "src/Philox.h",
        "src/SpatialIndex.h",
    ],
    includes = ["src"],
    visibility = ["//src/fsv:__subpackages__"],
    deps = [
        ":geometry",
        ":stack",
    ],
)

# Process stack and its ITF reader.
cc_library(
    name = "stack",
    srcs = [
        "src/Itf.cpp",
        "src/Stack.cpp",
    ],
    hdrs = [
        "include/fsv/Itf.h",
        "include/fsv/Stack.h",
    ],
    includes = ["include"],
)

# The state behind the Tcl commands.
cc_library(
    name = "field_solver",
    srcs = ["src/FieldSolver.cpp"],
    hdrs = ["include/fsv/FieldSolver.h"],
    includes = ["include"],
    deps = [
        ":fsv",
        ":geometry",
        ":stack",
        "//src/utl",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "src/MakeFieldSolver.cpp",
        ":swig",
        ":tcl",
    ],
    hdrs = ["include/fsv/MakeFieldSolver.h"],
    includes = ["include"],
    deps = [
        ":field_solver",
        ":fsv",
        ":geometry",
        "//:ord",
        "//src/utl",
        "@boost.stacktrace",
        "@tcl_lang//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = ["src/FieldSolver.tcl"],
    char_array_name = "fsv_tcl_inits",
    namespace = "fsv",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/FieldSolver.i",
        "//:error_swig",
    ],
    module = "fsv",
    namespace_prefix = "fsv",
    root_swig_src = "src/FieldSolver.i",
    swig_includes = ["src/fsv/src"],
)

filegroup(
    name = "doc_files",
    srcs = [
        "README.md",
        "src/FieldSolver.tcl",
    ],
    visibility = ["//src/fsv/test:__pkg__"],
)

exports_files(["README.md"])

messages_txt(
    visibility = [
        "//docs:__pkg__",
        "//src/fsv/test:__pkg__",
    ],
)
