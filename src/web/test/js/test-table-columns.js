// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Resizable table columns (ui-utils.js makeResizableHeaders), shared by the
// timing, hierarchy and selection tables.

import './setup-dom.js';
import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { makeResizableHeaders, savedColumnWidths, textColumnWidths,
         kMaxAutoColumnPx } from '../../src/ui-utils.js';

describe('makeResizableHeaders', () => {
    function table3(widths) {
        const table = document.createElement('table');
        const thead = document.createElement('thead');
        const tr = document.createElement('tr');
        for (const name of ['Start', 'End', 'Slack']) {
            const th = document.createElement('th');
            th.textContent = name;
            tr.appendChild(th);
        }
        thead.appendChild(tr);
        table.appendChild(thead);
        document.body.appendChild(table);
        makeResizableHeaders(table, widths);
        return table;
    }

    it('widens the table when a column is dragged wider, not the others narrower', () => {
        const table = table3(['100px', '200px', '50px']);
        const ths = table.querySelectorAll('thead th');
        // jsdom has no layout: the dragged column reports its styled width
        Object.defineProperty(ths[0], 'offsetWidth', { value: 100 });
        const grip = ths[0].querySelector('.col-resize-grip');
        grip.dispatchEvent(new window.MouseEvent('mousedown', { clientX: 0, bubbles: true }));
        document.dispatchEvent(new window.MouseEvent('mousemove', { clientX: 400 }));
        document.dispatchEvent(new window.MouseEvent('mouseup', {}));
        assert.equal(ths[0].style.width, '500px');
        assert.equal(ths[1].style.width, '200px');
        assert.equal(table.style.width, '750px');
    });

    it('widens the last column too, and keeps it when the header is rebuilt', () => {
        const table = table3(['100px', '200px', '50px']);
        const ths = table.querySelectorAll('thead th');
        Object.defineProperty(ths[2], 'offsetWidth', { value: 50 });
        const grip = ths[2].querySelector('.col-resize-grip');
        assert.ok(grip, 'the last column has a grip');
        grip.dispatchEvent(new window.MouseEvent('mousedown', { clientX: 0, bubbles: true }));
        document.dispatchEvent(new window.MouseEvent('mousemove', { clientX: 250 }));
        document.dispatchEvent(new window.MouseEvent('mouseup', {}));
        assert.equal(ths[2].style.width, '');
        assert.equal(table.style.width, '600px');
        // a re-render empties the table and passes the saved widths back
        const saved = savedColumnWidths(table);
        assert.deepEqual(saved, ['100px', '200px', '300px']);
        table.innerHTML = '';
        assert.deepEqual(savedColumnWidths(table), saved);
        const thead = document.createElement('thead');
        const tr = document.createElement('tr');
        for (const name of ['Start', 'End', 'Slack']) {
            const th = document.createElement('th');
            th.textContent = name;
            tr.appendChild(th);
        }
        thead.appendChild(tr);
        table.appendChild(thead);
        makeResizableHeaders(table, saved);
        assert.equal(table.style.width, '600px');
    });

    it('passes no widths on until a column is dragged', () => {
        const table = table3();
        assert.equal(savedColumnWidths(table), undefined);
        assert.equal(savedColumnWidths(null), undefined);
    });

    it('measures again when the saved widths are for other columns', () => {
        const table = table3(['100px', '200px']);
        assert.equal(savedColumnWidths(table), undefined);
        assert.notEqual(table.querySelector('thead th').style.width, '100px');
    });
});

describe('textColumnWidths', () => {
    it('sizes columns from their text, with no layout, a long name capped', () => {
        const table = document.createElement('table');
        table.innerHTML = '<thead><tr><th>Clock</th><th>Slack</th><th>End</th></tr></thead>'
            + '<tbody><tr><td>clock</td><td>-0.1234</td><td>'
            + 'core/backend/inner_ctrlBlock/'.repeat(20) + 'x_reg/D</td></tr>'
            + '<tr><td colspan="3">a message row</td></tr></tbody>';
        table.style.fontSize = '10px';
        // not attached to a document: every offsetWidth is 0
        const w = textColumnWidths(table);
        // 0.6 em a character plus 24 px of padding and grip: 'Clock' and
        // 'clock' are 5 characters, '-0.1234' 7; the message row is skipped
        assert.deepEqual(w, [5 * 6 + 24, 7 * 6 + 24, kMaxAutoColumnPx]);
    });
});
