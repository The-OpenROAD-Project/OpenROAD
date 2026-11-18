# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, Precision Innovations Inc.

load("//bazel:python_wrap_cc.bzl", "python_wrap_cc", "python_wrap_library")
load("//test:regression.bzl", "messages_txt")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

# Public headers live physically in openroad/odb/include/odb/. They are exported
# so each per-subdir Bazel package can reference the headers it owns via
# cross-package labels.
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

# ---- SWIG / Python bindings ----
#
# The Python targets (swig-py, py_wrap, odb_py) stay in this package
# because the generated odb_py.py import path encodes the Bazel package
# path (openroad.odb.odb_py), which other generated wrappers import; moving
# them would break every downstream Python consumer. The TCL SWIG, tcl_encode, swig_common, and ui targets
# live in //openroad/odb/src/swig.

python_wrap_cc(
    name = "swig-py",
    srcs = glob([
        "include/odb/*.h",
    ]) + [
        "//:openroad/Design.i",
        "//:openroad/Exception.i",
        "//openroad/odb/src/swig:swig_imports_py",
    ],
    module = "odb_py",
    root_swig_src = "//openroad/odb/src/swig:common/odb.i",
    swig_includes = [
        "include",
        "src/swig/common",
        "src/swig/python",
    ],
    swig_options = [
        "-w509,503,501,472,467,402,401,317,325,378,383,389,365,362,314,258,240,203,201",
    ],
)

# The odb module is part of //:openroad_py.
alias(
    name = "odb_py",
    actual = "//:openroad_py",
    visibility = ["//visibility:public"],
)

# ---- Aliases for targets that moved to //openroad/odb/src/swig ----

alias(
    name = "swig",
    actual = "//openroad/odb/src/swig",
)

alias(
    name = "ui",
    actual = "//openroad/odb/src/swig:ui",
)

alias(
    name = "tcl",
    actual = "//openroad/odb/src/swig:tcl",
)

filegroup(
    name = "doc_files",
    srcs = [
        "README.md",
        "//openroad/odb/src/swig:tcl/odb.tcl",
    ],
    visibility = ["//openroad/odb/test:__pkg__"],
)

# Every directory under src/ is its own package, so the sources come in through
# extra_srcs and src_patterns only has to cover the public headers.
messages_txt(
    extra_srcs = [
        "//openroad/odb/src/3dblox:message_srcs",
        "//openroad/odb/src/cdl:message_srcs",
        "//openroad/odb/src/db:message_srcs",
        "//openroad/odb/src/defin:message_srcs",
        "//openroad/odb/src/defout:message_srcs",
        "//openroad/odb/src/gdsin:message_srcs",
        "//openroad/odb/src/gdsout:message_srcs",
        "//openroad/odb/src/lefin:message_srcs",
        "//openroad/odb/src/lefout:message_srcs",
        "//openroad/odb/src/swig:message_srcs",
        "//openroad/odb/src/wireNormalizer:message_srcs",
    ],
    src_patterns = [
        "include/odb/*.h",
    ],
    visibility = [
        "//docs:__pkg__",
        "//openroad/odb/test:__pkg__",
    ],
)

python_wrap_library(
    name = "py_wrap",
    includes = ["src/swig/common"],
    local_defines = [],
    deps = ["//openroad/odb/src/swig:swig_common"],
)
