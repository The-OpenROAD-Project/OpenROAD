# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

# rmp has its own extension when it is built; otherwise its commands are in
# the combined openroadpy module.
try:
    from rmp_py import *
except ImportError:
    from openroadpy import *
