# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

import _openroad_loader  # noqa: F401  (must precede the wrapper import)
from ord_py import *

# Import every tool module, as `openroad -python` does at startup, so that
# objects Design returns (getOpendp(), getGlobalRouter(), ...) get their
# tool's Python class rather than a bare SWIG pointer.
import ant, cgt, cts, dpl, drt, exa, fin, gpl, grt, ifp  # noqa: E401, F401
import odb, par, pdn, ppl, psm, rcx, rmp, rsz, stt, tap  # noqa: E401, F401
import utl, wmk  # noqa: E401, F401
