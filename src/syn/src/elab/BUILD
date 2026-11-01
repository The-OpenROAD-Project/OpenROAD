# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//test:regression.bzl", "message_srcs")

package(features = ["layering_check"])

# syn IR backend for sv-elab — full library with implementations.
#
# The shared sv-elab frontend is pulled in as file lists (@sv-elab's
# common_srcs/frontend_hdrs) and compiled into this target together with the
# backend. The frontend's sources include this package's ir.h/backend_builder.h
# and this backend includes slang_frontend.h, so the two sides must share one
# compilation unit rather than being split across Bazel modules/targets.
cc_library(
    name = "elab",
    srcs = [
        "backend_builder.cc",
        "blackboxes.cc",
        "driver.cc",
        "error.cc",
        "ir.cc",
        "@sv-elab//:common_srcs",
    ],
    hdrs = [
        "backend_builder.h",
        "blackboxes.h",
        "driver.h",
        "ir.h",
    ],
    defines = [
        "SLANG_MUX_LOWERING",
        "SLANG_NO_YOSYS",
    ],
    includes = ["."],
    visibility = [
        "//src/syn:__pkg__",
        "//src/syn/test:__pkg__",
    ],
    deps = [
        "//src/dbSta",
        "//src/sta:opensta_lib",
        "//src/syn/src/ir",
        "//src/utl",
        "@fmt",
        "@sv-elab//:frontend_hdrs",
        "@sv-elab//:hashlib",
        "@sv-elab//:slang",
        "@sv-elab//:version_hdrs",
    ],
    alwayslink = True,
)

message_srcs(
    visibility = ["//src/syn:__pkg__"],
)
