// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Entry point for the bundle the web server serves.  Identical to
// entry-report.js but for the schematic libraries, which are 1.9 MB and belong
// only here: the saved report inlines its bundle as text, and the schematic
// panel needs a live server anyway.

import './leaflet-global.js';
import './vendor-globals.js';
import './main.js';
