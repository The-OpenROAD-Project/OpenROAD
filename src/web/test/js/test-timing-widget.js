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

describe('TimingWidget._showInspector', () => {
    function pathWithNode(node) {
        return makePath(0.0, 'e0', { data_nodes: [node] });
    }

    const FULL_NODE = {
        pin: 'U1/A',
        fanout: 3,
        rise: true,
        clk: false,
        time: 0.12,
        delay: 0.03,
        slew: 0.05,
        load: 0.002,
        direction: 'input',
        instance: 'U1',
        master: 'AND2_X1',
        net: 'n5',
    };

    it('renders a popover populated from the node', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        widget.showPaths('setup', [pathWithNode(FULL_NODE)]);
        widget._selectPathRow(0);

        widget._showInspector(0);

        const pop = widget.element.querySelector('.timing-inspector-popover');
        assert.ok(pop, 'popover was created');
        const text = pop.textContent;
        assert.ok(text.includes('U1/A'), 'shows pin');
        assert.ok(text.includes('input'), 'shows direction');
        assert.ok(text.includes('U1'), 'shows instance');
        assert.ok(text.includes('AND2_X1'), 'shows master');
        assert.ok(text.includes('n5'), 'shows net');
    });

    it('omits rows whose value is empty or missing', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        const minimal = {
            pin: 'TOP_IN', fanout: 0, rise: true, clk: false,
            time: 0, delay: 0, slew: 0, load: 0,
            // no direction, instance, master, net
        };
        widget.showPaths('setup', [pathWithNode(minimal)]);
        widget._selectPathRow(0);

        widget._showInspector(0);

        const pop = widget.element.querySelector('.timing-inspector-popover');
        const headers = Array.from(pop.querySelectorAll('th'), th => th.textContent);
        assert.ok(!headers.includes('Direction'));
        assert.ok(!headers.includes('Instance'));
        assert.ok(!headers.includes('Master'));
        assert.ok(!headers.includes('Net'));
        assert.ok(headers.includes('Pin'));
    });

    it('closes on Escape', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        widget.showPaths('setup', [pathWithNode(FULL_NODE)]);
        widget._selectPathRow(0);
        widget._showInspector(0);
        assert.ok(widget.element.querySelector('.timing-inspector-popover'));

        document.dispatchEvent(new window.KeyboardEvent('keydown', { key: 'Escape' }));

        assert.equal(widget.element.querySelector('.timing-inspector-popover'), null);
    });

    it('closes on the × button', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        widget.showPaths('setup', [pathWithNode(FULL_NODE)]);
        widget._selectPathRow(0);
        widget._showInspector(0);

        const btn = widget.element.querySelector('.timing-inspector-popover .close-btn');
        btn.click();

        assert.equal(widget.element.querySelector('.timing-inspector-popover'), null);
    });

    it('closes when opening a new inspector (replaces existing)', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        widget.showPaths('setup', [
            makePath(0.0, 'e0', { data_nodes: [FULL_NODE, { ...FULL_NODE, pin: 'U2/A' }] }),
        ]);
        widget._selectPathRow(0);

        widget._showInspector(0);
        widget._showInspector(1);

        const popovers = widget.element.querySelectorAll('.timing-inspector-popover');
        assert.equal(popovers.length, 1);
        assert.ok(popovers[0].textContent.includes('U2/A'));
    });
});

describe('TimingWidget path-table sorting', () => {
    function endPins(widget) {
        const rows = widget._pathTable.querySelectorAll('tbody tr');
        return Array.from(rows, tr => tr.querySelectorAll('td')[9].textContent);
    }

    it('defaults to Slack ascending', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        widget.showPaths('setup', [
            makePath(0.5, 'pos'),
            makePath(-0.2, 'worst'),
            makePath(0.1, 'small'),
        ]);
        assert.deepEqual(endPins(widget), ['worst', 'small', 'pos']);
    });

    it('clicking the Slack header toggles to descending', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        widget.showPaths('setup', [
            makePath(0.5, 'pos'),
            makePath(-0.2, 'worst'),
            makePath(0.1, 'small'),
        ]);

        const slackTh = widget._pathTable.querySelectorAll('thead th')[3];
        slackTh.dispatchEvent(new window.MouseEvent('click', { bubbles: true }));

        assert.deepEqual(endPins(widget), ['pos', 'small', 'worst']);
        // Re-query the header after the re-render triggered by the click.
        const slackThAfter = widget._pathTable.querySelectorAll('thead th')[3];
        assert.ok(slackThAfter.textContent.includes('▼'));
    });

    it('sorts by a different column when its header is clicked', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        widget.showPaths('setup', [
            makePath(0.0, 'charlie'),
            makePath(0.0, 'alpha'),
            makePath(0.0, 'bravo'),
        ]);
        const endTh = widget._pathTable.querySelectorAll('thead th')[9];
        endTh.dispatchEvent(new window.MouseEvent('click', { bubbles: true }));
        assert.deepEqual(endPins(widget), ['alpha', 'bravo', 'charlie']);
    });

    it('ignores clicks on the resize grip', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        widget.showPaths('setup', [
            makePath(0.5, 'pos'),
            makePath(-0.2, 'worst'),
        ]);

        // Simulate a click whose target is the grip span.
        const slackTh = widget._pathTable.querySelectorAll('thead th')[3];
        const grip = document.createElement('span');
        grip.className = 'col-resize-grip';
        slackTh.appendChild(grip);
        grip.dispatchEvent(new window.MouseEvent('click', { bubbles: true }));

        // Still ascending.
        assert.deepEqual(endPins(widget), ['worst', 'pos']);
    });

    it('preserves selection across re-sorts (by path object identity)', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        widget.showPaths('setup', [
            makePath(0.5, 'pos'),
            makePath(-0.2, 'worst'),
            makePath(0.1, 'small'),
        ]);
        // Default ascending: index 0 → 'worst'.  Select 'small' (row index 1).
        widget._selectPathRow(1);
        assert.equal(widget._selectedPathIndex, 1);

        // Click End header: alphabetical ascending → pos, small, worst.
        const endTh = widget._pathTable.querySelectorAll('thead th')[9];
        endTh.dispatchEvent(new window.MouseEvent('click', { bubbles: true }));

        // 'small' now sits at row index 1 (p < s < w).
        const rows = widget._pathTable.querySelectorAll('tbody tr');
        assert.equal(rows[widget._selectedPathIndex].querySelectorAll('td')[9].textContent, 'small');
    });

    it('header tooltips match Qt wording for Skew / Logic Delay / Logic Depth', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        widget.showPaths('setup', [makePath(0.0, 'a')]);
        const ths = widget._pathTable.querySelectorAll('thead th');
        // Columns: Clock(0) Required(1) Arrival(2) Slack(3) Skew(4)
        //          Logic Delay(5) Logic Depth(6) Fanout(7) Start(8) End(9)
        assert.ok(ths[4].title.includes('CRPR'), 'skew tooltip');
        assert.ok(ths[5].title.includes('excluding buffers'), 'logic delay tooltip');
        assert.ok(ths[6].title.includes('excluding buffers'), 'logic depth tooltip');
    });
});
