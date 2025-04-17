# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, Precision Innovations Inc.

load("//:bazel/tcl_encode_or.bzl", "tcl_encode")
load("//:bazel/tcl_wrap_cc.bzl", "tcl_wrap_cc")

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

cc_library(
    name = "odb",
    srcs = glob([
        "src/cdl/*.cpp",
        "src/db/*.cpp",
        "src/db/*.h",
        "src/db/*.hpp",
        "src/zutil/*.cpp",
        "src/defout/*.cpp",
        "src/defout/*.h",
        "src/defin/*.cpp",
        "src/defin/*.h",
        "src/lefin/*.cpp",
        "src/lefin/*.h",
        "src/lefout/*.cpp",
        "src/swig/tcl/MakeOdb.cpp",
        "src/swig/common/swig_common.cpp",
        "src/swig/common/swig_common.h",
    ]),
    hdrs = glob([
        "include/odb/*.h",
        "include/odb/*.hpp",
    ]),
    copts = [
        "-fexceptions",
        "-Wno-error",
    ],
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
        "//src/utl",
        "@boost.algorithm",
        "@boost.bind",
        "@boost.config",
        "@boost.fusion",
        "@boost.geometry",
        "@boost.lambda",
        "@boost.optional",
        "@boost.phoenix",
        "@boost.polygon",
        "@boost.property_tree",
        "@boost.regex",
        "@boost.spirit",
        "@spdlog",
        "@tk_tcl//:tcl",
        "@zlib",
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
