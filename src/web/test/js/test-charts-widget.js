// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import {
    ChartsWidget,
    computeHistogramLayout, hitTestColumn,
    kLeftMargin, kRightMargin, kTopMargin, kBottomMargin,
} from '../../src/charts-widget.js';

function installCanvasStubs() {
    window.HTMLCanvasElement.prototype.getContext = function () {
        return {
            clearRect() {},
            fillRect() {},
            strokeRect() {},
            beginPath() {},
            moveTo() {},
            lineTo() {},
            stroke() {},
            fillText(_text, x, y) {
                assert.ok(Number.isFinite(x), 'x coordinate must be finite');
                assert.ok(Number.isFinite(y), 'y coordinate must be finite');
            },
            save() {},
            restore() {},
            translate() {},
            rotate() {},
            setTransform() {},
            fillStyle: '',
            strokeStyle: '',
            lineWidth: 1,
            font: '',
            textAlign: '',
            textBaseline: '',
        };
    };
}

function createWidgetApp() {
    const requests = [];
    return {
        requests,
        websocketManager: {
            request(msg) {
                requests.push(msg);
                if (msg.type === 'timing_report') {
                    return Promise.resolve({ paths: [] });
                }
                return Promise.resolve({});
            },
        },
        focusComponent() {},
        timingWidget: {
            showPaths() {},
        },
    };
}

function createWidget() {
    installCanvasStubs();
    const app = createWidgetApp();
    const widget = new ChartsWidget(app, () => {});
    document.body.appendChild(widget.element);
    widget.element.getBoundingClientRect = () => ({
        left: 0, top: 0, width: 320, height: 240, right: 320, bottom: 240,
    });
    widget._canvas.getBoundingClientRect = () => ({
        left: 0, top: 0, width: 320, height: 240, right: 320, bottom: 240,
    });
    return { app, widget };
}

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

describe('hitTestColumn', () => {
    // Build a two-bar layout: one tall bar, one very short bar.
    const layout = computeHistogramLayout({
        bins: [
            { lower: -0.1, upper: 0.0, count: 100, negative: true },
            { lower: 0.0, upper: 0.1, count: 2, negative: false },
        ],
        time_unit: 'ns',
    }, 500, 400);
    const { bars, chartArea } = layout;
    // Sanity: the second bar is much shorter than the first.
    assert.ok(bars[1].height < bars[0].height / 5);

    it('hits a bar by clicking within its rendered area', () => {
        const cx = bars[0].x + bars[0].width / 2;
        const cy = bars[0].y + bars[0].height / 2;
        assert.equal(hitTestColumn(bars, chartArea, cx, cy), bars[0]);
    });

    it('hits a short bar by clicking above it in the same column', () => {
        // Click in the middle of the column, well above the tiny bar.
        const cx = bars[1].x + bars[1].width / 2;
        const cy = chartArea.top + 5;  // near the top of the chart
        assert.equal(hitTestColumn(bars, chartArea, cx, cy), bars[1]);
    });

    it('returns null when clicking outside the chart area vertically', () => {
        const cx = bars[0].x + bars[0].width / 2;
        assert.equal(hitTestColumn(bars, chartArea, cx, chartArea.top - 1), null);
        assert.equal(hitTestColumn(bars, chartArea, cx, chartArea.bottom + 1), null);
    });

    it('returns null when clicking between bars (gap region)', () => {
        // The gap is between bar 0's right edge and bar 1's left edge.
        const cx = bars[0].x + bars[0].width + 1;  // in the gap
        const cy = (chartArea.top + chartArea.bottom) / 2;
        // Only null if the point is truly outside both bars' x ranges.
        const hit = hitTestColumn(bars, chartArea, cx, cy);
        if (cx < bars[1].x) {
            assert.equal(hit, null);
        }
    });

    it('returns null for null bars or chartArea', () => {
        assert.equal(hitTestColumn(null, chartArea, 100, 200), null);
        assert.equal(hitTestColumn(bars, null, 100, 200), null);
    });

    it('returns null when clicking left of all bars', () => {
        const cy = (chartArea.top + chartArea.bottom) / 2;
        assert.equal(hitTestColumn(bars, chartArea, bars[0].x - 5, cy), null);
    });
});

describe('ChartsWidget debug charts', () => {
    it('ignores histogram hover and click handlers while a debug chart is active', async () => {
        const { app, widget } = createWidget();
        widget._histogramData = {
            bins: [{ lower: 0, upper: 1, count: 7, negative: false }],
            time_unit: 'ns',
        };
        widget._bars = [{
            x: 0, y: 0, width: 160, height: 120, count: 7, lower: 0, upper: 1,
        }];
        widget._chartArea = { left: 0, right: 200, top: 0, bottom: 200 };
        widget._hoveredBar = widget._bars[0];
        widget._tooltip.style.display = 'block';
        widget._render = () => {
            throw new Error('histogram render should not run in debug mode');
        };

        widget.setDebugCharts([{
            name: 'GPL',
            x_label: 'iter',
            y_labels: ['hpwl'],
            points: [{ x: 5, ys: [42] }],
        }]);

        widget._handleHover({ clientX: 10, clientY: 10 });
        assert.equal(widget._hoveredBar, null);
        assert.equal(widget._tooltip.style.display, 'none');

        await widget._handleClick({ clientX: 10, clientY: 10 });
        assert.equal(
            app.requests.filter((req) => req.type === 'timing_report').length,
            0);
    });

    it('renders a single-point debug chart without NaN axis coordinates', () => {
        const { widget } = createWidget();
        widget.setDebugCharts([{
            name: 'GPL',
            x_label: 'iter',
            y_labels: ['hpwl'],
            points: [{ x: 5, ys: [42] }],
        }]);
    });
});
