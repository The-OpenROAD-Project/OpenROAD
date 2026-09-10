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
        assert.equal(highlight.is_setup, true);
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
        assert.equal(highlight.is_setup, false);
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

describe('TimingWidget path table sorting', () => {
    function endPins(widget) {
        const rows = widget._pathTable.querySelectorAll('tbody tr');
        return Array.from(rows, r => r.cells[r.cells.length - 1].textContent);
    }

    function headerByLabel(widget, label) {
        const headers = widget._pathTable.querySelectorAll('thead th');
        return Array.from(headers).find(th => th.textContent.startsWith(label));
    }

    it('sorts by slack ascending by default', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        widget.showPaths('setup', [
            makePath(0.1, 'c'),
            makePath(-0.1, 'a'),
            makePath(0.0, 'b'),
        ]);

        assert.deepEqual(endPins(widget), ['a', 'b', 'c']);
        assert.ok(headerByLabel(widget, 'Slack').textContent.includes('▲'));
    });

    it('clicking a header sorts by that column and toggles order', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        widget.showPaths('setup', [
            makePath(0.0, 'b', { fanout: 2 }),
            makePath(0.1, 'c', { fanout: 1 }),
            makePath(0.2, 'a', { fanout: 3 }),
        ]);

        headerByLabel(widget, 'Fanout').click();
        assert.deepEqual(endPins(widget), ['c', 'b', 'a']);
        assert.ok(headerByLabel(widget, 'Fanout').textContent.includes('▲'));

        headerByLabel(widget, 'Fanout').click();
        assert.deepEqual(endPins(widget), ['a', 'b', 'c']);
        assert.ok(headerByLabel(widget, 'Fanout').textContent.includes('▼'));
    });

    it('sorts string columns lexicographically', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        widget.showPaths('setup', [
            makePath(0.0, 'z'),
            makePath(0.1, 'm'),
            makePath(0.2, 'a'),
        ]);

        headerByLabel(widget, 'End').click();
        assert.deepEqual(endPins(widget), ['a', 'm', 'z']);
    });

    it('does not reorder cached static report paths when sorting', async () => {
        // In static mode the timing_report response is a shared cached
        // object; sorting must not disturb its server-side path order,
        // which overlay indices are keyed on.
        const cachedSetup = {
            paths: [makePath(0.2, 'c'), makePath(-0.1, 'a'), makePath(0.0, 'b')],
        };
        const cachedHold = { paths: [makePath(0.3, 'z'), makePath(0.1, 'y')] };
        const app = createMockApp({
            timing_report: (msg) => msg.is_setup ? cachedSetup : cachedHold,
        });
        const widget = new TimingWidget(app, () => {});
        await widget.update();

        // The widget shows sorted order...
        assert.deepEqual(endPins(widget), ['a', 'b', 'c']);
        // ...but the cached arrays keep their original order.
        assert.deepEqual(cachedSetup.paths.map(p => p.end_pin), ['c', 'a', 'b']);
        assert.deepEqual(cachedHold.paths.map(p => p.end_pin), ['z', 'y']);
    });

    it('does not reorder the array passed to showPaths', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        const paths = [makePath(0.2, 'c'), makePath(-0.1, 'a')];
        widget.showPaths('setup', paths);

        assert.deepEqual(endPins(widget), ['a', 'c']);
        assert.deepEqual(paths.map(p => p.end_pin), ['c', 'a']);
    });

    it('keeps timing_highlight indices correct after sorting', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        // Server order: c(0), b(1), a(2). Default slack sort shows a, b, c.
        widget.showPaths('setup', [
            makePath(0.2, 'c'),
            makePath(0.1, 'b'),
            makePath(0.0, 'a'),
        ]);

        app.requests.length = 0;
        widget._selectPathRow(0);  // row 0 is 'a', server index 2

        const highlight = app.requests.find(r => r.type === 'timing_highlight');
        assert.equal(highlight.path_index, 2);
    });

    it('clears the selection when the sort changes', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        widget.showPaths('setup', [makePath(0.0, 'a'), makePath(0.1, 'b')]);
        widget._selectPathRow(1);

        headerByLabel(widget, 'End').click();

        assert.equal(widget._selectedPathIndex, -1);
        assert.equal(widget._pathTable.querySelectorAll('tbody tr.selected').length, 0);
    });

    it('keeps the column resize grips after a sort re-render', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        widget.showPaths('setup', [makePath(0.0, 'a'), makePath(0.1, 'b')]);

        // All columns except the last get a resize grip.
        const gripCount = () =>
            widget._pathTable.querySelectorAll('thead .col-resize-grip').length;
        assert.equal(gripCount(), TimingWidget.PATH_COLS.length - 1);

        headerByLabel(widget, 'Fanout').click();
        assert.equal(gripCount(), TimingWidget.PATH_COLS.length - 1);
    });

    it('does not change the sort when a column resize drag ends', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        // The drag suppressor listens on document, so the widget must be
        // attached for events to propagate to it.
        document.body.appendChild(widget.element);
        widget.showPaths('setup', [
            makePath(-0.1, 'a'),
            makePath(0.0, 'b'),
        ]);

        // Simulate a resize drag on the Slack header's grip: mousedown on
        // the grip, mouseup, then the browser-generated click on the th.
        const slackTh = headerByLabel(widget, 'Slack');
        const grip = slackTh.querySelector('.col-resize-grip');
        const mouse = (type, target) => target.dispatchEvent(
            new window.MouseEvent(type, { bubbles: true, clientX: 0 }));
        mouse('mousedown', grip);
        mouse('mouseup', slackTh);
        mouse('click', slackTh);

        // Sort must still be slack ascending.
        assert.ok(headerByLabel(widget, 'Slack').textContent.includes('▲'));
        assert.deepEqual(endPins(widget), ['a', 'b']);

        // The click suppressor is one-shot: a regular click still sorts.
        headerByLabel(widget, 'Slack').click();
        assert.ok(headerByLabel(widget, 'Slack').textContent.includes('▼'));

        widget.element.remove();
    });

    it('shows tooltips on the Skew, Logic Delay and Logic Depth headers', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        widget.showPaths('setup', [makePath(0.0, 'a')]);

        const tip = widget._headerTooltip;
        const hover = (label) => headerByLabel(widget, label).dispatchEvent(
            new window.MouseEvent('mousemove',
                                  { bubbles: true, clientX: 5, clientY: 5 }));

        for (const [label, text] of [['Skew', 'arrival times'],
                                     ['Logic Delay', 'Path delay'],
                                     ['Logic Depth', 'Path instances']]) {
            hover(label);
            assert.equal(tip.style.display, 'block', label + ' tooltip shown');
            assert.ok(tip.textContent.includes(text), label + ' tooltip text');
            headerByLabel(widget, label).dispatchEvent(
                new window.MouseEvent('mouseleave'));
            assert.equal(tip.style.display, 'none', label + ' tooltip hidden');
        }

        // Columns without a tooltip don't show one.
        hover('Clock');
        assert.equal(tip.style.display, 'none');

        // A re-render (e.g. sorting) hides a visible tooltip.
        hover('Skew');
        assert.equal(tip.style.display, 'block');
        headerByLabel(widget, 'Skew').click();
        assert.equal(tip.style.display, 'none');
    });

    it('keeps the header tooltip inside the viewport', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        widget.showPaths('setup', [makePath(0.0, 'a')]);

        // Hover near the right viewport edge; the tooltip must be clamped.
        headerByLabel(widget, 'Skew').dispatchEvent(new window.MouseEvent(
            'mousemove',
            { bubbles: true, clientX: window.innerWidth - 1, clientY: 5 }));

        const tip = widget._headerTooltip;
        assert.equal(tip.style.display, 'block');
        assert.ok(parseFloat(tip.style.left) + tip.offsetWidth
                      <= window.innerWidth,
                  'tooltip fits in the viewport');
    });

    it('does not show a header tooltip while a mouse button is held', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {});
        widget.showPaths('setup', [makePath(0.0, 'a')]);

        const tip = widget._headerTooltip;
        const move = (buttons) => headerByLabel(widget, 'Skew').dispatchEvent(
            new window.MouseEvent('mousemove',
                                  { bubbles: true, clientX: 5, buttons }));

        // Moving over the header mid-drag (e.g. a column resize) hides any
        // visible tooltip instead of showing one.
        move(0);
        assert.equal(tip.style.display, 'block');
        move(1);
        assert.equal(tip.style.display, 'none');
        move(1);
        assert.equal(tip.style.display, 'none');
    });
});
