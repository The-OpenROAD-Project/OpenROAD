# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

# Patterns built from a process stack by layer name.
cc_library(
    name = "patterns",
    srcs = ["src/Patterns.cpp"],
    hdrs = ["include/pex/Patterns.h"],
    includes = ["include"],
    deps = [
        "//src/fsv:geometry",
        "//src/fsv:stack",
    ],
)

# The capacitance tables and their lookup.
cc_library(
    name = "model",
    srcs = ["src/Model.cpp"],
    hdrs = ["include/pex/Model.h"],
    includes = ["include"],
)

# How a piece's covers map onto the model's contexts.
cc_library(
    name = "cover_map",
    srcs = ["src/CoverMap.cpp"],
    hdrs = ["include/pex/CoverMap.h"],
    includes = ["include"],
    deps = [
        ":model",
        ":patterns",
        "//src/fsv:stack",
    ],
)

# Builds the tables by solving patterns with fsv.
cc_library(
    name = "model_builder",
    srcs = ["src/ModelBuilder.cpp"],
    hdrs = ["include/pex/ModelBuilder.h"],
    includes = ["include"],
    deps = [
        ":model",
        ":patterns",
        "//src/fsv",
        "//src/fsv:geometry",
        "//src/fsv:stack",
        "//src/utl",
    ],
)

# The model file.
cc_library(
    name = "model_json",
    srcs = ["src/ModelJson.cpp"],
    hdrs = ["include/pex/ModelJson.h"],
    copts = ["-DBOOST_JSON_HEADER_ONLY"],
    includes = ["include"],
    deps = [
        ":model",
        "@boost.json",
    ],
)

exports_files(["README.md"])
