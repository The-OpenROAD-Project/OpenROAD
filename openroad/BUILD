# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors
load("@openroad_rules_python//python:packaging.bzl", "py_wheel", "py_package")


py_library(
    name = "openroadpy",
    srcs = [
        "__init__.py",
        "utl.py",
        "odb.py",
    ],
    deps = [
        "//src/utl:utl_py",
        "//src/odb:odb_py",
        "//:ord_py",
    ],
    imports = ["pypi"],
    visibility = ["//visibility:public"],
)

py_package(
    name = "openroad_pkg",
    deps = [
        ":openroadpy",
    ]
)

py_wheel(
    name = "openroad_wheel",
    distribution = "openroad",
    python_tag = "py3",
    version = "0.0.1",
    deps = [
        ":openroad_pkg",
    ],
)