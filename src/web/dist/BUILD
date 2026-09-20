# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@aspect_bazel_lib//lib:write_source_files.bzl", "write_source_files")

package(default_visibility = ["//src/web:__subpackages__"])

# The checked-in output of the browser bundling in //src/web.
#
# The viewer's libraries come from npm and are bundled and minified by esbuild,
# which only Bazel can drive -- but the CMake build embeds the same assets, and
# `cmake -B build` has to keep working on its own.  So the blobs are checked in
# and both builds read them from here, the way both builds read src/web.tcl.
#
# Everything in this directory is generated.  To refresh it after changing
# anything under src/web/src or bumping a dependency in src/web/package.json:
#
#     bazel run //src/web/dist:dist
#
# The test this rule generates is what fails when someone forgets.
_BUNDLES = [
    "app.min.css",
    "app.min.js",
    "gl-dark.min.css",
    "gl-light.min.css",
    "index.min.html",
    "report.min.js",
    "vendor.min.css",
]

write_source_files(
    name = "dist",
    files = {f: "//src/web:" + f for f in _BUNDLES},
)

exports_files(_BUNDLES)
