// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The 3D viewer reads three.js off window.THREE, which vendor-globals.js fills
// from three-subset.js; a name missing there would only fail in the browser.

import { it } from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import * as subset from '../../src/three-subset.js';

it('three-subset.js exports every three.js name the 3D viewer uses', () => {
    const widget = readFileSync(
        new URL('../../src/3d-viewer-widget.js', import.meta.url), 'utf8');
    const used = new Set(
        [...widget.matchAll(/\bTHREE\.([A-Za-z_$][\w$]*)/g)].map((m) => m[1]));
    assert.ok(used.size > 0);
    for (const name of used) {
        assert.notEqual(subset[name], undefined, `three-subset.js lacks ${name}`);
    }
});
