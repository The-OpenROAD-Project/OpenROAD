# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

"""Shared helpers for SWIG wrapping rules (tcl_wrap_cc, python_wrap_cc).

The two languages use distinct providers (TclSwigInfo vs PythonSwigInfo) so
that a python_wrap_cc target cannot be silently passed as a dep of
tcl_wrap_cc. The depset-building logic, however, is provider-agnostic and
lives here.
"""

def get_transitive_srcs(provider, srcs, deps):
    return depset(
        srcs,
        transitive = [dep[provider].transitive_srcs for dep in deps],
    )

def get_transitive_includes(provider, local_includes, deps):
    return depset(
        local_includes,
        transitive = [dep[provider].includes for dep in deps],
    )

def get_transitive_options(provider, options, deps):
    return depset(
        options,
        transitive = [dep[provider].swig_options for dep in deps],
    )
