# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, The OpenROAD Authors

# Build file overlaid onto an archived OpenROAD source snapshot; see the
# "ODB schema snapshots" section of //MODULE.bazel.
#
# The snapshot's own BUILD files are deleted on fetch (patch_cmds), because
# at these revisions //src/odb:odb is a monolith that also compiles the
# DEF/LEF parsers and links Tcl, and //src/utl pulls in OpenSTA. None of
# that is reachable from dbDatabase::read/write, so this file builds only
# the serialization core plus the logger it reports through.
#
# Nothing here may depend on the checked-out tree: the whole point of a
# snapshot is that it is frozen, so it carries its own copy of utl.

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "odb_snapshot",
    # allow_empty: these globs run against frozen trees of differing vintage,
    # so a pattern that matches in one snapshot may legitimately match nothing
    # in another. An empty match is not a mistake here the way it is in the
    # checked-out tree.
    srcs = glob(
        [
            "src/odb/src/db/*.cpp",
            "src/odb/src/db/*.h",
            "src/odb/src/db/*.hpp",
            "src/odb/src/zutil/*.cpp",
        ],
        allow_empty = True,
    ) + [
        # The logger, and the little of utl that Logger.cpp reaches into.
        # Logger's constructor builds a PrometheusRegistry and its header
        # holds a unique_ptr<PrometheusMetricsServer>, so the metrics
        # server has to be linked in even though a converter never starts
        # an endpoint.
        "src/utl/src/CommandLineProgress.cpp",
        "src/utl/src/CommandLineProgress.h",
        "src/utl/src/Logger.cpp",
        "src/utl/src/LoggerCommon.cpp",
        "src/utl/src/LoggerCommon.h",
        "src/utl/src/Metrics.cpp",
        "src/utl/src/Progress.cpp",
        "src/utl/src/prometheus/metrics_server.cpp",
    ],
    hdrs = glob(
        [
            "src/odb/include/odb/*.h",
            "src/odb/include/odb/*.hpp",
            "src/odb/include/odb/*.inc",
            "src/utl/include/utl/*.h",
            "src/utl/include/utl/prometheus/*.h",
        ],
        allow_empty = True,
    ),
    # Snapshots are frozen source compiled by a newer toolchain than they
    # were written for, so the repo-wide -Werror set in //.bazelrc does not
    # apply to them. A warning here is not actionable: the fix would have to
    # be a patch against an immutable revision.
    copts = ["-Wno-error"],
    features = [
        "-layering_check",
        "-use_header_modules",
    ],
    includes = [
        "src/odb/include",
        "src/utl/include",
        "src/utl/src",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@boost.algorithm",
        "@boost.asio",
        "@boost.beast",
        "@boost.bind",
        "@boost.config",
        "@boost.container",
        "@boost.fusion",
        "@boost.geometry",
        "@boost.integer",
        "@boost.iostreams",
        "@boost.iterator",
        "@boost.lambda",
        "@boost.multi_array",
        "@boost.optional",
        "@boost.phoenix",
        "@boost.polygon",
        "@boost.property_tree",
        "@boost.random",
        "@boost.regex",
        "@boost.spirit",
        "@spdlog",
        "@zlib",
    ],
)
