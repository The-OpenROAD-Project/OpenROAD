// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// What the schematic panel needs, reaching the page as globals rather than as
// imports.  Only the served bundle pulls this in: the saved report has no
// schematic panel, and these are 1.9 MB.
//
// The two libraries' UMD wrappers set their globals themselves only when no
// module system is present.  Inside a bundle they see CommonJS and assign to
// module.exports, so the globals are set here explicitly.
//
// The skin is a global for a different reason: `import` of an .svg is something
// only the bundler understands, and schematic-widget.js is loaded directly by
// the unit tests, which run in plain node.
//
// The order of the first two imports is load-bearing: see elk-global.js.

import './elk-global.js';
import netlistsvg from 'netlistsvg/built/netlistsvg.bundle.js';
import openroadSkin from './openroad_skin.svg';

window.netlistsvg = netlistsvg;
window.openroadSkin = openroadSkin;
