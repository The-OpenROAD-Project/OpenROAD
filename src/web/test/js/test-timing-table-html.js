// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { renderTimingTableHTML } from '../../src/timing-table-html.js';

const SAMPLE_PATHS = {
    paths: [
        {
            start_clk: 'core_clock',
            end_clk: 'core_clock',
            required: 1.377,
            arrival: 1.5,
            slack: -0.123,
            skew: 0.012,
            path_delay: 1.2,
            logic_depth: 5,
            fanout: 3,
            start_pin: 'ctrl/state_reg[0]/CK',
            end_pin: 'datapath/result_reg[0]/D',
        },
        {
            start_clk: 'core_clock',
            end_clk: 'core_clock',
            required: 1.377,
            arrival: 1.4,
            slack: 0.023,
            skew: 0.012,
            path_delay: 1.1,
            logic_depth: 3,
            fanout: 2,
            start_pin: 'ctrl/state_reg[1]/CK',
            end_pin: 'datapath/result_reg[1]/D',
        },
    ],
};

describe('renderTimingTableHTML', () => {
    it('returns a table for valid data', () => {
        const html = renderTimingTableHTML(SAMPLE_PATHS);
        assert.ok(html.includes('<table'));
        assert.ok(html.includes('</table>'));
    });

    it('includes header row', () => {
        const html = renderTimingTableHTML(SAMPLE_PATHS);
        assert.ok(html.includes('<thead>'));
        assert.ok(html.includes('Slack'));
        assert.ok(html.includes('Path Delay'));
        assert.ok(html.includes('Start Pin'));
    });

    it('renders correct number of rows', () => {
        const html = renderTimingTableHTML(SAMPLE_PATHS);
        const rowCount = (html.match(/<tr>/g) || []).length;
        // 1 header row + 2 data rows
        assert.equal(rowCount, 3);
    });

    it('marks negative slack with class', () => {
        const html = renderTimingTableHTML(SAMPLE_PATHS);
        assert.ok(html.includes('class="negative"'), 'negative slack should have class');
    });

    it('formats time values to 4 decimal places', () => {
        const html = renderTimingTableHTML(SAMPLE_PATHS);
        assert.ok(html.includes('-0.1230'), 'slack should be formatted');
        assert.ok(html.includes('1.2000'), 'path delay should be formatted');
    });

    it('includes pin names', () => {
        const html = renderTimingTableHTML(SAMPLE_PATHS);
        assert.ok(html.includes('ctrl/state_reg[0]/CK'));
        assert.ok(html.includes('datapath/result_reg[0]/D'));
    });

    it('includes clock info', () => {
        const html = renderTimingTableHTML(SAMPLE_PATHS);
        assert.ok(html.includes('core_clock'));
    });

    it('handles empty paths', () => {
        const html = renderTimingTableHTML({ paths: [] });
        assert.ok(html.includes('No timing paths'));
    });

    it('handles null data', () => {
        const html = renderTimingTableHTML(null);
        assert.ok(html.includes('No timing paths'));
    });

    it('escapes HTML in pin names', () => {
        const data = {
            paths: [{
                start_clk: 'clk', end_clk: 'clk',
                slack: 0, required: 0, arrival: 0, path_delay: 0,
                logic_depth: 0, fanout: 0, skew: 0,
                start_pin: '<script>alert(1)</script>',
                end_pin: 'normal_pin',
            }],
        };
        const html = renderTimingTableHTML(data);
        assert.ok(!html.includes('<script>'), 'should escape HTML');
        assert.ok(html.includes('&lt;script&gt;'));
    });
});
