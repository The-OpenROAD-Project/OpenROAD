# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Loads every OpenROAD Python extension module from one shared library.

Each SWIG module has its own init function (PyInit__ord_py, PyInit__odb,
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

# The extension modules in _openroad.so, without any package prefix.
_EXTENSIONS = frozenset(
    ["_ord_py", "_odb", "_utl", "_gpl", "_wmk"]
    + [
        f"_{tool}_py"
        for tool in (
            "ant cgt cts dpl drt exa fin grt ifp par pdn ppl psm rcx rmp rsz" " stt tap"
        ).split()
    ]
)


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
        if fullname.rpartition(".")[2] not in _EXTENSIONS:
            return None
        loader = importlib.machinery.ExtensionFileLoader(fullname, LIBRARY)
        return importlib.util.spec_from_file_location(fullname, LIBRARY, loader=loader)


if not any(isinstance(finder, _OpenRoadFinder) for finder in sys.meta_path):
    sys.meta_path.insert(0, _OpenRoadFinder())
