// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Libraries only some panels use, handed over as globals: the schematic reads
// netlistsvg and its skin, the 3D view reads THREE.

import * as netlistsvg from 'netlistsvg';
import * as THREE from './three-subset.js';
import openroadSkin from './openroad_skin.svg';

window.netlistsvg = netlistsvg;
window.openroadSkin = openroadSkin;
window.THREE = THREE;
