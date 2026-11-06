# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:python_wrap_cc.bzl", "python_wrap_cc", "python_wrap_library")
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
        "src/GaussianSurface.cpp",
        "src/LayerModes.cpp",
        "src/LayeredCubeGreens.cpp",
        "src/LayeredSquareGreens.cpp",
        "src/SpatialIndex.cpp",
        "src/SquareGreens.cpp",
    ],
    hdrs = [
        "src/AliasTable.h",
        "src/BoxFace.h",
        "src/CubeGreens.h",
        "src/CubePanels.h",
        "src/Dielectric.h",
        "src/GaussianSurface.h",
        "src/LayerModes.h",
        "src/LayeredCubeGreens.h",
        "src/LayeredSquareGreens.h",
        "src/Philox.h",
        "src/SpatialIndex.h",
        "src/SquareGreens.h",
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
    name = "net_window",
    srcs = ["src/NetWindow.cpp"],
    hdrs = ["include/fsv/NetWindow.h"],
    includes = ["include"],
    deps = [
        ":core",
        ":geometry",
        "//src/odb/src/db",
    ],
)

cc_library(
    name = "field_solver",
    srcs = ["src/FieldSolver.cpp"],
    hdrs = ["include/fsv/FieldSolver.h"],
    includes = ["include"],
    deps = [
        ":fsv",
        ":geometry",
        ":net_window",
        ":stack",
        "//src/odb/src/db",
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

python_wrap_cc(
    name = "swig-py",
    srcs = [
        "src/FieldSolver-py.i",
        "//:error_swig-py",
    ],
    module = "fsv_py",
    root_swig_src = "src/FieldSolver-py.i",
    swig_includes = ["src/fsv/src"],
)

# Compiled into //:_openroad.so with the other Python modules, so that it
# shares the session the Tcl commands use.
python_wrap_library(
    name = "py_wrap",
    deps = [
        ":field_solver",
        ":fsv",
        ":geometry",
        ":net_window",
    ],
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
