# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2026, The OpenROAD Authors

load("@rules_python//python:defs.bzl", "py_library")

py_library(
    name = "extract_utils",
    srcs = ["extract_utils.py"],
    imports = ["."],
    visibility = ["//visibility:public"],
)

py_library(
    name = "man_tcl_check",
    srcs = ["man_tcl_check.py"],
    imports = ["."],
    visibility = ["//visibility:public"],
    deps = [":extract_utils"],
)
