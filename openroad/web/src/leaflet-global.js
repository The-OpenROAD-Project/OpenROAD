// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Leaflet sets window.L as it loads; the entries import this module first.  The
// modules using L read the global, so their unit tests can load them with no DOM.

import 'leaflet';
