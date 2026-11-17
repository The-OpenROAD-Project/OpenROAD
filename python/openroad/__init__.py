# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

import os as _os
import sys as _sys

import _openroad_loader  # noqa: F401  (must precede any wrapper import)

# The SWIG wrappers live in openroad.<tool> (openroad/<tool>/<tool>_py.py).
# Under Bazel they sit in the repository's openroad/ directory, apart from this
# package, so add every such directory on sys.path to the package path. In the
# wheel both are installed into the same directory.
__path__ = list(__path__) + [
    _os.path.join(_p, "openroad")
    for _p in _sys.path
    if _os.path.isdir(_os.path.join(_p, "openroad"))
    and _os.path.join(_p, "openroad") not in __path__
]

# Importing any tool module (import odb) imports this package first, as the
# parent of its wrapper, so the ORD API and the tool modules load lazily, on
# first use of a name from ord_py (Tech, Design, ...).
_loaded = False
_public = []


def _load():
    global _loaded
    if _loaded:
        return
    # Set first, so that a lookup during the imports below does not recurse.
    _loaded = True
    try:
        _import_all()
    except BaseException:
        _loaded = False
        raise


def _import_all():
    import importlib

    import ord_py

    public = getattr(ord_py, "__all__", None)
    if public is None:
        public = [name for name in vars(ord_py) if not name.startswith("_")]
    globals().update({name: getattr(ord_py, name) for name in public})
    _public.extend(public)

    # Import every tool module so that objects Design returns (getOpendp(),
    # getGlobalRouter(), ...) get their tool's Python class rather than a bare
    # SWIG pointer. Use import_module so top-level tool modules do not
    # overwrite openroad.<tool> subpackages (such as openroad.odb and
    # openroad.utl).
    for tool in _openroad_loader._TOOLS:
        importlib.import_module(tool)


def __getattr__(name):
    # from openroad import * exports ord_py's names, as an eager import did.
    if name == "__all__":
        _load()
        return list(_public)
    # Other dunder lookups come from the import system and introspection, and
    # tool names are submodules, which the import system finds on __path__
    # after this lookup fails; neither needs ord_py.
    if not name.startswith("__") and name not in _openroad_loader._TOOLS:
        _load()
        if name in globals():
            return globals()[name]
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")


def __dir__():
    _load()
    return sorted(globals())
