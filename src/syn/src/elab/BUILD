# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

# Headers that the slang-elab shared frontend code needs to find:
# ir.h, backend_builder.h, log_stubs.h
# This is a header-only target so the frontend can compile against
# our ir:: types and backend interface.
cc_library(
    name = "elab_hdrs",
    hdrs = [
        "backend_builder.h",
        "ir.h",
        "log_stubs.h",
    ],
    includes = ["."],
    visibility = ["//visibility:public"],
    deps = [
        "//src/syn/src/ir",
    ],
)

# syn IR backend for slang-elab — full library with implementations
cc_library(
    name = "elab",
    srcs = [
        "abort_helpers.cc",
        "backend_builder.cc",
        "blackboxes.cc",
        "driver.cc",
        "error.cc",
        "ir.cc",
    ],
    hdrs = [
        "blackboxes.h",
        "driver.h",
    ],
    defines = [
        "SLANG_MUX_LOWERING",
        "SLANG_NO_YOSYS",
    ],
    includes = ["."],
    visibility = ["//visibility:public"],
    deps = [
        ":elab_hdrs",
        "//src/dbSta",
        "//src/syn/src/ir",
        "//third-party/slang-elab/src:slang_frontend",
        "@fmt",
        "@sv-lang//:libsvlang",
    ],
    alwayslink = True,
)
