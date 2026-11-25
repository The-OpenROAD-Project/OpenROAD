# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_python//python:defs.bzl", "py_binary")

# Regenerate the ODB sources in the source tree:
#   bazelisk run //openroad/odb/src/codeGenerator:gen
# It reads the schema and templates from the source tree, and formats the
# output with the clang-format that //:fmt_cc_test uses.
py_binary(
    name = "gen",
    srcs = [
        "field_types.py",
        "gen.py",
        "helper.py",
        "parser.py",
        "schema_models.py",
    ],
    args = ["--clang-format=$(rootpath //bazel:clang-format)"],
    data = ["//bazel:clang-format"],
    imports = ["."],
    deps = ["@openroad-pip//jinja2"],
)
