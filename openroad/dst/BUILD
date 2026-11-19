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
    name = "dst_impl",
    srcs = [
        "BalancerConnection.cc",
        "Distributed.cc",
        "JobMessage.cc",
        "LoadBalancer.cc",
        "Worker.cc",
        "WorkerConnection.cc",
    ],
    hdrs = [
        "BalancerConnection.h",
        "BalancerJobDescription.h",
        "BroadcastJobDescription.h",
        "Distributed.h",
        "JobCallBack.h",
        "JobMessage.h",
        "LoadBalancer.h",
        "Worker.h",
        "WorkerConnection.h",
    ],
    visibility = [
        "//openroad/dst:__subpackages__",
    ],
    deps = [
        "//openroad/utl",
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/synchronization",
        "@boost.asio",
        "@boost.bind",
        "@boost.serialization",
        "@boost.smart_ptr",
        "@boost.system",
    ] + select({
        "@platforms//os:macos": ["@boost.thread//:thread_mac"],
        "//conditions:default": ["@boost.thread//:thread_posix"],
    }),
)

cc_library(
    name = "dst",
    hdrs = [
        "BalancerJobDescription.h",
        "BroadcastJobDescription.h",
        "Distributed.h",
        "JobCallBack.h",
        "JobMessage.h",
    ],
    deps = [
        ":dst_impl",
        "@boost.asio",
        "@boost.serialization",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "MakeDistributed.cc",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "MakeDistributed.h",
    ],
    deps = [
        ":dst_impl",
        "//:ord",
        "//openroad/utl",
        "@tcl_lang//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "Distributed.tcl",
    ],
    char_array_name = "dst_tcl_inits",
    namespace = "dst",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "Distributed.i",
        "//:error_swig",
    ],
    module = "dst",
    namespace_prefix = "dst",
    root_swig_src = "Distributed.i",
    deps = [
        "//openroad/odb:swig",
    ],
)

messages_txt(
    src_dir = ".",
    visibility = ["//docs:__pkg__"],
)
