# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:python_wrap_cc.bzl", "python_wrap_cc")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")
load("//test:regression.bzl", "messages_txt")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "private_hdrs",
    hdrs = [
        "src/DelayEstimator.hh",
        "src/MoveCommitter.hh",
        "src/MoveTracker.hh",
        "src/OptimizerTypes.hh",
        "src/RepairSetupContext.hh",
        "src/RepairTargetCollector.hh",
        "src/move/MoveCandidate.hh",
        "src/move/MoveGenerator.hh",
        "src/move/VtSwapMtCandidate.hh",
        "src/policy/OptimizationPolicy.hh",
        "src/policy/SetupLegacyBase.hh",
        "src/policy/SetupLegacyMtPolicy.hh",
        "src/policy/SetupLegacyPolicy.hh",
        "src/policy/SetupMt1Policy.hh",
    ],
    includes = [
        "src",
        "src/move",
        "src/policy",
    ],
    visibility = ["//src/rsz:__subpackages__"],
    deps = [
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/odb/src/db",
        "//src/sta:opensta_lib",
        "//src/utl",
    ],
)

cc_library(
    name = "rsz",
    srcs = [
        "src/BufferedNet.cc",
        "src/BufferedNet.hh",
        "src/ConcreteSwapArithModules.cc",
        "src/ConcreteSwapArithModules.hh",
        "src/DelayEstimator.cc",
        "src/DelayEstimatorReporter.cc",
        "src/DelayEstimatorReporter.hh",
        "src/MoveCommitter.cc",
        "src/MoveTracker.cc",
        "src/OdbCallBack.cc",
        "src/Optimizer.cc",
        "src/Optimizer.hh",
        "src/OptimizerTypes.cc",
        "src/PreChecks.cc",
        "src/PreChecks.hh",
        "src/Rebuffer.cc",
        "src/Rebuffer.hh",
        "src/RecoverPower.cc",
        "src/RecoverPower.hh",
        "src/RepairDesign.cc",
        "src/RepairDesign.hh",
        "src/RepairHold.cc",
        "src/RepairHold.hh",
        "src/RepairTargetCollector.cc",
        "src/Resizer.cc",
        "src/ResizerObserver.hh",
        "src/SwapArithModules.hh",
        "src/move/BufferCandidate.cc",
        "src/move/BufferCandidate.hh",
        "src/move/BufferGenerator.cc",
        "src/move/BufferGenerator.hh",
        "src/move/CloneCandidate.cc",
        "src/move/CloneCandidate.hh",
        "src/move/CloneGenerator.cc",
        "src/move/CloneGenerator.hh",
        "src/move/MeasuredVtSwapCandidate.cc",
        "src/move/MeasuredVtSwapCandidate.hh",
        "src/move/MeasuredVtSwapGenerator.cc",
        "src/move/MeasuredVtSwapGenerator.hh",
        "src/move/MoveCandidate.cc",
        "src/move/MoveGenerator.cc",
        "src/move/RerouteCandidate.cc",
        "src/move/RerouteCandidate.hh",
        "src/move/RerouteGenerator.cc",
        "src/move/RerouteGenerator.hh",
        "src/move/SizeDownCandidate.cc",
        "src/move/SizeDownCandidate.hh",
        "src/move/SizeDownGenerator.cc",
        "src/move/SizeDownGenerator.hh",
        "src/move/SizeUpCandidate.cc",
        "src/move/SizeUpCandidate.hh",
        "src/move/SizeUpGenerator.cc",
        "src/move/SizeUpGenerator.hh",
        "src/move/SizeUpMatchCandidate.cc",
        "src/move/SizeUpMatchCandidate.hh",
        "src/move/SizeUpMatchGenerator.cc",
        "src/move/SizeUpMatchGenerator.hh",
        "src/move/SizeUpMtCandidate.cc",
        "src/move/SizeUpMtCandidate.hh",
        "src/move/SizeUpMtGenerator.cc",
        "src/move/SizeUpMtGenerator.hh",
        "src/move/SplitLoadCandidate.cc",
        "src/move/SplitLoadCandidate.hh",
        "src/move/SplitLoadGenerator.cc",
        "src/move/SplitLoadGenerator.hh",
        "src/move/SwapPinsCandidate.cc",
        "src/move/SwapPinsCandidate.hh",
        "src/move/SwapPinsGenerator.cc",
        "src/move/SwapPinsGenerator.hh",
        "src/move/UnbufferCandidate.cc",
        "src/move/UnbufferCandidate.hh",
        "src/move/UnbufferGenerator.cc",
        "src/move/UnbufferGenerator.hh",
        "src/move/VtSwapCandidate.cc",
        "src/move/VtSwapCandidate.hh",
        "src/move/VtSwapGenerator.cc",
        "src/move/VtSwapGenerator.hh",
        "src/move/VtSwapMtCandidate.cc",
        "src/move/VtSwapMtGenerator.cc",
        "src/move/VtSwapMtGenerator.hh",
        "src/policy/MeasuredVtSwapPolicy.cc",
        "src/policy/MeasuredVtSwapPolicy.hh",
        "src/policy/OptimizationPolicy.cc",
        "src/policy/SetupCritVtSwapPolicy.cc",
        "src/policy/SetupCritVtSwapPolicy.hh",
        "src/policy/SetupDirectionalPolicy.cc",
        "src/policy/SetupDirectionalPolicy.hh",
        "src/policy/SetupLastGaspPolicy.cc",
        "src/policy/SetupLastGaspPolicy.hh",
        "src/policy/SetupLegacyBase.cc",
        "src/policy/SetupLegacyMtPolicy.cc",
        "src/policy/SetupLegacyPolicy.cc",
        "src/policy/SetupMt1Policy.cc",
        "src/policy/SetupReroutePolicy.cc",
        "src/policy/SetupReroutePolicy.hh",
        "src/policy/SetupTnsPolicy.cc",
        "src/policy/SetupTnsPolicy.hh",
        "src/policy/SetupWnsPolicy.cc",
        "src/policy/SetupWnsPolicy.hh",
    ],
    hdrs = [
        "include/rsz/OdbCallBack.hh",
        "include/rsz/Resizer.hh",
    ],
    includes = [
        "include",
        "src",
        "src/move",
        "src/policy",
    ],
    linkopts = ["-pthread"],
    deps = [
        ":private_hdrs",
        "//src/dbSta",
        "//src/dbSta:SpefWriter",
        "//src/dbSta:dbNetwork",
        "//src/dpl",
        "//src/est",
        "//src/grt",
        "//src/grt:groute",
        "//src/odb/src/db",
        "//src/sta:opensta_lib",
        "//src/stt",
        "//src/utl",
        "@boost.container_hash",
        "@boost.multi_array",
        "@spdlog",
        "@tcl_lang//:tcl",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "src/BufferedNet.hh",
        "src/Graphics.cc",
        "src/Graphics.hh",
        "src/MakeResizer.cc",
        "src/ResizerObserver.hh",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/rsz/MakeResizer.hh",
    ],
    includes = [
        "include",
        "src",
    ],
    deps = [
        ":rsz",
        "//:ord",
        "//src/dbSta:dbNetwork",
        "//src/gui",
        "//src/odb/src/db",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@boost.stacktrace",
        "@spdlog",
        "@tcl_lang//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/Resizer.tcl",
    ],
    char_array_name = "rsz_tcl_inits",
    namespace = "rsz",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/Resizer.i",
        "//:error_swig",
        "//src/sta:sta_swig_files",
    ],
    module = "rsz",
    namespace_prefix = "rsz",
    root_swig_src = "src/Resizer.i",
    swig_includes = [
        "../sta",
        "src",
    ],
)

python_wrap_cc(
    name = "swig-py",
    srcs = [
        "include/rsz/Resizer.hh",
        "src/Resizer-py.i",
        "//:error_swig-py",
    ],
    module = "rsz_py",
    root_swig_src = "src/Resizer-py.i",
    swig_includes = [
        "include",
        "src",
    ],
    deps = [
        "//src/odb:swig-py",
    ],
)

filegroup(
    name = "doc_files",
    srcs = [
        "README.md",
        "src/Resizer.tcl",
    ],
    visibility = ["//src/rsz/test:__pkg__"],
)

messages_txt(
    visibility = ["//src/rsz/test:__pkg__"],
)
