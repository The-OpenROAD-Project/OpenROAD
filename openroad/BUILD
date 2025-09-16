# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors
load("@openroad_rules_python//python:packaging.bzl", "py_package", "py_wheel")

py_library(
    name = "openroadpy",
    srcs = [
        "__init__.py",
        "odb.py",
        "utl.py",
    ],
    imports = ["pypi"],
    visibility = ["//visibility:public"],
    deps = [
        "//:ord_py",
        "//src/odb:odb_py",
        "//src/utl:utl_py",
    ],
)

py_package(
    name = "openroad_pkg",
    deps = [
        ":openroadpy",
    ],
)

py_wheel(
    name = "openroad_wheel",
    distribution = "openroad",
    homepage = "https://github.com/The-OpenROAD-Project/OpenROAD",
    python_tag = "py3",
    version = "0.0.1",
    deps = [
        ":openroad_pkg",
    ],
)
