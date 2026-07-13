// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { computeBoundsTransforms, boundsEqual } from '../../src/ui-utils.js';

describe('computeBoundsTransforms', () => {
    it('derives the tile-grid transforms from a bounds response', () => {
        // ibex-like: block bbox inflated by the pin-label margin.
        const t = computeBoundsTransforms([[-63538, -63538],
                                           [544908, 544908]]);
        assert.equal(t.originX, -63538);
        assert.equal(t.originY, -63538);
        assert.equal(t.maxDXDY, 608446);
        assert.equal(t.scale, 256 / 608446);
        assert.deepEqual(t.fitBounds, [[-256, 0], [0, 256]]);
    });

    it('uses the larger dimension for non-square designs', () => {
        const t = computeBoundsTransforms([[0, 0], [100, 400]]);
        assert.equal(t.maxDXDY, 400);
        // fitBounds top edge reflects the smaller height.
        assert.deepEqual(t.fitBounds,
                         [[-256, 0], [(100 - 400) * (256 / 400), 256]]);
    });

    it('returns null for an empty or degenerate design', () => {
        assert.equal(computeBoundsTransforms(null), null);
        assert.equal(computeBoundsTransforms([[0, 0], [0, 0]]), null);
        assert.equal(computeBoundsTransforms([[10, 10], [10, 400]]), null);
    });
});

describe('boundsEqual', () => {
    it('compares all four corners', () => {
        const a = [[0, 0], [100, 100]];
        assert.ok(boundsEqual(a, [[0, 0], [100, 100]]));
        assert.ok(!boundsEqual(a, [[0, 0], [100, 101]]));
        assert.ok(!boundsEqual(a, [[-1, 0], [100, 100]]));
        assert.ok(!boundsEqual(a, null));
        assert.ok(!boundsEqual(null, a));
    });
});
