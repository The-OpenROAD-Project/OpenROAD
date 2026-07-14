// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it, beforeEach } from 'node:test';
import assert from 'node:assert/strict';
import { SelectionBrowser } from '../../src/selection-browser.js';

const PALETTE = Array.from({ length: 16 }, (_, i) => [i, 2 * i, 3 * i, 100]);

const LIST = {
    ok: 1,
    truncated: false,
    selection: [
        { name: 'inst_a', type: 'Inst', bbox: [0, 0, 100, 100] },
        { name: 'net_b', type: 'Net', bbox: [10, 10, 50, 50] },
    ],
    groups: Array.from({ length: 16 }, (_, g) =>
        (g === 4 ? [{ name: 'inst_c', type: 'Inst',
                      bbox: [5, 5, 20, 20] }] : [])),
};

function createMockApp(responses = {}) {
    const requests = [];
    return {
        websocketManager: {
            request(msg) {
                requests.push(msg);
                const type = msg.type;
                if (responses[type]) {
                    return Promise.resolve(responses[type](msg));
                }
                return Promise.resolve({ ok: 1 });
            },
        },
        techData: { highlight_colors: PALETTE },
        designScale: 1,
        designMaxDXDY: 100000,
        designOriginX: 0,
        designOriginY: 0,
        showDbu: true,
        getDbuPerMicron() { return 1000; },
        map: { fitBounds() {} },
        focusComponent() {},
        updateInspector() {},
        _requests: requests,
    };
}

function makeContainer() {
    return { element: document.createElement('div'), on() {} };
}

const settle = () => new Promise(r => setTimeout(r, 0));

describe('SelectionBrowser', () => {
    let app, browser, container;

    beforeEach(async () => {
        app = createMockApp({ list_selection: () => LIST });
        container = makeContainer();
        browser = new SelectionBrowser(container, app, () => {});
        await settle();
    });

    it('renders tabs with live counters', () => {
        const tabs = container.element.querySelectorAll('.sb-tab');
        assert.equal(tabs.length, 2);
        assert.equal(tabs[0].textContent, 'Selected (2)');
        assert.equal(tabs[1].textContent, 'Highlighted (1)');
    });

    it('lists selection rows with bounds and actions', () => {
        const rows = container.element.querySelectorAll('.sb-row');
        assert.equal(rows.length, 2);
        assert.ok(rows[0].textContent.includes('inst_a'));
        assert.ok(rows[0].querySelector('.sb-row-btn[title="De-select"]'));
    });

    it('filters by substring across name and type', () => {
        const filter = container.element.querySelector('.sb-filter');
        filter.value = 'net';
        filter.dispatchEvent(new window.Event('input'));
        const rows = container.element.querySelectorAll('.sb-row');
        assert.equal(rows.length, 1);
        assert.ok(rows[0].textContent.includes('net_b'));
    });

    it('row click sends inspect_selection with the stable index', async () => {
        app._requests.length = 0;
        container.element.querySelectorAll('.sb-row')[1].click();
        await settle();
        assert.equal(app._requests[0].type, 'inspect_selection');
        assert.equal(app._requests[0].index, 1);
    });

    it('de-select button sends deselect and refreshes', async () => {
        app._requests.length = 0;
        container.element
            .querySelector('.sb-row-btn[title="De-select"]').click();
        await settle();
        assert.equal(app._requests[0].type, 'deselect');
        assert.equal(app._requests[0].index, 0);
        assert.ok(app._requests.some(r => r.type === 'list_selection'),
                  'refreshes the list after removal');
    });

    it('highlighted tab shows the group chip and inspects by group', async () => {
        const tabs = container.element.querySelectorAll('.sb-tab');
        tabs[1].click();
        const chip = container.element.querySelector('.sb-chip');
        assert.ok(chip, 'group color chip rendered');
        const c = PALETTE[4];
        assert.equal(chip.style.backgroundColor,
                     `rgb(${c[0]}, ${c[1]}, ${c[2]})`);
        assert.ok(container.element.textContent.includes('Group 5'));

        app._requests.length = 0;
        container.element.querySelector('.sb-row').click();
        await settle();
        assert.equal(app._requests[0].type, 'inspect_group');
        assert.equal(app._requests[0].group, 4);
        assert.equal(app._requests[0].index, 0);
    });

    it('sorting keeps actions on the right object', async () => {
        // Sort by name descending: net_b first, inst_a second.
        const nameTh = container.element.querySelector('th.sb-sortable');
        nameTh.click();  // asc
        nameTh.click();  // desc
        const rows = container.element.querySelectorAll('.sb-row');
        assert.ok(rows[0].textContent.includes('net_b'));

        app._requests.length = 0;
        rows[0].click();
        await settle();
        // net_b is index 1 in fetch order — the sort must not change that.
        assert.equal(app._requests[0].index, 1);
    });

    it('shows the truncation notice when the server capped the list', async () => {
        app = createMockApp({
            list_selection: () => ({ ...LIST, truncated: true }),
        });
        container = makeContainer();
        browser = new SelectionBrowser(container, app, () => {});
        await settle();
        assert.ok(container.element.querySelector('.sb-truncated'));
    });

    it('row hover starts a continuous animation and mouseleave stops it', () => {
        const calls = [];
        app.animateSelection = (bbox, opts) => calls.push(['start', bbox, opts]);
        app.stopSelectionAnimation = () => calls.push(['stop']);

        const row = container.element.querySelector('tbody tr');
        row.dispatchEvent(new Event('mouseenter'));
        assert.deepEqual(calls[0],
                         ['start', [0, 0, 100, 100], { repeats: 0 }]);
        row.dispatchEvent(new Event('mouseleave'));
        assert.deepEqual(calls[1], ['stop']);
    });
});
