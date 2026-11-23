# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Run the shared command-count checker for an OpenROAD module."""

load("@rules_python//python:defs.bzl", "py_test")

def man_tcl_check_test(name, module, tags = ["py"]):
    """Check that a module's Tcl commands and README have matching counts.

    Args:
        name: Test target name.
        module: OpenROAD module directory name.
        tags: Additional test tags; doc_check is always included.
    """
    py_test(
        name = name,
        args = ["--module", module],
        data = ["//openroad/{}:doc_files".format(module)],
        main_module = "man_tcl_check",
        tags = ["doc_check"] + tags,
        deps = [Label("//docs/src/scripts:man_tcl_check")],
    )
