# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

genrule(
    name = "post9",
    srcs = [
        "src/flt/etc/POST9.dat",
    ],
    outs = ["src/flt/etc/POST9.cpp"],
    cmd = "$(execpath //etc:file_to_string) --inputs $(location src/flt/etc/POST9.dat) --output \"$@\" --varname post9 --namespace stt::flt",
    tools = [
        "//etc:file_to_string",
    ],
)

genrule(
    name = "powv9",
    srcs = [
        "src/flt/etc/POWV9.dat",
    ],
    outs = ["src/flt/etc/POWV9.cpp"],
    cmd = "$(execpath //etc:file_to_string) --inputs $(location src/flt/etc/POWV9.dat) --output \"$@\" --varname powv9 --namespace stt::flt",
    tools = [
        "//etc:file_to_string",
    ],
)

cc_library(
    name = "stt",
    srcs = [
        "src/LinesRenderer.cpp",
        "src/LinesRenderer.h",
        "src/MakeSteinerTreeBuilder.cpp",
        "src/SteinerTreeBuilder.cpp",
        "src/flt/etc/POST9.cpp",
        "src/flt/etc/POWV9.cpp",
        "src/flt/flute.cpp",
        "src/pdr/src/pd.cpp",
    ],
    hdrs = [
        "include/stt/MakeSteinerTreeBuilder.h",
        "include/stt/SteinerTreeBuilder.h",
        "include/stt/flute.h",
        "include/stt/pd.h",
    ],
    includes = [
        "include",
        "src",
    ],
    deps = [
        "//:ord",
        "//src/gui",
        "//src/odb",
        "//src/utl",
        "@boost.heap",
        "@boost.multi_array",
        "@com_github_quantamhd_lemon//:lemon",
        "@tk_tcl//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/SteinerTreeBuilder.tcl",
    ],
    char_array_name = "stt_tcl_inits",
    namespace = "stt",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/SteinerTreeBuilder.i",
        "//:error_swig",
    ],
    module = "stt",
    namespace_prefix = "stt",
    root_swig_src = "src/SteinerTreeBuilder.i",
    swig_includes = [
        "src",
    ],
)
