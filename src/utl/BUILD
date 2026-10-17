# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:python_wrap_cc.bzl", "python_wrap_cc", "python_wrap_library")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")
load("//test:regression.bzl", "messages_txt")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

# TODO: the prometheus target should be broken out, and possibly logger.
# However, there are cyclic dependencies currently between all these, which
# need to be taken care of first
cc_library(
    name = "utl",
    srcs = [
        "src/CFileUtils.cpp",
        "src/CommandLineProgress.cpp",
        "src/CommandLineProgress.h",
        "src/CsvParser.cpp",
        "src/Logger.cpp",
        "src/Metrics.cpp",
        "src/Progress.cpp",
        "src/ScopedTemporaryFile.cpp",
        "src/ServiceRegistry.cpp",
        "src/SuppressStdout.cpp",
        "src/ThreadPool.cpp",
        "src/cpu_topology.cpp",
        "src/decode.cpp",
        "src/histogram.cpp",
        "src/mem_stats.cpp",
        "src/prometheus/metrics_server.cpp",
        "src/timer.cpp",
        "src/unionFind.cpp",
    ],
    hdrs = [
        "include/utl/CFileUtils.h",
        "include/utl/CsvParser.h",
        "include/utl/Logger.h",
        "include/utl/Metrics.h",
        "include/utl/Progress.h",
        "include/utl/ScopedTemporaryFile.h",
        "include/utl/ServiceRegistry.h",
        "include/utl/SuppressStdout.h",
        "include/utl/ThreadPool.h",
        "include/utl/algorithms.h",
        "include/utl/cpu_topology.h",
        "include/utl/decode.h",
        "include/utl/deleter.h",
        "include/utl/env.h",
        "include/utl/exception.h",
        "include/utl/histogram.h",
        "include/utl/mem_stats.h",
        "include/utl/options.h",
        "include/utl/prometheus/atomic_floating.h",
        "include/utl/prometheus/benchmark.h",
        "include/utl/prometheus/builder.h",
        "include/utl/prometheus/ckms_quantiles.h",
        "include/utl/prometheus/client_metric.h",
        "include/utl/prometheus/collectable.h",
        "include/utl/prometheus/counter.h",
        "include/utl/prometheus/family.h",
        "include/utl/prometheus/gauge.h",
        "include/utl/prometheus/hash.h",
        "include/utl/prometheus/histogram.h",
        "include/utl/prometheus/metric_family.h",
        "include/utl/prometheus/metrics_server.h",
        "include/utl/prometheus/prometheus_metric.h",
        "include/utl/prometheus/registry.h",
        "include/utl/prometheus/save_to_file.h",
        "include/utl/prometheus/summary.h",
        "include/utl/prometheus/text_serializer.h",
        "include/utl/prometheus/time_window_quantiles.h",
        "include/utl/scope.h",
        "include/utl/spatialIndex.h",
        "include/utl/timer.h",
        "include/utl/unionFind.h",
        "include/utl/unique_name.h",
        "include/utl/validation.h",
    ],
    includes = [
        "include",
        "src",
    ],
    deps = [
        "@abseil-cpp//absl/synchronization",
        "@boost.algorithm",
        "@boost.asio",
        "@boost.beast",
        "@boost.geometry",
        "@boost.iostreams",
        "@boost.iterator",
        "@boost.random",
        "@boost.tokenizer",
        "@spdlog",
        "@tcl_lang//:tcl",
    ],
)

# The utl module is part of //:openroad_py.
alias(
    name = "utl_py",
    actual = "//:openroad_py",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "ui",
    srcs = [
        "src/LoggerCommon.cpp",
        "src/LoggerCommon.h",
        "src/MakeLogger.cpp",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/utl/MakeLogger.h",
    ],
    includes = [
        "include",
        "src",
    ],
    deps = [
        ":utl",
        "//:ord",
        "@boost.stacktrace",
        "@rules_python//python/cc:current_py_cc_headers",
        "@tcl_lang//:tcl",
    ],
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/Logger.i",
        "src/LoggerCommon.h",
        "//:error_swig",
    ],
    module = "utl",
    namespace_prefix = "utl",
    root_swig_src = "src/Logger.i",
    swig_includes = [
        "src/utl/src",
    ],
    visibility = ["//visibility:public"],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/Utl.tcl",
    ],
    char_array_name = "utl_tcl_inits",
    namespace = "utl",
    visibility = ["//visibility:public"],
)

python_wrap_cc(
    name = "swig-py",
    srcs = [
        "include/utl/Logger.h",
        "src/Logger-py.i",
        "src/LoggerCommon.h",
        "//:error_swig-py",
    ],
    module = "utl_py",
    root_swig_src = "src/Logger-py.i",
    swig_includes = [
        "include",
        "src",
    ],
    visibility = ["//visibility:public"],
)

filegroup(
    name = "doc_files",
    srcs = [
        "README.md",
        "src/Utl.tcl",
    ],
    visibility = ["//src/utl/test:__pkg__"],
)

messages_txt(
    visibility = [
        "//docs:__pkg__",
        "//src/utl/test:__pkg__",
    ],
)

# LoggerCommon.cpp comes with the Tcl bindings that ord links.
python_wrap_library(
    name = "py_wrap",
    srcs = ["src/LoggerCommon.h"],
    includes = ["src"],
    deps = [":utl"],
)
