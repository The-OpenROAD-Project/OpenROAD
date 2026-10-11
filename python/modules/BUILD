# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_python//python:defs.bzl", "py_library")

package(features = ["layering_check"])

# Top-level per-tool modules (import odb, import ant, ...), matching the
# modules `openroad -python` provides. They re-export the extension
# modules, so //python/openroad:openroadpy depends on this target and the
# wheel installs these files at the top level.
py_library(
    name = "modules",
    srcs = glob(["*.py"]),
    imports = ["."],
    visibility = ["//visibility:public"],
)
