// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { TimingWidget } from '../../src/timing-widget.js';

// jsdom does not implement scrollIntoView; stub it so _selectPathRow runs.
if (!window.HTMLElement.prototype.scrollIntoView) {
    window.HTMLElement.prototype.scrollIntoView = function () {};
}

// Build an app stub that records every websocketManager.request call and
// lets each test control the response per message type.
function createMockApp(responses = {}) {
    const requests = [];
    return {
        requests,
        websocketManager: {
            request(msg) {
                requests.push(msg);
                const handler = responses[msg.type];
                const value = handler ? handler(msg) : {};
                return Promise.resolve(value);
            },
        },
    };
}

function makePath(slack, endPin, extra = {}) {
    return {
        start_clk: 'clk', end_clk: 'clk',
        slack, arrival: 0, required: 0, skew: 0,
        path_delay: 0, logic_depth: 0, fanout: 1,
        start_pin: 'start', end_pin: endPin,
        data_nodes: [], capture_nodes: [],
        ...extra,
    };
}

describe('TimingWidget._selectPathRow', () => {
    it('sends timing_highlight with the row index when paths are unfiltered', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        widget.showPaths('setup', [
            makePath(-0.1, 'a'),
            makePath(0.0, 'b'),
            makePath(0.1, 'c'),
        ]);

        // Clear requests from showPaths' _clearTimingHighlight.
        app.requests.length = 0;

        widget._selectPathRow(1);

        const highlight = app.requests.find(r => r.type === 'timing_highlight');
        assert.ok(highlight, 'timing_highlight was requested');
        assert.equal(highlight.path_index, 1);
        assert.equal(highlight.is_setup, 1);
    });

    it('sends timing_highlight with _originalIndex when paths were filtered', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        // Filtered result — table row 0 maps to original overlay index 2.
        widget.showPaths('setup', [
            { ...makePath(0.0, 'b'), _originalIndex: 2 },
            { ...makePath(0.05, 'c'), _originalIndex: 4 },
        ]);

        app.requests.length = 0;

        widget._selectPathRow(0);
        let highlight = app.requests.find(r => r.type === 'timing_highlight');
        assert.equal(highlight.path_index, 2);

        app.requests.length = 0;
        widget._selectPathRow(1);
        highlight = app.requests.find(r => r.type === 'timing_highlight');
        assert.equal(highlight.path_index, 4);
    });

    it('uses hold side flag when current tab is hold', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        widget.showPaths('hold', [
            { ...makePath(0.1, 'h0'), _originalIndex: 3 },
        ]);

        app.requests.length = 0;
        widget._selectPathRow(0);

        const highlight = app.requests.find(r => r.type === 'timing_highlight');
        assert.equal(highlight.path_index, 3);
        assert.equal(highlight.is_setup, 0);
    });

    it('ignores out-of-range indices without sending a request', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        widget.showPaths('setup', [makePath(0.0, 'a')]);

        app.requests.length = 0;
        widget._selectPathRow(-1);
        widget._selectPathRow(5);

        assert.equal(
            app.requests.filter(r => r.type === 'timing_highlight').length, 0);
    });
});
