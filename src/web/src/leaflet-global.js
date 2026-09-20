// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Leaflet reaches the viewer as the global L, the way its own <script> tag used
// to set it before the libraries moved into the bundle (issue #11065).  Its UMD
// wrapper only sets the global when no module system is present, and inside a
// bundle it sees CommonJS, so the assignment happens here.
//
// Keeping it a global rather than an import in each of the nine modules that
// use it is deliberate: leaflet touches `window` as it loads, and those modules
// hold pure functions the unit tests exercise with no DOM at all.

import * as L from 'leaflet';

window.L = L;
