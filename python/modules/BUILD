# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_python//python:defs.bzl", "py_library")

package(features = ["layering_check"])

# Top-level per-tool modules (import odb, import ant, ...), matching the
# modules `openroad -python` provides, except syn, ram and web, which have
# no Python extension in the Bazel build. Each imports its own SWIG module
# through _openroad_loader, which loads it from //:_openroad.so.
# //:openroad_py depends on this target and the wheel installs these files
# at the top level.
py_library(
    name = "modules",
    srcs = glob(["*.py"]),
    imports = ["."],
    visibility = ["//visibility:public"],
)
