# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, Precision Innovations Inc.

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_python//python:defs.bzl", "py_library")
load("//bazel:python_wrap_cc.bzl", "PYTHON_EXTENSION_LINKOPTS", "PYTHON_STABLE_API_DEFINE", "python_wrap_cc")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")
load("//test:regression.bzl", "messages_txt")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

# Module-specific public headers whose implementation lives in a single
# src/<module>/ subdirectory. These are exposed to the matching subpackage
# via exports_files so each subdir cc_library can republish its own public
# header as hdrs, making //src/odb:<module> the canonical owner of
# odb/<module>.h. Kept exported (not moved) so the CMake build, which
# expects all public headers under src/odb/include/odb/, continues to work.
exports_files(
    ["include/odb/3dblox.h"],
    visibility = ["//src/odb/src/3dblox:__pkg__"],
)

exports_files(
    ["include/odb/cdl.h"],
    visibility = ["//src/odb/src/cdl:__pkg__"],
)

exports_files(
    ["include/odb/defin.h"],
    visibility = ["//src/odb/src/defin:__pkg__"],
)

exports_files(
    ["include/odb/defout.h"],
    visibility = ["//src/odb/src/defout:__pkg__"],
)

exports_files(
    [
        "include/odb/gdsUtil.h",
        "include/odb/gdsin.h",
    ],
    visibility = ["//src/odb/src/gdsin:__pkg__"],
)

exports_files(
    ["include/odb/gdsout.h"],
    visibility = ["//src/odb/src/gdsout:__pkg__"],
)

exports_files(
    ["include/odb/lefin.h"],
    visibility = ["//src/odb/src/lefin:__pkg__"],
)

exports_files(
    ["include/odb/lefout.h"],
    visibility = ["//src/odb/src/lefout:__pkg__"],
)

exports_files(
    [
        "include/odb/poly_decomp.h",
        "include/odb/util.h",
    ],
    visibility = ["//src/odb/src/db:__pkg__"],
)

cc_library(
    name = "headers",
    hdrs = glob(
        include = ["include/odb/*.h"],
        exclude = [
            # Not part of any library target; built into //:ord separately.
            "include/odb/MakeOdb.h",
            # Module-specific headers owned by their subdir library.
            "include/odb/3dblox.h",
            "include/odb/cdl.h",
            "include/odb/defin.h",
            "include/odb/defout.h",
            "include/odb/gdsUtil.h",
            "include/odb/gdsin.h",
            "include/odb/gdsout.h",
            "include/odb/lefin.h",
            "include/odb/lefout.h",
            "include/odb/poly_decomp.h",
            "include/odb/util.h",
        ],
    ) + [
        "include/odb/dbCompare.inc",
        "include/odb/dbMap.inc",
    ],
    features = [
        "-use_header_modules",
    ],
    includes = ["include"],
    deps = [
        "//src/utl",
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/synchronization",
        "@boost.container",
        "@boost.geometry",
        "@boost.multi_array",
        "@boost.polygon",
    ],
)

filegroup(
    name = "swig_imports",
    srcs = glob([
        "src/swig/tcl/*.i",
        "src/swig/common/*.i",
    ]),
)

filegroup(
    name = "swig_imports_py",
    srcs = glob([
        "src/swig/python/*.i",
        "src/swig/common/*.i",
    ]),
)

# Umbrella target that aggregates every per-subdirectory odb library and the
# shared SWIG glue (swig_common). Downstream BUILD files depend on
# //src/odb:odb to keep working unchanged after the split of the old
# monolith into per-subdirectory targets.
cc_library(
    name = "odb",
    srcs = ["src/swig/common/swig_common.cpp"],
    # The umbrella target republishes the public odb/*.h headers so that
    # downstream targets that depend on //src/odb (and use
    # layering_check) continue to see odb/*.h through a direct dep.
    # The per-subdir libraries depend on //src/odb:headers directly.
    hdrs = glob(
        include = ["include/odb/*.h"],
        exclude = ["include/odb/MakeOdb.h"],
    ) + [
        "include/odb/dbCompare.inc",
        "include/odb/dbMap.inc",
        "src/swig/common/swig_common.h",
    ],
    features = [
        "-use_header_modules",
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
        "//src/utl",
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/synchronization",
        "@boost.container",
        "@boost.geometry",
        "@boost.multi_array",
        "@boost.polygon",
    ],
)

# This target compiles the SWIG C++ wrapper into a shared library (.so)
# that Python can load dynamically.
cc_binary(
    name = "_odb.so",
    srcs = [":swig-py"],
    defines = [PYTHON_STABLE_API_DEFINE],
    includes = ["src/swig/common"],
    linkopts = PYTHON_EXTENSION_LINKOPTS,
    linkshared = True,
    deps = [
        ":odb",  # Depends on the core odb C++ library
        "@rules_python//python/cc:current_py_cc_headers",
    ],
)

# This packages the SWIG-generated Python wrapper (odb.py) and the
# compiled C++ extension (_odb.so) together.
py_library(
    name = "odb_py",
    srcs = [":swig-py"],  # Use the .py output from the swig-py rule
    # The data attribute makes the .so file available at runtime.
    data = [":_odb.so"],
    # This allows imports relative to the workspace root.
    imports = ["."],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "ui",
    srcs = [
        "include/odb/geom.h",
        "src/swig/common/swig_common.h",
        "src/swig/tcl/MakeOdb.cpp",
        ":swig",
        ":tcl",
    ],
    hdrs = ["include/odb/MakeOdb.h"],
    copts = [
        "-Wno-missing-braces",  # from TCL swigging
    ],
    features = [
        "-use_header_modules",
    ],
    includes = [
        "include",
        "src/swig/common",
    ],
    deps = [
        ":odb",
        "//:ord",
        "//src/utl",
        "@abseil-cpp//absl/synchronization",
        "@boost.container",
        "@boost.geometry",
        "@boost.multi_array",
        "@boost.polygon",
        "@boost.stacktrace",
        "@tcl_lang//:tcl",
    ],
)

tcl_wrap_cc(
    name = "swig",
    srcs = glob([
        "include/odb/*.h",
    ]) + [
        ":swig_imports",
        "//:src/Design.i",
        "//:src/Exception.i",
    ],
    module = "odbtcl",
    namespace_prefix = "odb",
    root_swig_src = "src/swig/common/odb.i",
    swig_includes = [
        "include",
        "src/swig/common",
        "src/swig/tcl",
    ],
    swig_options = [
        # These values are derived from the CMakeList.txt and represent the "rules" this swig file
        # breaks. Swig refuses to compile our swig files unless we acknowledge we are ignoring the
        # following warnings. They can be derived by attemtpting to compile without them and fixing
        # the warnings one by one.
        "-w509,503,501,472,467,402,401,317,325,378,383,389,365,362,314,258,240,203,201",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/swig/tcl/odb.tcl",
    ],
    char_array_name = "odbtcl_tcl_inits",
    namespace = "odb",
)

python_wrap_cc(
    name = "swig-py",
    srcs = glob([
        "include/odb/*.h",
    ]) + [
        ":swig_imports_py",
        "//:src/Design.i",
        "//:src/Exception.i",
    ],
    module = "odb",
    root_swig_src = "src/swig/common/odb.i",
    swig_includes = [
        "include",
        "src/swig/common",
        "src/swig/python",
    ],
    swig_options = [
        # These values are derived from the CMakeList.txt and represent the "rules" this swig file
        # breaks. Swig refuses to compile our swig files unless we acknowledge we are ignoring the
        # following warnings. They can be derived by attemtpting to compile without them and fixing
        # the warnings one by one.
        "-w509,503,501,472,467,402,401,317,325,378,383,389,365,362,314,258,240,203,201",
    ],
)

filegroup(
    name = "doc_files",
    srcs = [
        "README.md",
        "src/swig/tcl/odb.tcl",
    ],
    visibility = ["//src/odb/test:__pkg__"],
)

messages_txt(
    src_patterns = [
        "src/**/*.cc",
        "src/**/*.cpp",
        "src/**/*.h",
        "src/**/*.hh",
        "src/**/*.tcl",
    ],
    visibility = ["//src/odb/test:__pkg__"],
)
