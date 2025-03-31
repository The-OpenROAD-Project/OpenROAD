###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2025, Precision Innovations Inc.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and#or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
###############################################################################

# TODO: this file should eventually be empty. Right now it still pulls in a few
# dependencies via bazel_rules_hdl.

workspace(name = "openroad")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

rules_hdl_git_hash = "22e4a4afaef17df38fffc124907144983f1e6492"
rules_hdl_git_sha256 = "e016d11b69c4eaaf2ac0757358859270db7a90f4aa725a1aee39e40f5b84efd3"

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
net_zlib() # BCR has @zlib we use, but some above dep uses it w/ differet name
org_gnu_readline()
org_pcre_ftp() # there is a newer pcre2 in BCR
