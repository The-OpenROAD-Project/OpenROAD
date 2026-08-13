# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Exposes the kepler-formal LEC binary to tests without a global flag change.

kepler-formal is invoked as a subprocess and never linked (GPL-3.0 vs
BSD-3-Clause). Building it here needs one accommodation: its @onetbb dependency
is a rules_foreign_cc `cmake()` target, and rules_foreign_cc puts the build's
`--cxxopt`s into CMAKE_CXX_FLAGS, which CMake reuses *at link time* ahead of the
object files. OpenROAD's global `-xc++` (.bazelrc) then makes clang parse .o
files as C++ source and the CMake compiler probe fails.

`-xc++` is inert for every OpenROAD compile (hermetic-llvm dispatches C++
compiles to clang++, which already defaults to C++ mode), so the flag is only
removed for this one subgraph rather than globally, keeping the change local to
the tests that need the tool.

There is no per-target escape hatch inside rules_foreign_cc: a user-supplied
CMAKE_CXX_FLAGS cache entry or CXXFLAGS env var is silently dropped
(cmake_script.bzl sets CMAKE_CXX_FLAGS_INIT from the toolchain with
replace = False). Hence a configuration transition.
"""

def _strip_xcxx_impl(settings, _attr):
    return {
        "//command_line_option:cxxopt": [
            opt
            for opt in settings["//command_line_option:cxxopt"]
            if opt != "-xc++"
        ],
        "//command_line_option:host_cxxopt": [
            opt
            for opt in settings["//command_line_option:host_cxxopt"]
            if opt != "-xc++"
        ],
    }

# 1:1 transition. rules_foreign_cc reads ctx.fragments.cpp.cxxopts, which this
# changes; host_cxxopt is stripped too so an exec-configured CMake build (a
# cmake() target reached through a tool attribute) is covered as well.
_strip_xcxx = transition(
    implementation = _strip_xcxx_impl,
    inputs = [
        "//command_line_option:cxxopt",
        "//command_line_option:host_cxxopt",
    ],
    outputs = [
        "//command_line_option:cxxopt",
        "//command_line_option:host_cxxopt",
    ],
)

def _transitioned_binary_impl(ctx):
    if len(ctx.attr.binary) != 1:
        fail("binary must name exactly one target")
    dep = ctx.attr.binary[0]
    default_info = dep[DefaultInfo]

    # Re-export the dependency's files and runfiles unchanged. Deliberately not
    # symlinked into this package: the binary loads naja's and TBB's shared
    # libraries through an $ORIGIN-relative rpath, so it has to keep its own
    # location relative to the _solib directories in the runfiles tree.
    return [
        DefaultInfo(
            files = default_info.files,
            runfiles = default_info.default_runfiles,
        ),
    ]

transitioned_binary = rule(
    doc = "Re-exports an executable built with `-xc++` stripped from cxxopts.",
    implementation = _transitioned_binary_impl,
    attrs = {
        "binary": attr.label_list(
            cfg = _strip_xcxx,
            mandatory = True,
            doc = "Exactly one executable target to build in the stripped configuration.",
        ),
    },
)
