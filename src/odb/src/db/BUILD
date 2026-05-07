# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, Precision Innovations Inc.

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "db",
    srcs = glob([
        "*.cpp",
        "*.h",
        "*.hpp",
        "*.inc",
    ]),
    hdrs = [
        "//src/odb:include/odb/db.h",
        "//src/odb:include/odb/dbBlockCallBackObj.h",
        "//src/odb:include/odb/dbBlockSet.h",
        "//src/odb:include/odb/dbCCSegSet.h",
        "//src/odb:include/odb/dbChipCallBackObj.h",
        "//src/odb:include/odb/dbCompare.inc",
        "//src/odb:include/odb/dbDatabaseObserver.h",
        "//src/odb:include/odb/dbExtControl.h",
        "//src/odb:include/odb/dbId.h",
        "//src/odb:include/odb/dbIterator.h",
        "//src/odb:include/odb/dbMap.h",
        "//src/odb:include/odb/dbMap.inc",
        "//src/odb:include/odb/dbMatrix.h",
        "//src/odb:include/odb/dbNetSet.h",
        "//src/odb:include/odb/dbObject.h",
        "//src/odb:include/odb/dbSet.h",
        "//src/odb:include/odb/dbShape.h",
        "//src/odb:include/odb/dbStream.h",
        "//src/odb:include/odb/dbTransform.h",
        "//src/odb:include/odb/dbTypes.h",
        "//src/odb:include/odb/dbUtil.h",
        "//src/odb:include/odb/dbViaParams.h",
        "//src/odb:include/odb/dbWireCodec.h",
        "//src/odb:include/odb/dbWireGraph.h",
        "//src/odb:include/odb/geom.h",
        "//src/odb:include/odb/geom_boost.h",
        "//src/odb:include/odb/isotropy.h",
        "//src/odb:include/odb/odbDList.h",
        "//src/odb:include/odb/poly_decomp.h",
        "//src/odb:include/odb/unfoldedModel.h",
        "//src/odb:include/odb/util.h",
        "//src/odb:include/odb/wOrder.h",
    ],
    includes = ["../../include"],
    deps = [
        "//src/utl",
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/synchronization",
        "@boost.container",
        "@boost.geometry",
        "@boost.integer",
        "@boost.multi_array",
        "@boost.polygon",
        "@boost.property_tree",
        "@boost.regex",
        "@spdlog",
    ],
)

filegroup(
    name = "message_srcs",
    srcs = glob([
        "*.cpp",
        "*.h",
    ]),
    visibility = ["//src/odb:__pkg__"],
)
