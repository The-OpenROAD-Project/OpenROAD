# SPDX-License-Identifier: MIT
# Copyright (c) 2022-2025 Alexei Sibidanov

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "core-math",
    srcs = ["erff.c"],
    hdrs = ["erff.h"],
    # Contracting a*b+c into an FMA changes the rounding of the
    # polynomial evaluation; .bazelrc sets this flag for C++ only.
    copts = ["-ffp-contract=off"],
    # Exposes `erff.h` to dependents without the `third-party/core-math/`
    # path prefix, so they write `#include "erff.h"`.
    includes = ["."],
    tags = ["no-lint"],
    visibility = ["//visibility:public"],
)
