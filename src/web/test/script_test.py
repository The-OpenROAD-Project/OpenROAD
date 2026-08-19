#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Shared scaffolding for the tests that exercise src/web's build scripts.  Those
# scripts are executables rather than a package, so each test imports one by
# path and reports every failure it finds instead of stopping at the first.

import importlib.util
import os
import sys

# Importing by path would otherwise leave __pycache__ in the source tree.
sys.dont_write_bytecode = True

THIS_DIR = os.path.dirname(os.path.abspath(__file__))

failures = []


def load_module(path):
    """Import a script by path, without registering it in sys.modules."""
    spec = importlib.util.spec_from_file_location("script_under_test", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def load_script(relative_path):
    """Import one of src/web's scripts, named relative to src/web."""
    return load_module(os.path.join(THIS_DIR, "..", relative_path))


def check(name, condition, detail=""):
    if not condition:
        failures.append(f"{name}: {detail}" if detail else name)


def check_raises(name, call):
    try:
        call()
    except SystemExit:
        return
    failures.append(f"{name}: expected SystemExit, none raised")


def report(message):
    """Exit code for the test: the failures, or the all-clear."""
    if failures:
        print("\n".join(failures), file=sys.stderr)
        return 1
    print(message)
    return 0
