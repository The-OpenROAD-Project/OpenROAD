// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// netlistsvg's prebuilt browser bundle does not bundle ELK; it picks it up off
// the global scope.  This lives in a module of its own because a module's
// imports are hoisted above its statements: only a separate module is
// guaranteed to have run its assignment before the next import is evaluated.

import ELK from 'elkjs/lib/elk.bundled.js';

window.ELK = ELK;
