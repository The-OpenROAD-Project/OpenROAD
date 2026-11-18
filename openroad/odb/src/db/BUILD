# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, Precision Innovations Inc.

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//test:regression.bzl", "message_srcs")

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
        "//openroad/odb:include/odb/PtrSetMap.h",
        "//openroad/odb:include/odb/db.h",
        "//openroad/odb:include/odb/dbBlockCallBackObj.h",
        "//openroad/odb:include/odb/dbBlockSet.h",
        "//openroad/odb:include/odb/dbCCSegSet.h",
        "//openroad/odb:include/odb/dbChipCallBackObj.h",
        "//openroad/odb:include/odb/dbCompare.inc",
        "//openroad/odb:include/odb/dbDatabaseObserver.h",
        "//openroad/odb:include/odb/dbExtControl.h",
        "//openroad/odb:include/odb/dbId.h",
        "//openroad/odb:include/odb/dbIterator.h",
        "//openroad/odb:include/odb/dbMap.h",
        "//openroad/odb:include/odb/dbMap.inc",
        "//openroad/odb:include/odb/dbMatrix.h",
        "//openroad/odb:include/odb/dbNetSet.h",
        "//openroad/odb:include/odb/dbObject.h",
        "//openroad/odb:include/odb/dbSet.h",
        "//openroad/odb:include/odb/dbShape.h",
        "//openroad/odb:include/odb/dbStream.h",
        "//openroad/odb:include/odb/dbTransform.h",
        "//openroad/odb:include/odb/dbTypes.h",
        "//openroad/odb:include/odb/dbUtil.h",
        "//openroad/odb:include/odb/dbViaParams.h",
        "//openroad/odb:include/odb/dbWireCodec.h",
        "//openroad/odb:include/odb/dbWireGraph.h",
        "//openroad/odb:include/odb/geom.h",
        "//openroad/odb:include/odb/geom_boost.h",
        "//openroad/odb:include/odb/isotropy.h",
        "//openroad/odb:include/odb/odbDList.h",
        "//openroad/odb:include/odb/poly_decomp.h",
        "//openroad/odb:include/odb/util.h",
    ],
    includes = ["../../include"],
    deps = [
        "//openroad/utl",
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

message_srcs(
    visibility = ["//openroad/odb:__pkg__"],
)
