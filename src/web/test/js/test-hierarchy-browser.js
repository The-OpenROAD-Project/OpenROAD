// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// A flat design has no dbModule tree, so the browser is handed one synthesized
// from instance-name paths instead.  That happens automatically, which makes
// the labelling load-bearing: these cover that a synthesized tree is coloured
// and navigable like a real one, and that it never claims to be the netlist.

import { describe, it, beforeEach } from 'node:test';
import assert from 'node:assert/strict';
import { JSDOM } from 'jsdom';

const dom = new JSDOM('<!DOCTYPE html><html><body></body></html>');
globalThis.document = dom.window.document;

const { HierarchyBrowser } = await import('../../src/hierarchy-browser.js');

const MODULE = 0;
const LEAF_GROUP = 1;
const NAME_GROUP = 4;

// Minimal app double: the browser only reaches websocketManager and the
// two display helpers.
function makeApp(response) {
    const sent = [];
    return {
        app: {
            showDbu: false,
            getDbuPerMicron: () => 1000,
            websocketManager: {
                request: async (req) => {
                    sent.push(req);
                    if (req.type === 'module_hierarchy') {
                        return response;
                    }
                    return { ok: 1, count: 0 };
                },
            },
        },
        sent,
    };
}

function makeBrowser(response) {
    const { app, sent } = makeApp(response);
    const container = { element: document.createElement('div') };
    const browser = new HierarchyBrowser(container, app, () => {});
    return { browser, sent, container };
}

// top -> riscv -> dp, as the synthesis emits it: every colourable row is a
// NAME_GROUP, the top is group 0, and ids ascend in DFS order.
const NAME_GROUPED = {
    name_grouped: true,
    nodes: [
        {
            id: 0, parent_id: -1, inst_name: 'aes', module_name: '',
            node_kind: NAME_GROUP, odb_id: 0, color: [10, 20, 30],
            insts: 100, local_insts: 5, modules: 2, local_modules: 1,
        },
        {
            id: 1, parent_id: 0, inst_name: 'riscv', module_name: '',
            node_kind: NAME_GROUP, odb_id: 1, color: [40, 50, 60],
            insts: 95, local_insts: 0, modules: 1, local_modules: 1,
        },
        {
            id: 2, parent_id: 1, inst_name: 'dp', module_name: '',
            node_kind: NAME_GROUP, odb_id: 2, color: [70, 80, 90],
            insts: 95, local_insts: 95, modules: 0, local_modules: 0,
        },
    ],
};

const MODULE_TREE = {
    nodes: [
        {
            id: 0, parent_id: -1, inst_name: 'aes', module_name: 'aes',
            odb_id: 7, color: [10, 20, 30], insts: 100, local_insts: 5,
        },
        {
            id: 1, parent_id: 0, inst_name: 'u0', module_name: 'sub',
            odb_id: 8, color: [40, 50, 60], insts: 95, local_insts: 95,
        },
    ],
};

describe('HierarchyBrowser name-group mode', () => {
    let browser, sent;

    beforeEach(async () => {
        ({ browser, sent } = makeBrowser(NAME_GROUPED));
        await browser.update();
    });

    it('says the tree came from names, not from the netlist', () => {
        const status = browser._statusLabel.textContent;
        assert.match(status, /from instance names/);
        assert.match(status, /no module hierarchy/);
        // The top row is the design, not a recovered level.
        assert.match(status, /^2 groups/);
    });

    it('colors name groups, so the overlay is driven the same way', () => {
        // Every colourable row reaches the color map the server is sent.
        const colorReq = sent.find(r => r.type === 'set_module_colors');
        assert.ok(colorReq, 'no set_module_colors request was sent');
        const keys = colorReq.colors.split(';').map(p => p.split(':')[0]);
        assert.deepEqual(keys.sort(), ['0', '1', '2']);
    });

    it('keeps group 0 rather than dropping it as falsy', () => {
        // odb_id 0 is the top level in this mode.  A truthiness check instead
        // of a null check would silently drop it and leave the top uncolored.
        assert.ok(browser._moduleState.has(0));
    });

    it('marks the rows as inferred', () => {
        const rows = [...browser._table.querySelectorAll('tbody tr')];
        const marked = rows.filter(
            r => r.classList.contains('hierarchy-name-group'));
        assert.ok(marked.length > 0, 'no row was marked as recovered');
        for (const row of marked) {
            assert.match(row.title, /Recovered from instance names/);
        }
    });

    it('collapses below the top level, as the module tree does', () => {
        // The tree is rendered without virtualization, so the default
        // collapse is what keeps a large flat design from building a row
        // per group up front.
        const names = [...browser._table.querySelectorAll('tbody tr')]
            .map(r => r.querySelector('td').textContent);
        assert.ok(names.some(n => n.includes('aes')));
        assert.ok(names.some(n => n.includes('riscv')));
        assert.ok(!names.some(n => n.includes('dp')),
                  'depth-2 group should start collapsed');
    });

    it('reports capping when the server truncated the tree', async () => {
        const capped = {
            ...NAME_GROUPED, name_groups_capped: true,
        };
        const { browser: b } = makeBrowser(capped);
        await b.update();
        assert.match(b._statusLabel.textContent, /truncated/);
    });
});

describe('HierarchyBrowser module mode is unchanged', () => {
    it('still reports a module count', async () => {
        const { browser } = makeBrowser(MODULE_TREE);
        await browser.update();
        assert.equal(browser._statusLabel.textContent, '2 modules');
    });

    it('does not mark module rows as inferred', async () => {
        const { browser } = makeBrowser(MODULE_TREE);
        await browser.update();
        const rows = [...browser._table.querySelectorAll('tbody tr')];
        assert.equal(
            rows.filter(r => r.classList.contains('hierarchy-name-group'))
                .length,
            0);
    });

    it('colors by the real dbModule ids', async () => {
        const { browser, sent } = makeBrowser(MODULE_TREE);
        await browser.update();
        const colorReq = sent.find(r => r.type === 'set_module_colors');
        const keys = colorReq.colors.split(';').map(p => p.split(':')[0]);
        assert.deepEqual(keys.sort(), ['7', '8']);
    });

    it('leaves structural rows uncolored', async () => {
        const withLeaf = {
            nodes: [
                ...MODULE_TREE.nodes,
                {
                    id: 2, parent_id: 0, inst_name: 'Leaf instances',
                    node_kind: LEAF_GROUP, insts: 5,
                },
            ],
        };
        const { browser, sent } = makeBrowser(withLeaf);
        await browser.update();
        const colorReq = sent.find(r => r.type === 'set_module_colors');
        const keys = colorReq.colors.split(';').map(p => p.split(':')[0]);
        assert.deepEqual(keys.sort(), ['7', '8']);
        assert.equal(MODULE, 0);
    });
});
