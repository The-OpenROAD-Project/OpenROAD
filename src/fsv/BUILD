# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

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
        "src/LayeredCubeGreens.cpp",
        "src/SpatialIndex.cpp",
    ],
    hdrs = [
        "src/AliasTable.h",
        "src/BoxFace.h",
        "src/CubeGreens.h",
        "src/CubePanels.h",
        "src/LayeredCubeGreens.h",
        "src/Philox.h",
        "src/SpatialIndex.h",
    ],
    includes = ["src"],
    visibility = ["//src/fsv:__subpackages__"],
    deps = [":geometry"],
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
