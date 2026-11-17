# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:bison.bzl", "genyacc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

DEF_HEADERS = [
    "def/defiAlias.hpp",
    "def/defiAssertion.hpp",
    "def/defiBlockage.hpp",
    "def/defiComponent.hpp",
    "def/defiDebug.hpp",
    "def/defiDefs.hpp",
    "def/defiFPC.hpp",
    "def/defiFill.hpp",
    "def/defiGroup.hpp",
    "def/defiIOTiming.hpp",
    "def/defiKRDefs.hpp",
    "def/defiMisc.hpp",
    "def/defiNet.hpp",
    "def/defiNonDefault.hpp",
    "def/defiPartition.hpp",
    "def/defiPath.hpp",
    "def/defiPinCap.hpp",
    "def/defiPinProp.hpp",
    "def/defiProp.hpp",
    "def/defiPropType.hpp",
    "def/defiRegion.hpp",
    "def/defiRowTrack.hpp",
    "def/defiScanchain.hpp",
    "def/defiSite.hpp",
    "def/defiSlot.hpp",
    "def/defiTimingDisable.hpp",
    "def/defiUser.hpp",
    "def/defiUtil.hpp",
    "def/defiVia.hpp",
    "def/defrCallBacks.hpp",
    "def/defrData.hpp",
    "def/defrReader.hpp",
    "def/defrSettings.hpp",
    "def/defwWriter.hpp",
    "def/defwWriterCalls.hpp",
]

cc_library(
    name = "def_bison_lib",
    srcs = [
        ":def_bison",
    ],
    hdrs = DEF_HEADERS + ["def/def_parser.hpp"],
    # Ignore warnings in generated code
    copts = [
        "-Wno-unused-but-set-variable",
        "-Wno-modules-import-nested-redundant",
    ],
    includes = ["def"],
)

# TODO: This can probably be broken into smaller, more specific parts.
# TODO: once we establish src/odb as package, make aliases so that we
#  don't need the awkward long path with double src
#  //src/odb/src/def:def but can say //src/odb:def
cc_library(
    name = "def",
    srcs = [
        "def/def_keywords.cpp",
        "def/defiAlias.cpp",
        "def/defiAssertion.cpp",
        "def/defiBlockage.cpp",
        "def/defiComponent.cpp",
        "def/defiDebug.cpp",
        "def/defiFPC.cpp",
        "def/defiFill.cpp",
        "def/defiGroup.cpp",
        "def/defiIOTiming.cpp",
        "def/defiMisc.cpp",
        "def/defiNet.cpp",
        "def/defiNonDefault.cpp",
        "def/defiPartition.cpp",
        "def/defiPath.cpp",
        "def/defiPinCap.cpp",
        "def/defiPinProp.cpp",
        "def/defiProp.cpp",
        "def/defiPropType.cpp",
        "def/defiRegion.cpp",
        "def/defiRowTrack.cpp",
        "def/defiScanchain.cpp",
        "def/defiSite.cpp",
        "def/defiSlot.cpp",
        "def/defiTimingDisable.cpp",
        "def/defiUtil.cpp",
        "def/defiVia.cpp",
        "def/defrCallbacks.cpp",
        "def/defrData.cpp",
        "def/defrReader.cpp",
        "def/defrSettings.cpp",
        "def/defwWriter.cpp",
        "def/defwWriterCalls.cpp",
    ],
    hdrs = DEF_HEADERS,
    includes = ["def"],
    deps = [
        ":def_bison_lib",
    ],
)

genyacc(
    name = "def_bison",
    src = "def/def.y",
    header_out = "def/def_parser.hpp",
    prefix = "defyy",
    source_out = "def/def_parser.cpp",
)

cc_library(
    name = "defzlib",
    srcs = [
        "defzlib/defzlib.cpp",
    ],
    hdrs = [
        "defzlib/defzlib.hpp",
    ],
    includes = ["defzlib"],
    deps = [
        ":def",
        ":def_bison_lib",
        "@zlib",
    ],
)
