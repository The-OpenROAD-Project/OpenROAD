# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

# TODO: this file should eventually be empty. Right now it still pulls in a few
# dependencies via bazel_rules_hdl.

workspace(name = "openroad")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

rules_hdl_git_hash = "8cc8977cc305ed94ec7852495ed576fcbde1c18d"

rules_hdl_git_sha256 = "046193f4a0b006f43bd5f9615c218d2171d0169e231028a22d8d9c011c675ad6"

http_archive(
    name = "rules_hdl",
    sha256 = rules_hdl_git_sha256,
    strip_prefix = "bazel_rules_hdl-%s" % rules_hdl_git_hash,
    urls = [
        "https://github.com/hdl/bazel_rules_hdl/archive/%s.tar.gz" % rules_hdl_git_hash,
    ],
)

# Direct dependencies needed in openroad, and others that these in turn need.
# This essentially reads as a TODO list of what needs to be upstreamed to BCR
load("@rules_hdl//dependency_support/com_github_westes_flex:com_github_westes_flex.bzl", "com_github_westes_flex")
load("@rules_hdl//dependency_support/net_zlib:net_zlib.bzl", "net_zlib")
load("@rules_hdl//dependency_support/org_gnu_bison:org_gnu_bison.bzl", "org_gnu_bison")
load("@rules_hdl//dependency_support/org_pcre_ftp:org_pcre_ftp.bzl", "org_pcre_ftp")
load("@rules_hdl//dependency_support/org_swig:org_swig.bzl", "org_swig")
load("@rules_hdl//dependency_support/tk_tcl:tk_tcl.bzl", "tk_tcl")

tk_tcl()

# Swig exists in BCR, but in a newer version where we need to test how to make
# it to work with TCL.
org_swig()

# rules_flex and rules_bison already exist in BCR, but we use rules_hdl wrappers
# around them. We should use the rules flex/bison-provided ones.
com_github_westes_flex()

org_gnu_bison()

# remaining user is tcl. Once we can get that from BCR, this can go.
net_zlib()  # BCR has @zlib we use, but some above dep uses it w/ differet name

org_pcre_ftp()  # there is a newer pcre2 in BCR
