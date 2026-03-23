// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { layerRangeSet } from '../../src/display-controls.js';

// 10 layers: Metal1, Via1, Metal2, Via2, ... Metal5, Via5
const COUNT = 10;

describe('layerRangeSet', () => {
    it('show only selected (center only)', () => {
        const s = layerRangeSet(3, 0, 0, COUNT);
        assert.deepEqual(s, new Set([3]));
    });

    it('range ±1 in the middle', () => {
        const s = layerRangeSet(4, 1, 1, COUNT);
        assert.deepEqual(s, new Set([3, 4, 5]));
    });

    it('range ±2 in the middle', () => {
        const s = layerRangeSet(5, 2, 2, COUNT);
        assert.deepEqual(s, new Set([3, 4, 5, 6, 7]));
    });

    it('range down only (lower=1, upper=0)', () => {
        const s = layerRangeSet(4, 1, 0, COUNT);
        assert.deepEqual(s, new Set([3, 4]));
    });

    it('range up only (lower=0, upper=1)', () => {
        const s = layerRangeSet(4, 0, 1, COUNT);
        assert.deepEqual(s, new Set([4, 5]));
    });

    it('clamps at lower bound', () => {
        const s = layerRangeSet(0, 2, 2, COUNT);
        assert.deepEqual(s, new Set([0, 1, 2]));
    });

    it('clamps at upper bound', () => {
        const s = layerRangeSet(9, 2, 2, COUNT);
        assert.deepEqual(s, new Set([7, 8, 9]));
    });

    it('single layer total', () => {
        const s = layerRangeSet(0, 1, 1, 1);
        assert.deepEqual(s, new Set([0]));
    });

    it('range down at first layer returns only first', () => {
        const s = layerRangeSet(0, 1, 0, COUNT);
        assert.deepEqual(s, new Set([0]));
    });

    it('range up at last layer returns only last', () => {
        const s = layerRangeSet(9, 0, 1, COUNT);
        assert.deepEqual(s, new Set([9]));
    });
});
