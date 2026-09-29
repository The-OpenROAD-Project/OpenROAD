# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@aspect_bazel_lib//lib:write_source_files.bzl", "write_source_files")

package(default_visibility = ["//src/web:__subpackages__"])

# A copy of //src/web's bundles for the CMake build, which cannot run the
# bundler.  Regenerate everything but this file and README.md with:
#
#     bazel run //src/web/dist:dist
_BUNDLES = [
    "SOURCES.sha256",
    "THIRD_PARTY_LICENSES.txt",
    "app.min.js",
    "index.min.html",
    "report.min.js",
]

write_source_files(
    name = "dist",
    files = {f: "//src/web:" + f for f in _BUNDLES},
)
