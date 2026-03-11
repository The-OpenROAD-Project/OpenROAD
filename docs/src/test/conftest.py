## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2024-2026, The OpenROAD Authors

import os
import sys

# Make the symlinked scripts importable when pytest is invoked from any directory.
sys.path.insert(0, os.path.dirname(__file__))
