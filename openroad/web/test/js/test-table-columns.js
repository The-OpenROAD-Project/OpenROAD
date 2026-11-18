// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Resizable table columns (ui-utils.js makeResizableHeaders), shared by the
// timing, hierarchy and selection tables.

import './setup-dom.js';
import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { ColumnWidths, makeResizableHeaders, textColumnWidths,
         kMaxAutoColumnPx } from '../../src/ui-utils.js';

function addHeader(table, names) {
    const thead = document.createElement('thead');
    const tr = document.createElement('tr');
    for (const name of names) {
        const th = document.createElement('th');
        th.textContent = name;
        tr.appendChild(th);
    }
    thead.appendChild(tr);
    table.appendChild(thead);
}

function addRow(table, cells) {
    let tbody = table.querySelector('tbody');
    if (!tbody) {
        tbody = document.createElement('tbody');
        table.appendChild(tbody);
    }
    const tr = document.createElement('tr');
    for (const text of cells) {
        const td = document.createElement('td');
        td.textContent = text;
        tr.appendChild(td);
    }
    tbody.appendChild(tr);
}

function drag(th, from, to) {
    th.querySelector('.col-resize-grip').dispatchEvent(
        new window.MouseEvent('mousedown', { clientX: from, bubbles: true }));
    document.dispatchEvent(new window.MouseEvent('mousemove', { clientX: to }));
    document.dispatchEvent(new window.MouseEvent('mouseup', {}));
}

describe('makeResizableHeaders', () => {
    function table3(dragged) {
        const columns = new ColumnWidths();
        columns.dragged = dragged || null;
        const table = document.createElement('table');
        addHeader(table, ['Start', 'End', 'Slack']);
        document.body.appendChild(table);
        makeResizableHeaders(table, columns);
        return { table, columns };
    }

    it('widens the table when a column is dragged wider, not the others narrower', () => {
        const { table } = table3([100, 200, 50]);
        const ths = table.querySelectorAll('thead th');
        // jsdom has no layout: the dragged column reports its styled width
        Object.defineProperty(ths[0], 'offsetWidth', { value: 100 });
        drag(ths[0], 0, 400);
        assert.equal(ths[0].style.width, '500px');
        assert.equal(ths[1].style.width, '200px');
        assert.equal(table.style.width, '750px');
        // widths include the padding, so the columns sum to the table width
        assert.equal(ths[0].style.boxSizing, 'border-box');
    });

    it('widens the last column too, and keeps it when the table is rebuilt', () => {
        const { table, columns } = table3([100, 200, 50]);
        const ths = table.querySelectorAll('thead th');
        Object.defineProperty(ths[2], 'offsetWidth', { value: 50 });
        drag(ths[2], 0, 250);
        assert.equal(ths[2].style.width, '');
        assert.equal(table.style.width, '600px');
        assert.deepEqual(columns.dragged, [100, 200, 300]);
        // a re-render empties the table; the widths live on in `columns`
        table.innerHTML = '';
        addHeader(table, ['Start', 'End', 'Slack']);
        makeResizableHeaders(table, columns);
        assert.equal(table.style.width, '600px');
    });

    it('keeps no dragged widths until a column is dragged', () => {
        const { columns } = table3();
        assert.equal(columns.dragged, null);
        assert.equal(columns.auto.length, 3);
    });

    it('measures again when the dragged widths are for other columns', () => {
        const { table, columns } = table3([100, 200]);
        assert.notEqual(table.querySelector('thead th').style.width, '100px');
        // the other table's widths stay for when it is shown again
        assert.deepEqual(columns.dragged, [100, 200]);
    });

    it('does not narrow a measured column on a re-render, until reset', () => {
        const columns = new ColumnWidths();
        const table = document.createElement('table');
        table.style.fontSize = '10px';
        const render = (name) => {
            table.innerHTML = '';
            addHeader(table, ['Name', 'Slack']);
            addRow(table, [name, '0.1']);
            makeResizableHeaders(table, columns);
            return table.querySelector('thead th').style.width;
        };
        // 20 characters at 6 px plus 24 px of padding and grip
        assert.equal(render('a'.repeat(20)), '144px');
        assert.equal(render('b'), '144px');
        columns.reset();
        assert.equal(render('b'), '48px');  // 'Name' is the widest
    });

    it('shows an elided cell in full as its tooltip', () => {
        const table = document.createElement('table');
        addHeader(table, ['Name']);
        addRow(table, ['core/u_alu/add_reg/D']);
        document.body.appendChild(table);
        makeResizableHeaders(table);
        const td = table.querySelector('td');
        Object.defineProperty(td, 'scrollWidth', { value: 200 });
        Object.defineProperty(td, 'clientWidth', { value: 100 });
        td.dispatchEvent(new window.MouseEvent('mouseover', { bubbles: true }));
        assert.equal(td.title, 'core/u_alu/add_reg/D');
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

    it("adds a cell's indent and its checkbox and swatch", () => {
        const table = document.createElement('table');
        table.innerHTML = '<thead><tr><th>Instance</th></tr></thead><tbody><tr>'
            + '<td style="padding-left: 72px"><input type="checkbox">'
            + '<span></span><span>u1</span></td></tr></tbody>';
        table.style.fontSize = '10px';
        // 'Instance' is 8 characters (48 px); the row is 2 characters plus a
        // 72 px indent and two 16 px widgets
        assert.deepEqual(textColumnWidths(table), [12 + 72 + 32 + 24]);
    });
});
