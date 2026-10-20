// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Resizable table columns (ui-utils.js makeResizableHeaders), shared by the
// timing, hierarchy and selection tables.

import './setup-dom.js';
import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { makeResizableHeaders, textColumnWidths, kMaxAutoColumnPx }
    from '../../src/ui-utils.js';

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

    it('reads a width in other units from the layout, not as pixels', () => {
        const table = table3(['100px', '200px', '50px']);
        const ths = table.querySelectorAll('thead th');
        ths[0].style.width = '30ch';
        Object.defineProperty(ths[0], 'offsetWidth', { value: 240 });
        table.fitColumns();
        assert.equal(table.style.width, '490px');
    });

    it('widens the last column too, and keeps it when the header is rebuilt', () => {
        const table = table3(['100px', '200px', '50px']);
        let ths = table.querySelectorAll('thead th');
        Object.defineProperty(ths[2], 'offsetWidth', { value: 50 });
        const grip = ths[2].querySelector('.col-resize-grip');
        assert.ok(grip, 'the last column has a grip');
        grip.dispatchEvent(new window.MouseEvent('mousedown', { clientX: 0, bubbles: true }));
        document.dispatchEvent(new window.MouseEvent('mousemove', { clientX: 250 }));
        document.dispatchEvent(new window.MouseEvent('mouseup', {}));
        assert.equal(ths[2].style.width, '');
        assert.equal(table.style.width, '600px');
        // a re-render rebuilds the header and passes the saved widths back
        ths = table.querySelectorAll('thead th');
        const saved = Array.from(ths, (th) => th.style.width);
        table.querySelector('thead').remove();
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
});

describe('textColumnWidths', () => {
    it('sizes columns from their text, with no layout, a long name capped', () => {
        const table = document.createElement('table');
        table.innerHTML = '<thead><tr><th>Clock</th><th>Slack</th><th>End</th></tr></thead>'
            + '<tbody><tr><td>clock</td><td>-0.1234</td><td>'
            + 'core/backend/inner_ctrlBlock/'.repeat(20) + 'x_reg/D</td></tr>'
            + '<tr><td colspan="3">a message row</td></tr></tbody>';
        // not attached to a document: every offsetWidth is 0
        const w = textColumnWidths(table);
        assert.equal(w.length, 3);
        assert.ok(w[0] > 24 && w[1] > w[0], `widths ${w}`);
        assert.equal(w[2], kMaxAutoColumnPx);
    });
});
