# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:python_wrap_cc.bzl", "PYTHON_STABLE_API_DEFINE", "python_wrap_cc")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")
load("//test:regression.bzl", "messages_txt")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "fin",
    srcs = [
        "src/DensityFill.cpp",
        "src/DensityFill.h",
        "src/Finale.cpp",
        "src/MakeFinale.cpp",
        "src/graphics.cpp",
        "src/graphics.h",
        "src/polygon.h",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/fin/Finale.h",
        "include/fin/MakeFinale.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//:ord",
        "//src/odb/src/db",
        "//src/utl",
        "//src/web:core",
        "@boost.lexical_cast",
        "@boost.polygon",
        "@boost.property_tree",
        "@boost.stacktrace",
        "@tcl_lang//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/finale.tcl",
    ],
    char_array_name = "fin_tcl_inits",
    namespace = "fin",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/finale.i",
        "//:error_swig",
    ],
    module = "fin",
    namespace_prefix = "fin",
    root_swig_src = "src/finale.i",
    swig_includes = [
        "src/fin/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)

python_wrap_cc(
    name = "swig-py",
    srcs = [
        "include/fin/Finale.h",
        "src/finale-py.i",
        "//:error_swig-py",
    ],
    module = "fin_py",
    root_swig_src = "src/finale-py.i",
    swig_includes = [
        "include",
    ],
    deps = [
        "//src/odb:swig-py",
    ],
)

filegroup(
    name = "doc_files",
    srcs = [
        "README.md",
        "src/finale.tcl",
    ],
    visibility = ["//src/fin/test:__pkg__"],
)

messages_txt(
    visibility = [
        "//docs:__pkg__",
        "//src/fin/test:__pkg__",
    ],
)

# Compiled into //:_openroad.so, the one library every OpenROAD Python
# module is loaded from.
cc_library(
    name = "py_wrap",
    srcs = [":swig-py"],
    includes = ["src"],
    local_defines = [
        "BAZEL=1",
        PYTHON_STABLE_API_DEFINE,
    ],
    visibility = ["//:__pkg__"],
    deps = [
        ":fin",
        "//:ord",
        "//src/odb/src/db",
        "//src/utl",
        "@boost.stacktrace",
        "@rules_python//python/cc:current_py_cc_headers",
    ],
    alwayslink = True,
)
