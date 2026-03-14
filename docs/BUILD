# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

load("//bazel:man_pages.bzl", "man_pages")

man_pages(
    name = "man_pages",
    makefile = "Makefile",
    man1_srcs = glob(["md/man1/*.md"]),
    man2_srcs = [
        "//src/ant:README.md",
        "//src/cgt:README.md",
        "//src/cts:README.md",
        "//src/cut:README.md",
        "//src/dft:README.md",
        "//src/dpl:README.md",
        "//src/drt:README.md",
        "//src/est:README.md",
        "//src/exa:README.md",
        "//src/fin:README.md",
        "//src/gpl:README.md",
        "//src/grt:README.md",
        "//src/gui:README.md",
        "//src/ifp:README.md",
        "//src/mpl:README.md",
        "//src/odb:README.md",
        "//src/pad:README.md",
        "//src/par:README.md",
        "//src/pdn:README.md",
        "//src/ppl:README.md",
        "//src/psm:README.md",
        "//src/rcx:README.md",
        "//src/rmp:README.md",
        "//src/rsz:README.md",
        "//src/stt:README.md",
        "//src/tap:README.md",
        "//src/upf:README.md",
        "//src/utl:README.md",
    ],
    scripts = [
        "src/scripts/link_readmes.sh",
        "src/scripts/md_roff_compat.py",
        "src/scripts/manpage.py",
        "src/scripts/extract_utils.py",
    ],
    visibility = ["//visibility:public"],
)
