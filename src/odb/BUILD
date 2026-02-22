# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, Precision Innovations Inc.

load("@openroad_rules_python//python:defs.bzl", "py_library")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:python_wrap_cc.bzl", "PYTHON_STABLE_API_DEFINE", "python_wrap_cc")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
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

cc_library(
    name = "odb",
    srcs = glob([
        "src/cdl/*.cpp",
        "src/db/*.cpp",
        "src/db/*.h",
        "src/db/*.hpp",
        "src/db/*.inc",
        "src/zutil/*.cpp",
        "src/defout/*.cpp",
        "src/defout/*.h",
        "src/defin/*.cpp",
        "src/defin/*.h",
        "src/gdsin/*.cpp",
        "src/gdsout/*.cpp",
        "src/lefin/*.cpp",
        "src/lefin/*.h",
        "src/lefout/*.cpp",
        "src/3dblox/*.cpp",
        "src/3dblox/*.h",
        "include/odb/dbCompare.inc",
        "include/odb/dbMap.inc",
    ] + [
        "src/swig/common/swig_common.cpp",
    ]),
    hdrs = glob(
        include = [
            "include/odb/*.h",
        ],
        exclude = ["include/odb/MakeOdb.h"],
    ) + ["src/swig/common/swig_common.h"],
    features = [
        "-use_header_modules",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//src/odb/src/def",
        "//src/odb/src/def:defzlib",
        "//src/odb/src/lef",
        "//src/odb/src/lef:lefzlib",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/synchronization",
        "@boost.algorithm",
        "@boost.bind",
        "@boost.config",
        "@boost.container",
        "@boost.fusion",
        "@boost.geometry",
        "@boost.integer",
        "@boost.iostreams",
        "@boost.lambda",
        "@boost.multi_array",
        "@boost.optional",
        "@boost.phoenix",
        "@boost.polygon",
        "@boost.property_tree",
        "@boost.regex",
        "@boost.spirit",
        "@spdlog",
        "@tcl_lang//:tcl",
        "@yaml-cpp",
        "@zlib",
    ],
)

# This target compiles the SWIG C++ wrapper into a shared library (.so)
# that Python can load dynamically.
cc_binary(
    name = "_odb.so",
    srcs = [":swig-py"],
    defines = [PYTHON_STABLE_API_DEFINE],
    includes = ["src/swig/common"],
    linkopts = select({
        "@platforms//os:macos": [
            "-undefined",
            "dynamic_lookup",
        ],
        "//conditions:default": [],
    }),
    linkshared = True,
    deps = [
        ":odb",  # Depends on the core odb C++ library
        "@openroad_rules_python//python/cc:current_py_cc_headers",
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
        "-Isrc/odb/src/swig/common",
        "-Isrc/odb/include",
    ],
    features = [
        "-use_header_modules",
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
