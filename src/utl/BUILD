# -*- Python -*-

load("//:bazel/tcl_wrap_cc.bzl", "tcl_wrap_cc")
load("//:bazel/tcl_encode.bzl", "tcl_encode")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

# TODO: the prometheus target should be broken out, and possibly logger.
# However, there are cyclic dependencies currently between all thse, which
# need to be taken care of first
cc_library(
    name = "utl",
    srcs = [
        "src/CFileUtils.cpp",
        "src/CommandLineProgress.cpp",
        "src/CommandLineProgress.h",
        "src/Logger.cpp",
        "src/LoggerCommon.cpp",
        "src/LoggerCommon.h",
        "src/MakeLogger.cpp",
        "src/Metrics.cpp",
        "src/Progress.cpp",
        "src/ScopedTemporaryFile.cpp",
        "src/prometheus/metrics_server.cpp",
        "src/timer.cpp",
    ],
    hdrs = [
        "include/utl/CFileUtils.h",
        "include/utl/Logger.h",
        "include/utl/MakeLogger.h",
        "include/utl/Metrics.h",
        "include/utl/Progress.h",
        "include/utl/ScopedTemporaryFile.h",
        "include/utl/algorithms.h",
        "include/utl/deleter.h",
        "include/utl/exception.h",
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
        "include/utl/timer.h",
        "include/utl/validation.h",
    ],
    includes = ["include"],
    deps = [
        "@boost.asio",
        #"@boost.beast",   # this results in a linking issue (bzlmod)
        "@boost//:beast",  #.. so for now use this one (from bazel_rules_hdl)
        "@boost.core",
        "@spdlog",
        "@tk_tcl//:tcl",
        "//:opensta_lib",
    ],
)

tcl_wrap_cc(
    name = "utl_swig",
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
    name = "utl_tcl",
    srcs = [
        "src/Utl.tcl",
    ],
    char_array_name = "utl_tcl_inits",
    visibility = ["//visibility:public"],
)
