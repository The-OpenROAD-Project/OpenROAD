# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, Precision Innovations Inc.

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_python//python:defs.bzl", "py_library")
load("//bazel:python_wrap_cc.bzl", "PYTHON_EXTENSION_LINKOPTS", "PYTHON_STABLE_API_DEFINE", "python_wrap_cc")
load("//test:regression.bzl", "messages_txt")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

# Public headers live physically in src/odb/include/odb/ (preserving the
# CMake layout). They are exported so each per-subdir Bazel package can
# reference the headers it owns via cross-package labels.
exports_files(
    glob(
        include = ["include/odb/*.h"],
    ) + [
        "include/odb/dbCompare.inc",
        "include/odb/dbMap.inc",
    ],
)

# Filegroup for SWIG rules that need the full set of public headers as
# input (they cannot glob across package boundaries).
filegroup(
    name = "odb_public_headers",
    srcs = glob(["include/odb/*.h"]),
)

# Umbrella target that aggregates every per-subdirectory odb library.
# Downstream BUILD files depend on //src/odb:odb to keep working
# unchanged after the split of the old monolith into per-subdirectory
# targets.
cc_library(
    name = "odb",
    # The umbrella target republishes the public odb/*.h headers so that
    # downstream targets that depend on //src/odb (and use
    # layering_check) continue to see odb/*.h through a direct dep.
    hdrs = glob(
        include = ["include/odb/*.h"],
        exclude = ["include/odb/MakeOdb.h"],
    ) + [
        "include/odb/dbCompare.inc",
        "include/odb/dbMap.inc",
    ],
    includes = ["include"],
    deps = [
        "//src/odb/src/3dblox",
        "//src/odb/src/cdl",
        "//src/odb/src/db",
        "//src/odb/src/def",
        "//src/odb/src/def:defzlib",
        "//src/odb/src/defin",
        "//src/odb/src/defout",
        "//src/odb/src/gdsin",
        "//src/odb/src/gdsout",
        "//src/odb/src/lef",
        "//src/odb/src/lef:lefzlib",
        "//src/odb/src/lefin",
        "//src/odb/src/lefout",
        "//src/odb/src/swig:swig_common",
        "//src/utl",
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/synchronization",
        "@boost.container",
        "@boost.geometry",
        "@boost.multi_array",
        "@boost.polygon",
    ],
)

# ---- SWIG / Python bindings ----
#
# The Python targets (swig-py, _odb.so, odb_py) stay in this package
# because the generated odb.py import path encodes the Bazel package
# path (src.odb.odb); moving them would break every downstream Python
# consumer. The TCL SWIG, tcl_encode, swig_common, and ui targets
# live in //src/odb/src/swig.

python_wrap_cc(
    name = "swig-py",
    srcs = glob([
        "include/odb/*.h",
    ]) + [
        "//:src/Design.i",
        "//:src/Exception.i",
        "//src/odb/src/swig:swig_imports_py",
    ],
    module = "odb",
    root_swig_src = "//src/odb/src/swig:common/odb.i",
    swig_includes = [
        "include",
        "src/swig/common",
        "src/swig/python",
    ],
    swig_options = [
        "-w509,503,501,472,467,402,401,317,325,378,383,389,365,362,314,258,240,203,201",
    ],
)

cc_binary(
    name = "_odb.so",
    srcs = [":swig-py"],
    defines = [PYTHON_STABLE_API_DEFINE],
    includes = ["src/swig/common"],
    linkopts = PYTHON_EXTENSION_LINKOPTS,
    linkshared = True,
    deps = [
        ":odb",
        "//src/odb/src/swig:swig_common",
        "@rules_python//python/cc:current_py_cc_headers",
    ],
)

py_library(
    name = "odb_py",
    srcs = [":swig-py"],
    data = [":_odb.so"],
    imports = ["."],
    visibility = ["//visibility:public"],
)

# ---- Aliases for targets that moved to //src/odb/src/swig ----

alias(
    name = "swig",
    actual = "//src/odb/src/swig",
)

alias(
    name = "ui",
    actual = "//src/odb/src/swig:ui",
)

alias(
    name = "tcl",
    actual = "//src/odb/src/swig:tcl",
)

filegroup(
    name = "doc_files",
    srcs = [
        "README.md",
        "//src/odb/src/swig:tcl/odb.tcl",
    ],
    visibility = ["//src/odb/test:__pkg__"],
)

messages_txt(
    extra_srcs = [
        "//src/odb/src/3dblox:message_srcs",
        "//src/odb/src/cdl:message_srcs",
        "//src/odb/src/db:message_srcs",
        "//src/odb/src/defin:message_srcs",
        "//src/odb/src/defout:message_srcs",
        "//src/odb/src/gdsin:message_srcs",
        "//src/odb/src/gdsout:message_srcs",
        "//src/odb/src/lefin:message_srcs",
        "//src/odb/src/lefout:message_srcs",
        "//src/odb/src/swig:message_srcs",
    ],
    src_patterns = [
        "include/odb/*.h",
    ],
    visibility = ["//src/odb/test:__pkg__"],
)
