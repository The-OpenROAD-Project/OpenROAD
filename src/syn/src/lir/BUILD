# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

# L-IR: a hierarchical, word-level synthesis IR.

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//test:regression.bzl", "message_srcs")

package(
    default_visibility = ["//src/syn:__subpackages__"],
    features = ["layering_check"],
)

# Object id width: 32 bits by default, 64 with --define=lir_obj_id_bits=64
# (sets LIR_OBJ_ID_BITS=64). Run the tests with both widths.
config_setting(
    name = "obj_id_64",
    define_values = {"lir_obj_id_bits": "64"},
)

# IR core. Depends on absl, OpenSTA's Liberty model and utl; not on ODB.
cc_library(
    name = "lir",
    srcs = [
        "ConstPool.cc",
        "ConstView.cc",
        "NameInterner.cc",
        "Op.cc",
        "RegPinTypes.cc",
    ],
    hdrs = [
        "Checksum.h",
        "ConstPool.h",
        "ConstView.h",
        "Ids.h",
        "NameInterner.h",
        "ObjAttributes.h",
        "Op.h",
        "RegPinTypes.h",
        "SourceLoc.h",
    ],
    # In `defines` so that every dependent sees the same ObjIndex.
    defines = select({
        ":obj_id_64": ["LIR_OBJ_ID_BITS=64"],
        "//conditions:default": [],
    }),
    include_prefix = "syn/lir",
    strip_include_prefix = "//src/syn/src/lir",
    deps = [
        "//src/sta:opensta_lib",
        "//src/utl",
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/container:inlined_vector",
        "@abseil-cpp//absl/container:node_hash_map",
        "@abseil-cpp//absl/hash",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/synchronization",
    ],
)

message_srcs(
    visibility = ["//src/syn:__pkg__"],
)
