# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_python//python:defs.bzl", "py_library", "py_test")

py_library(
    name = "extract_utils",
    srcs = ["extract_utils.py"],
    imports = ["."],
    visibility = ["//visibility:public"],
)

py_test(
    name = "test_extract_utils",
    srcs = ["test_extract_utils.py"],
    deps = [":extract_utils"],
)
