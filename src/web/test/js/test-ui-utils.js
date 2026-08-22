// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { makeResizableHeaders } from '../../src/ui-utils.js';

function makeTable(columns) {
    const table = document.createElement('table');
    const thead = document.createElement('thead');
    const tr = document.createElement('tr');
    for (const name of columns) {
        const th = document.createElement('th');
        th.textContent = name;
        tr.appendChild(th);
    }
    thead.appendChild(tr);
    table.appendChild(thead);
    return table;
}

describe('makeResizableHeaders', () => {
    // A table rendered inside a hidden view measures every column at zero.
    // Locking that in switches the table to `fixed` with `width: 0px` headers,
    // which collapses the columns until something re-renders it while visible.
    //
    // jsdom does no layout, so every table here measures zero — the state the
    // guard is for.  It cannot prove the visible case; only a browser can.
    it('does not lock widths measured on a table with no layout', () => {
        const table = makeTable(['Name', 'Insts', 'Area']);
        makeResizableHeaders(table);
        assert.notEqual(table.style.tableLayout, 'fixed');
        for (const th of table.querySelectorAll('thead th')) {
            assert.notEqual(th.style.width, '0px');
        }
    });

    it('applies the widths a caller saved from a previous render', () => {
        const table = makeTable(['Name', 'Insts']);
        makeResizableHeaders(table, ['120px', '60px']);
        const headers = table.querySelectorAll('thead th');
        assert.equal(table.style.tableLayout, 'fixed');
        assert.equal(headers[0].style.width, '120px');
        assert.equal(headers[1].style.width, '60px');
    });

    // One grip per header except the last, which has nothing to its right.
    it('adds a resize grip to every header but the last', () => {
        const table = makeTable(['Name', 'Insts', 'Area']);
        makeResizableHeaders(table, ['120px', '60px', '60px']);
        assert.equal(table.querySelectorAll('.col-resize-grip').length, 2);
    });
});
