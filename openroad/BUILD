load("@rules_python//python:defs.bzl", "py_library")

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors
load("@rules_python//python:packaging.bzl", "py_package", "py_wheel")

package(features = ["layering_check"])

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
    abi = "abi3",
    distribution = "openroad",
    homepage = "https://github.com/The-OpenROAD-Project/OpenROAD",
    platform = "linux_x86_64",
    python_requires = ">=3.10",
    python_tag = "cp310",
    version = "0.0.1",
    deps = [
        ":openroad_pkg",
    ],
)
