# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_python//python:defs.bzl", "py_library", "py_test")

py_library(
    name = "extract_utils",
    srcs = ["extract_utils.py"],
    imports = ["."],
    visibility = ["//visibility:public"],
)

py_library(
    name = "man_tcl_check",
    testonly = True,
    srcs = ["man_tcl_check.py"],
    imports = ["."],
    visibility = ["//visibility:public"],
    deps = [":extract_utils"],
)

py_test(
    name = "test_extract_utils",
    srcs = ["test_extract_utils.py"],
    deps = [":extract_utils"],
)

py_test(
    name = "test_man_tcl_check",
    size = "small",
    srcs = ["test_man_tcl_check.py"],
    data = [
        "extract_utils.py",
        "man_tcl_check.py",
    ],
)

exports_files([
    "md_roff_compat.py",
    "manpage.py",
    "link_readmes.sh",
    "extract_utils.py",
])
