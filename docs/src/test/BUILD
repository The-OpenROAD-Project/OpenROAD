# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("//test:regression.bzl", "doc_check_test", "regression_test")

doc_check_test(
    name = "translator",
)

doc_check_test(
    name = "bazel_developer_activities_check",
    data = [
        "//docs:user/Bazel.md",
        "//docs:user/Bazel-caching.md",
        "//docs:user/Bazel-developer-activities.md",
        "//docs:user/Bazel-targets.md",
    ],
)

regression_test(
    name = "man_func",
    data = [
        "helpers.tcl",
        "man_func.ok",
        "//docs:man_pages",
    ],
)

# The files the Sphinx build in //docs reads.
filegroup(
    name = "docs",
    srcs = ["README.md"],
    visibility = ["//docs:__pkg__"],
)
