// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Libraries only the live panels use, handed over as globals so the saved
// report's bundle, which leaves this module out, carries none of them.
// elk-global.js has to run first: netlistsvg reads ELK as it loads.

import './elk-global.js';
import netlistsvg from 'netlistsvg/built/netlistsvg.bundle.js';
import * as THREE from './three-subset.js';
import openroadSkin from './openroad_skin.svg';

window.netlistsvg = netlistsvg;
window.openroadSkin = openroadSkin;
window.THREE = THREE;
