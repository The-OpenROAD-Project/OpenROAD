// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Entry point for the bundle inlined into a saved report.  It leaves out
// vendor-globals.js, so elk and netlistsvg stay out of every saved file;
// schematic-widget.js already stands down when it does not find them.

import './leaflet-global.js';
import './main.js';
