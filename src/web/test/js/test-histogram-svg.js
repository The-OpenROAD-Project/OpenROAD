// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { renderHistogramSVG } from '../../src/histogram-svg.js';

const SAMPLE_DATA = {
    bins: [
        { lower: -0.2, upper: -0.1, count: 5, negative: true },
        { lower: -0.1, upper: 0.0, count: 8, negative: true },
        { lower: 0.0, upper: 0.1, count: 20, negative: false },
        { lower: 0.1, upper: 0.2, count: 15, negative: false },
    ],
    time_unit: 'ns',
    total_endpoints: 48,
    unconstrained_count: 0,
};

describe('renderHistogramSVG', () => {
    it('returns valid SVG for sample data', () => {
        const svg = renderHistogramSVG(SAMPLE_DATA, 800, 400);
        assert.ok(svg.startsWith('<svg'));
        assert.ok(svg.endsWith('</svg>'));
        assert.ok(svg.includes('xmlns="http://www.w3.org/2000/svg"'));
    });

    it('contains bars as rect elements', () => {
        const svg = renderHistogramSVG(SAMPLE_DATA, 800, 400);
        const rectCount = (svg.match(/<rect /g) || []).length;
        // 1 background rect + 4 bar rects
        assert.ok(rectCount >= 4, `expected at least 4 rects, got ${rectCount}`);
    });

    it('uses negative colors for negative bins', () => {
        const svg = renderHistogramSVG(SAMPLE_DATA, 800, 400);
        assert.ok(svg.includes('#f08080'), 'should contain negative fill color');
        assert.ok(svg.includes('#8b0000'), 'should contain negative border color');
    });

    it('uses positive colors for positive bins', () => {
        const svg = renderHistogramSVG(SAMPLE_DATA, 800, 400);
        assert.ok(svg.includes('#90ee90'), 'should contain positive fill color');
        assert.ok(svg.includes('#006400'), 'should contain positive border color');
    });

    it('includes title with endpoint count', () => {
        const svg = renderHistogramSVG(SAMPLE_DATA, 800, 400);
        assert.ok(svg.includes('48 endpoints'), 'should show total endpoint count');
    });

    it('includes time unit', () => {
        const svg = renderHistogramSVG(SAMPLE_DATA, 800, 400);
        assert.ok(svg.includes('ns'), 'should include time unit');
    });

    it('handles empty data gracefully', () => {
        const svg = renderHistogramSVG({ bins: [] }, 800, 400);
        assert.ok(svg.includes('No data'));
    });

    it('handles null data gracefully', () => {
        const svg = renderHistogramSVG(null, 800, 400);
        assert.ok(svg.includes('No data'));
    });

    it('includes tooltip titles on bars', () => {
        const svg = renderHistogramSVG(SAMPLE_DATA, 800, 400);
        assert.ok(svg.includes('<title>'), 'should have tooltip titles');
        assert.ok(svg.includes('5 endpoints'), 'tooltip should show count');
    });
});
