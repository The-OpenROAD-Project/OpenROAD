// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import {
    computeHistogramLayout,
    kLeftMargin, kRightMargin, kTopMargin, kBottomMargin,
} from '../../src/charts-widget.js';

describe('computeHistogramLayout', () => {
    it('returns empty bars for null data', () => {
        const result = computeHistogramLayout(null, 500, 400);
        assert.equal(result.bars.length, 0);
    });

    it('returns empty bars for empty bins', () => {
        const result = computeHistogramLayout({ bins: [] }, 500, 400);
        assert.equal(result.bars.length, 0);
    });

    it('returns empty bars for undefined bins', () => {
        const result = computeHistogramLayout({}, 500, 400);
        assert.equal(result.bars.length, 0);
    });

    it('computes correct bar count for single bin', () => {
        const result = computeHistogramLayout({
            bins: [{ lower: 0, upper: 0.1, count: 10, negative: false }],
            time_unit: 'ns',
        }, 500, 400);
        assert.equal(result.bars.length, 1);
        assert.equal(result.bars[0].count, 10);
        assert.ok(result.bars[0].height > 0);
        assert.equal(result.bars[0].negative, false);
    });

    it('preserves negative flag from bins', () => {
        const result = computeHistogramLayout({
            bins: [
                { lower: -0.2, upper: -0.1, count: 5, negative: true },
                { lower: -0.1, upper: 0.0, count: 8, negative: true },
                { lower: 0.0, upper: 0.1, count: 20, negative: false },
            ],
            time_unit: 'ns',
        }, 500, 400);
        assert.equal(result.bars.length, 3);
        assert.ok(result.bars[0].negative);
        assert.ok(result.bars[1].negative);
        assert.ok(!result.bars[2].negative);
    });

    it('scales bar heights proportional to count', () => {
        const result = computeHistogramLayout({
            bins: [
                { lower: 0, upper: 0.1, count: 100, negative: false },
                { lower: 0.1, upper: 0.2, count: 50, negative: false },
            ],
            time_unit: 'ns',
        }, 500, 400);
        assert.ok(result.bars[0].height > result.bars[1].height);
    });

    it('positions bars within chart area', () => {
        const w = 500, h = 400;
        const result = computeHistogramLayout({
            bins: [
                { lower: 0, upper: 0.1, count: 10, negative: false },
                { lower: 0.1, upper: 0.2, count: 20, negative: false },
            ],
            time_unit: 'ns',
        }, w, h);

        for (const bar of result.bars) {
            assert.ok(bar.x >= kLeftMargin, 'bar starts after left margin');
            assert.ok(bar.x + bar.width <= w - kRightMargin + 1,
                       'bar ends before right margin');
            assert.ok(bar.y >= kTopMargin, 'bar top after top margin');
            assert.ok(bar.y + bar.height <= h - kBottomMargin + 1,
                       'bar bottom before bottom margin');
        }
    });

    it('provides chart area bounds', () => {
        const result = computeHistogramLayout({
            bins: [{ lower: 0, upper: 0.1, count: 5, negative: false }],
            time_unit: 'ns',
        }, 500, 400);
        assert.ok(result.chartArea);
        assert.equal(result.chartArea.left, kLeftMargin);
        assert.equal(result.chartArea.right, 500 - kRightMargin);
        assert.equal(result.chartArea.top, kTopMargin);
        assert.equal(result.chartArea.bottom, 400 - kBottomMargin);
    });

    it('handles all-zero counts', () => {
        const result = computeHistogramLayout({
            bins: [
                { lower: 0, upper: 0.1, count: 0, negative: false },
                { lower: 0.1, upper: 0.2, count: 0, negative: false },
            ],
            time_unit: 'ns',
        }, 500, 400);
        assert.equal(result.bars.length, 2);
        for (const bar of result.bars) {
            assert.equal(bar.height, 0);
        }
    });

    it('returns empty for tiny canvas', () => {
        const result = computeHistogramLayout({
            bins: [{ lower: 0, upper: 0.1, count: 5, negative: false }],
        }, 50, 50);
        assert.equal(result.bars.length, 0);
    });

    it('provides yTicks for small counts', () => {
        const result = computeHistogramLayout({
            bins: [{ lower: 0, upper: 0.1, count: 7, negative: false }],
            time_unit: 'ns',
        }, 500, 400);
        assert.ok(result.yTicks.length > 0);
        assert.equal(result.yTicks[0], 0);
        assert.ok(result.yMax >= 7);
    });

    it('provides yTicks for large counts', () => {
        const result = computeHistogramLayout({
            bins: [{ lower: 0, upper: 0.1, count: 350, negative: false }],
            time_unit: 'ns',
        }, 500, 400);
        assert.ok(result.yMax >= 350);
        assert.ok(result.yTicks.length >= 2);
    });
});
