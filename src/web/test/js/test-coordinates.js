// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { dbuToWorld, worldToDbu, dbuRectToWorldRect }
    from '../../src/coordinates.js';

// The layout view's world: (dbu - origin) * scale with y up, so the tile grid
// is the square [0, tileSize] whatever the design's aspect.
describe('dbuToWorld / worldToDbu', () => {
    const scale = 240 / 1000;   // tile size 240 over a 1000-DBU grid
    it('maps the grid origin to world (0, 0) and keeps y up', () => {
        assert.deepEqual(dbuToWorld(500, 700, scale, 500, 700), [0, 0]);
        assert.deepEqual(dbuToWorld(1500, 1700, scale, 500, 700), [240, 240]);
    });

    it('round-trips to whole DBU', () => {
        const [wx, wy] = dbuToWorld(1234, 567, scale, 100, 50);
        assert.deepEqual(worldToDbu(wx, wy, scale, 100, 50),
                         { dbuX: 1234, dbuY: 567 });
    });

    it('turns two corners in any order into [minX, minY, maxX, maxY]', () => {
        assert.deepEqual(dbuRectToWorldRect(1000, 0, 0, 500, scale),
                         [0, 0, 240, 120]);
    });
});

describe('round-trip dbu <-> world', () => {
    it('preserves zero', () => {
        const [wx, wy] = dbuToWorld(0, 0, 1e-6);
        assert.deepEqual(worldToDbu(wx, wy, 1e-6), { dbuX: 0, dbuY: 0 });
    });

    it('preserves large values', () => {
        const scale = 240 / 1e7;
        const [wx, wy] = dbuToWorld(9999999, 9999999, scale);
        assert.deepEqual(worldToDbu(wx, wy, scale),
                         { dbuX: 9999999, dbuY: 9999999 });
    });

    it('preserves negative coordinates below the origin', () => {
        const scale = 240 / 1000;
        const [wx, wy] = dbuToWorld(-50, 20, scale, 100, 100);
        assert.ok(wx < 0 && wy < 0);
        assert.deepEqual(worldToDbu(wx, wy, scale, 100, 100),
                         { dbuX: -50, dbuY: 20 });
    });
});
