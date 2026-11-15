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

# The model applied to conductor boxes.
cc_library(
    name = "evaluator",
    srcs = ["src/Evaluator.cpp"],
    hdrs = ["include/pex/Evaluator.h"],
    includes = ["include"],
    deps = [
        ":cover_map",
        ":model",
        ":patterns",
        "//src/fsv:geometry",
        "//src/fsv:stack",
        "//src/odb/src/db",
    ],
)

# The block's boxes in an rtree per layer.
cc_library(
    name = "index",
    srcs = ["src/Index.cpp"],
    hdrs = ["include/pex/Index.h"],
    includes = ["include"],
    deps = [
        "//src/odb/src/db",
        "//src/utl",
        "@boost.geometry",
        "@boost.iterator",
    ],
)

# Resistance of wires and vias from the stack.
cc_library(
    name = "resistance",
    srcs = ["src/Resistance.cpp"],
    hdrs = ["include/pex/Resistance.h"],
    includes = ["include"],
    deps = [
        "//src/fsv:stack",
        "//src/odb/src/db",
    ],
)

# A net's resistor network.
cc_library(
    name = "network",
    srcs = ["src/Network.cpp"],
    hdrs = ["include/pex/Network.h"],
    includes = ["include"],
    deps = [
        ":resistance",
        "//src/odb/src/db",
    ],
)

# Extraction of a block into ODB parasitics.
cc_library(
    name = "extractor",
    srcs = ["src/Extractor.cpp"],
    hdrs = ["include/pex/Extractor.h"],
    includes = ["include"],
    deps = [
        ":evaluator",
        ":index",
        ":model",
        ":network",
        ":patterns",
        ":resistance",
        "//src/fsv:geometry",
        "//src/fsv:net_window",
        "//src/fsv:stack",
        "//src/odb/src/db",
        "//src/odb/src/wireNormalizer",
        "//src/utl",
    ],
)

# SPEF from ODB parasitics.
cc_library(
    name = "spef_writer",
    srcs = ["src/SpefWriter.cpp"],
    hdrs = ["include/pex/SpefWriter.h"],
    includes = ["include"],
    deps = ["//src/odb/src/db"],
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

# The state behind the commands.
cc_library(
    name = "pex",
    srcs = ["src/Pex.cpp"],
    hdrs = ["include/pex/Pex.h"],
    includes = ["include"],
    deps = [
        ":extractor",
        ":model",
        ":model_builder",
        ":model_json",
        ":spef_writer",
        "//src/fsv:stack",
        "//src/odb/src/db",
        "//src/utl",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "src/MakePex.cpp",
        ":swig",
        ":tcl",
    ],
    hdrs = ["include/pex/MakePex.h"],
    includes = ["include"],
    deps = [
        ":extractor",
        ":pex",
        "//:ord",
        "//src/utl",
        "@tcl_lang//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = ["src/Pex.tcl"],
    char_array_name = "pex_tcl_inits",
    namespace = "pex",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/Pex.i",
        "//:error_swig",
    ],
    module = "pex",
    namespace_prefix = "pex",
    root_swig_src = "src/Pex.i",
    swig_includes = ["src/pex/src"],
)

python_wrap_cc(
    name = "swig-py",
    srcs = [
        "src/Pex-py.i",
        "//:error_swig-py",
    ],
    module = "pex_py",
    root_swig_src = "src/Pex-py.i",
    swig_includes = ["src/pex/src"],
)

# Compiled into //:_openroad.so with the other Python modules, so that it
# shares the session the Tcl commands use.
python_wrap_library(
    name = "py_wrap",
    deps = [
        ":extractor",
        ":pex",
    ],
)

filegroup(
    name = "doc_files",
    srcs = [
        "README.md",
        "src/Pex.tcl",
    ],
    visibility = ["//src/pex/test:__pkg__"],
)

exports_files(["README.md"])

messages_txt(
    visibility = [
        "//docs:__pkg__",
        "//src/pex/test:__pkg__",
    ],
)
