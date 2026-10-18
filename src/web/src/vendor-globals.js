// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Libraries only the live panels use, handed over as globals: the CMake build
// still sets them from CDNs, and a saved report goes without them.

import * as netlistsvg from 'netlistsvg';
import * as THREE from './three-subset.js';
import openroadSkin from './openroad_skin.svg';

window.netlistsvg = netlistsvg;
window.openroadSkin = openroadSkin;
window.THREE = THREE;
