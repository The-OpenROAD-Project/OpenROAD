# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, The OpenROAD Authors

# .odb schema converters.
#
# OpenROAD reads databases written by older OpenROAD revisions. Keeping that
# ability inside odb means every schema bump leaves a compatibility branch
# behind forever. Instead, each supported older schema range gets a small
# binary built from the OpenROAD source archive that already knew how to read
# it (see the "ODB schema snapshots" section of //MODULE.bazel), and openroad
# runs those binaries when it meets a file it is too new to parse itself.
#
# Adding support for an older range is a snapshot entry plus a target here.
# Nothing in this directory needs revisiting when the schema changes.

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_shell//shell:sh_test.bzl", "sh_test")

package(features = ["layering_check"])

# Reads schema 57..119, writes 119. Built from OpenROAD at 88bfc9fde1
# (2025-09-22), whose own current revision was 119.
cc_binary(
    name = "convert-57-119",
    srcs = ["odb_convert_main.cpp"],
    # Converters are dropped next to the openroad binary and must run there
    # with nothing else installed. fully_static_link is not available -- the
    # hermetic toolchain carries no libc.a -- but linkstatic plus the
    # toolchain's static libc++ already leaves only glibc, which is exactly
    # what openroad itself needs. converter_deps_test locks that down.
    linkstatic = True,
    visibility = ["//visibility:public"],
    deps = [
        "@boost.iostreams",
        "@odb_snapshot_119//:odb_snapshot",
    ],
)

# A converter is useless if it cannot be dropped next to openroad, so assert
# the shared-library set rather than trusting the link flags.
sh_test(
    name = "converter_deps_test",
    size = "small",
    srcs = ["converter_deps_test.sh"],
    data = [":convert-57-119"],
)

# The real contract: an old database goes in, a database the current
# OpenROAD can read comes out. Checked against files already in the tree at
# schema 57 and 98.
sh_test(
    name = "convert_roundtrip_test",
    size = "small",
    srcs = ["convert_roundtrip_test.sh"],
    data = [
        ":convert-57-119",
        "//src/odb/test:design_odb_data",
    ],
)
