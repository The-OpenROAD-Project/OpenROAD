# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

# TODO: this file should eventually be empty. Right now it still pulls in a few
# dependencies via bazel_rules_hdl.

workspace(name = "openroad")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

rules_hdl_git_hash = "4bfc8987e521f2002e7b898ba94d3df4c6204913"

rules_hdl_git_sha256 = "227ac0288299f2b0f31a188113cef9f733258398fd616215275bddab1e43d019"

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
load("@rules_hdl//dependency_support/com_github_quantamhd_lemon:com_github_quantamhd_lemon.bzl", "com_github_quantamhd_lemon")
load("@rules_hdl//dependency_support/com_github_westes_flex:com_github_westes_flex.bzl", "com_github_westes_flex")
load("@rules_hdl//dependency_support/edu_berkeley_abc:edu_berkeley_abc.bzl", "edu_berkeley_abc")
load("@rules_hdl//dependency_support/net_invisible_island_ncurses:net_invisible_island_ncurses.bzl", "net_invisible_island_ncurses")
load("@rules_hdl//dependency_support/net_zlib:net_zlib.bzl", "net_zlib")
load("@rules_hdl//dependency_support/org_gnu_bison:org_gnu_bison.bzl", "org_gnu_bison")
load("@rules_hdl//dependency_support/org_gnu_readline:org_gnu_readline.bzl", "org_gnu_readline")
load("@rules_hdl//dependency_support/org_llvm_openmp:org_llvm_openmp.bzl", "org_llvm_openmp")
load("@rules_hdl//dependency_support/org_pcre_ftp:org_pcre_ftp.bzl", "org_pcre_ftp")
load("@rules_hdl//dependency_support/org_swig:org_swig.bzl", "org_swig")
load("@rules_hdl//dependency_support/tk_tcl:tk_tcl.bzl", "tk_tcl")

# Direct dependencies needed in Openroad
com_github_quantamhd_lemon()

edu_berkeley_abc()

org_llvm_openmp()

tk_tcl()

# Swig exists in BCR, but in a newer version where we need to test how to make
# it to work with TCL.
org_swig()

# rules_flex and rules_bison already exist in BCR, but we use rules_hdl wrappers
# around them. We should use the rules flex/bison-provided ones.
com_github_westes_flex()

org_gnu_bison()

# secondary dependencies of the above libraries. Some of these are already
# in BCR with different name or sligtly newer API version.
net_invisible_island_ncurses()

net_zlib()  # BCR has @zlib we use, but some above dep uses it w/ differet name

org_gnu_readline()

org_pcre_ftp()  # there is a newer pcre2 in BCR
