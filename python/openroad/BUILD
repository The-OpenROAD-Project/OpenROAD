load("@rules_python//python:defs.bzl", "py_library", "py_test")

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors
load("@rules_python//python:packaging.bzl", "py_package", "py_wheel")
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

py_library(
    name = "openroadpy",
    srcs = [
        "__init__.py",
        "odb.py",
        "utl.py",
    ],
    imports = [".."],
    visibility = ["//visibility:public"],
    deps = ["//:openroad_py"],
)

regression_test(
    name = "tech_lifecycle",
    check_log = False,
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
    # Install the openroad package and the per-tool modules at the top
    # level. The extension modules keep their src/<tool> paths, which the
    # generated wrappers import from.
    strip_path_prefixes = [
        "python/modules/",
        "python/",
    ],
    version = "0.0.1",
    deps = [
        ":openroad_pkg",
    ],
)

# Python binding unit tests run on the hermetic interpreter with unittest.
py_test(
    name = "openroad_test",
    size = "small",
    srcs = ["openroad_test.py"],
    deps = [":openroadpy"],
)

# The wheel installs and imports on its own, without the Bazel runfiles.
py_test(
    name = "wheel_test",
    size = "small",
    srcs = ["wheel_test.py"],
    data = [":openroad_wheel"],
)

# The per-tool modules: separate namespaces, one shared library.
py_test(
    name = "modules_test",
    size = "small",
    srcs = ["modules_test.py"],
    data = [
        "//src/ant/test:ant_check.def",
        "//src/ant/test:ant_check.lef",
    ],
    deps = [":openroadpy"],
)
