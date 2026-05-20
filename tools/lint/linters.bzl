# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, Precision Innovations Inc.

"""Lint aspect declarations for OpenROAD."""

load("@aspect_rules_lint//lint:clang_tidy.bzl", "lint_clang_tidy_aspect")

clang_tidy = lint_clang_tidy_aspect(
    binary = Label("//tools/lint:clang_tidy"),
    global_config = [Label("//:.clang-tidy")],
    # Defer header filtering to HeaderFilterRegex in //:.clang-tidy.
    lint_target_headers = False,
    angle_includes_are_system = False,
)
