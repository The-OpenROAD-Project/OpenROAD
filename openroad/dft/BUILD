# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")
load("//test:regression.bzl", "messages_txt")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "dft",
    srcs = [
        "src/Dft.cpp",
    ],
    hdrs = [
        "include/dft/Dft.hh",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//openroad/dbSta",
        "//openroad/dft/src/architect",
        "//openroad/dft/src/cells",
        "//openroad/dft/src/clock_domain",
        "//openroad/dft/src/config",
        "//openroad/dft/src/optimizer",
        "//openroad/dft/src/replace",
        "//openroad/dft/src/stitch",
        "//openroad/dft/src/utils",
        "//openroad/odb/src/db",
        "//openroad/rsz",
        "//openroad/utl",
        "@boost.property_tree",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "src/MakeDft.cpp",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/dft/MakeDft.hh",
    ],
    includes = [
        "include",
        "src",
    ],
    deps = [
        ":dft",
        "//:ord",
        "//openroad/dft/src/architect",
        "//openroad/dft/src/clock_domain",
        "//openroad/dft/src/config",
        "//openroad/utl",
        "@tcl_lang//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/dft.tcl",
    ],
    char_array_name = "dft_tcl_inits",
    namespace = "dft",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/dft.i",
        "//:error_swig",
    ],
    module = "dft",
    namespace_prefix = "dft",
    root_swig_src = "src/dft.i",
    swig_includes = [
        "src",
    ],
    deps = [
        "//openroad/odb:swig",
    ],
)

filegroup(
    name = "doc_files",
    srcs = [
        "README.md",
        "src/dft.tcl",
    ],
    visibility = ["//openroad/dft/test:__pkg__"],
)

messages_txt(
    extra_srcs = [
        "//openroad/dft/src/architect:message_srcs",
        "//openroad/dft/src/cells:message_srcs",
        "//openroad/dft/src/clock_domain:message_srcs",
        "//openroad/dft/src/config:message_srcs",
        "//openroad/dft/src/optimizer:message_srcs",
        "//openroad/dft/src/replace:message_srcs",
        "//openroad/dft/src/stitch:message_srcs",
        "//openroad/dft/src/utils:message_srcs",
    ],
    visibility = [
        "//docs:__pkg__",
        "//openroad/dft/test:__pkg__",
    ],
)
