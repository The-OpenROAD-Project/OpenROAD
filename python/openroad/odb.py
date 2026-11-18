# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

import os as _os
import sys as _sys

__path__ = [
    _os.path.join(_p, "openroad", "odb")
    for _p in [_os.path.dirname(_os.path.dirname(__file__))] + _sys.path
    if _os.path.isdir(_os.path.join(_p, "openroad", "odb"))
]

from openroad.odb.odb_py import *
