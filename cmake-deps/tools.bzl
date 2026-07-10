# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Pinned official cmake and ninja release binaries for the deps bundle.

Host cmake is traditionally too old for OpenROAD; the bundle ships its
own, so a plain-CMake build needs no host toolchain packages at all.
Linux x86_64 only, like the rest of the bundle.
"""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

CMAKE_VERSION = "3.31.9"
NINJA_VERSION = "1.12.1"

def _cmake_tools_impl(_mctx):
    http_archive(
        name = "cmake-linux-x86_64",
        urls = [
            "https://github.com/Kitware/CMake/releases/download/v{v}/cmake-{v}-linux-x86_64.tar.gz".format(v = CMAKE_VERSION),
        ],
        sha256 = "312d78e0b7c5c9b4a97a87a46c3e525aa84895fe8135c238c4616bba73dc9518",
        strip_prefix = "cmake-{v}-linux-x86_64".format(v = CMAKE_VERSION),
        build_file_content = """
filegroup(
    name = "all_files",
    srcs = glob(["bin/*", "share/cmake-*/**"]),
    visibility = ["//visibility:public"],
)
""",
    )
    http_archive(
        name = "ninja-linux-x86_64",
        urls = [
            "https://github.com/ninja-build/ninja/releases/download/v{v}/ninja-linux.zip".format(v = NINJA_VERSION),
        ],
        sha256 = "6f98805688d19672bd699fbbfa2c2cf0fc054ac3df1f0e6a47664d963d530255",
        build_file_content = """
filegroup(
    name = "ninja_bin",
    srcs = ["ninja"],
    visibility = ["//visibility:public"],
)
""",
    )

cmake_tools = module_extension(
    implementation = _cmake_tools_impl,
)
