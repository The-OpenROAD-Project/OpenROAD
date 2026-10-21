# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@aspect_bazel_lib//lib:write_source_files.bzl", "write_source_files")

# The CMake build's copy of //src/web's bundle (see README.md); regenerate
# everything but this file and README.md with: bazelisk run //src/web/dist:dist
write_source_files(
    name = "dist",
    files = {
        f: "//src/web:bundle/" + f
        for f in [
            "THIRD_PARTY_LICENSES.txt",
            "app.min.js",
            "index.html",
            "report.min.js",
        ]
    },
)
