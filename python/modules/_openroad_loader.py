# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Loads every OpenROAD Python extension module from one shared library.

Each SWIG module has its own init function (PyInit__ord_py, PyInit__odb_py,
PyInit__ant_py, ...), and all of them are linked into _openroad.so. The
dynamic loader maps that file once, so the modules keep separate
namespaces while sharing one copy of the C++ state, as the modules linked
into the CMake `openroad` executable do.
"""

import importlib.abc
import importlib.machinery
import importlib.util
import os
import sys

# The tool modules in _openroad.so. Each SWIG wrapper src/<tool>/<tool>_py.py
# imports its extension as src.<tool>._<tool>_py; ord_py.py, at the top
# level, imports _ord_py. Only these exact names are loaded from the library,
# so a module of the same name in another package is left alone.
_TOOLS = """
    ant cgt cts dpl drt exa fin fsv gpl grt ifp odb
    par pdn ppl psm rcx rmp rsz stt tap utl wmk
""".split()
EXTENSIONS = frozenset(["_ord_py"] + [f"src.{tool}._{tool}_py" for tool in _TOOLS])


def _find_library():
    # The library sits next to this file in an installed wheel and at the
    # runfiles root under Bazel; both are on sys.path.
    for entry in [os.path.dirname(__file__)] + sys.path:
        candidate = os.path.join(entry or os.curdir, "_openroad.so")
        if os.path.exists(candidate):
            return os.path.abspath(candidate)
    raise ImportError("Cannot find _openroad.so on sys.path")


LIBRARY = _find_library()


class _OpenRoadFinder(importlib.abc.MetaPathFinder):
    def find_spec(self, fullname, path, target=None):
        if fullname not in EXTENSIONS:
            return None
        loader = importlib.machinery.ExtensionFileLoader(fullname, LIBRARY)
        return importlib.util.spec_from_file_location(fullname, LIBRARY, loader=loader)


if not any(isinstance(finder, _OpenRoadFinder) for finder in sys.meta_path):
    sys.meta_path.insert(0, _OpenRoadFinder())
