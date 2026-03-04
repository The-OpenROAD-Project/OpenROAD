# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:bison.bzl", "genyacc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

# TODO: This can probably be broken into smaller, more specific parts.
# TODO: once we establish src/odb as package, make aliases so that we
#  don't need the awkward long path with double src
#  //src/odb/src/lef:lef but can say //src/odb:lef
cc_library(
    name = "lef",
    srcs = [
        "lef/crypt.cpp",
        "lef/lef_keywords.cpp",
        "lef/lef_parser.cpp",
        "lef/lefiArray.cpp",
        "lef/lefiCrossTalk.cpp",
        "lef/lefiDebug.cpp",
        "lef/lefiEncryptInt.cpp",
        "lef/lefiLayer.cpp",
        "lef/lefiMacro.cpp",
        "lef/lefiMisc.cpp",
        "lef/lefiNonDefault.cpp",
        "lef/lefiProp.cpp",
        "lef/lefiPropType.cpp",
        "lef/lefiTBExt.cpp",
        "lef/lefiUnits.cpp",
        "lef/lefiVia.cpp",
        "lef/lefiViaRule.cpp",
        "lef/lefrCallbacks.cpp",
        "lef/lefrData.cpp",
        "lef/lefrReader.cpp",
        "lef/lefrSettings.cpp",
        "lef/lefwWriter.cpp",
        "lef/lefwWriterCalls.cpp",
    ],
    hdrs = [
        "lef/crypt.hpp",
        "lef/lef_parser.hpp",
        "lef/lefiArray.hpp",
        "lef/lefiCrossTalk.hpp",
        "lef/lefiDebug.hpp",
        "lef/lefiDefs.hpp",
        "lef/lefiEncryptInt.hpp",
        "lef/lefiKRDefs.hpp",
        "lef/lefiLayer.hpp",
        "lef/lefiMacro.hpp",
        "lef/lefiMisc.hpp",
        "lef/lefiNonDefault.hpp",
        "lef/lefiProp.hpp",
        "lef/lefiPropType.hpp",
        "lef/lefiUnits.hpp",
        "lef/lefiUser.hpp",
        "lef/lefiUtil.hpp",
        "lef/lefiVia.hpp",
        "lef/lefiViaRule.hpp",
        "lef/lefrCallBacks.hpp",
        "lef/lefrData.hpp",
        "lef/lefrReader.hpp",
        "lef/lefrSettings.hpp",
        "lef/lefwWriter.hpp",
        "lef/lefwWriterCalls.hpp",
        "lef/lex.h",
    ],
    includes = ["lef"],
)

genyacc(
    name = "lef_bison",
    src = "lef/lef.y",
    header_out = "lef/lef_parser.hpp",
    prefix = "lefyy",
    source_out = "lef/lef_parser.cpp",
)

cc_library(
    name = "lefzlib",
    srcs = [
        "lefzlib/lefzlib.cpp",
    ],
    hdrs = [
        "lefzlib/lefzlib.hpp",
    ],
    includes = ["lefzlib"],
    deps = [
        ":lef",
        "@zlib",
    ],
)
