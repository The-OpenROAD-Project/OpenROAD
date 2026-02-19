# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "dst",
    srcs = [
        "src/BalancerConnection.cc",
        "src/BalancerConnection.h",
        "src/Distributed.cc",
        "src/JobMessage.cc",
        "src/LoadBalancer.cc",
        "src/LoadBalancer.h",
        "src/Worker.cc",
        "src/Worker.h",
        "src/WorkerConnection.cc",
        "src/WorkerConnection.h",
    ],
    hdrs = [
        "include/dst/BalancerJobDescription.h",
        "include/dst/BroadcastJobDescription.h",
        "include/dst/Distributed.h",
        "include/dst/JobCallBack.h",
        "include/dst/JobMessage.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//src/odb",
        "//src/utl",
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
    name = "ui",
    srcs = [
        "include/dst/Distributed.h",
        "src/MakeDistributed.cc",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/dst/MakeDistributed.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//:ord",
        "//src/odb",
        "//src/utl",
        "@boost.asio",
        "@boost.stacktrace",
        "@tk_tcl//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/Distributed.tcl",
    ],
    char_array_name = "dst_tcl_inits",
    namespace = "dst",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/Distributed.i",
        "//:error_swig",
    ],
    module = "dst",
    namespace_prefix = "dst",
    root_swig_src = "src/Distributed.i",
    swig_includes = [
        "src/dst/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)
