# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "ir",
    srcs = [
        "Bundle.cc",
        "Const.cc",
        "Dump.cc",
        "Graph.cc",
        "Instance.cc",
        "NetTable.cc",
        "Parse.cc",
        "TritModel.cc",
    ],
    hdrs = [
        "Const.h",
        "ControlNet.h",
        "Graph.h",
        "Instance.h",
        "Net.h",
        "NetTable.h",
        "NetTableEntry.h",
        "Bundle.h",
        "TritModel.h",
    ],
    strip_include_prefix = "//src/syn/src/ir",
    include_prefix = "syn/ir",
    visibility = ["//visibility:public"],
    deps = [
        "@abc",
        "//src/sta:opensta_lib",
        "//src/utl",
    ],
)
