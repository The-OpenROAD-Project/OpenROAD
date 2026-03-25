// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Pure HTML renderer for timing path tables.
// Takes timing report JSON and produces an HTML <table> string.

import { fmtTime } from './timing-widget.js';

function escapeHtmlTable(s) {
    return String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;')
        .replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}

// Render timing paths as an HTML table string.
//
// timingData: { paths: [{start_clk, end_clk, slack, arrival, required,
//               path_delay, logic_depth, fanout, start_pin, end_pin}, ...] }
export function renderTimingTableHTML(timingData) {
    const paths = timingData?.paths;
    if (!paths || paths.length === 0) {
        return '<p style="color:#888">No timing paths available.</p>';
    }

    const rows = [];
    rows.push('<table class="timing-table">');
    rows.push('<thead><tr>');
    rows.push('<th>#</th>');
    rows.push('<th>Slack</th>');
    rows.push('<th>Path Delay</th>');
    rows.push('<th>Required</th>');
    rows.push('<th>Arrival</th>');
    rows.push('<th>Depth</th>');
    rows.push('<th>Fanout</th>');
    rows.push('<th>Start Pin</th>');
    rows.push('<th>End Pin</th>');
    rows.push('<th>Clock</th>');
    rows.push('</tr></thead>');
    rows.push('<tbody>');

    for (let i = 0; i < paths.length; i++) {
        const p = paths[i];
        const slackClass = p.slack < 0 ? ' class="negative"' : '';
        rows.push('<tr>');
        rows.push(`<td>${i + 1}</td>`);
        rows.push(`<td${slackClass}>${fmtTime(p.slack)}</td>`);
        rows.push(`<td>${fmtTime(p.path_delay)}</td>`);
        rows.push(`<td>${fmtTime(p.required)}</td>`);
        rows.push(`<td>${fmtTime(p.arrival)}</td>`);
        rows.push(`<td>${p.logic_depth ?? ''}</td>`);
        rows.push(`<td>${p.fanout ?? ''}</td>`);
        rows.push(`<td>${escapeHtmlTable(p.start_pin)}</td>`);
        rows.push(`<td>${escapeHtmlTable(p.end_pin)}</td>`);
        rows.push(`<td>${escapeHtmlTable(p.start_clk)}→${escapeHtmlTable(p.end_clk)}</td>`);
        rows.push('</tr>');
    }

    rows.push('</tbody></table>');
    return rows.join('\n');
}
