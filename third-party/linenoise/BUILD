# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2010-2023, Salvatore Sanfilippo <antirez at gmail dot com>

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "linenoise",
    srcs = ["linenoise.c"],
    hdrs = ["linenoise.h"],
    # Exposes `linenoise.h` to dependents without the
    # `third-party/linenoise/` path prefix, so the same `#include "linenoise.h"`
    # works under both Bazel and CMake (CMake adds the equivalent include
    # path via target_include_directories(PUBLIC) in this dir's CMakeLists.txt).
    includes = ["."],
    visibility = ["//visibility:public"],
)
