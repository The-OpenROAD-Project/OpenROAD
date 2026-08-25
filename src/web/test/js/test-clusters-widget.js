// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { waitForMicrotasks } from './setup-dom.js';
import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { ClustersWidget } from '../../src/clusters-widget.js';

// A two-level MPL-style cluster tree: root → two child clusters, one of
// which has a grandchild.
const NODES = [
    {
        id: 0, parent_id: -1, name: 'root', type: 'VISUAL_DEBUG', odb_id: 10,
        insts: 300, macros: 1, groups: 3, area: 12.5,
        local_insts: 0, local_macros: 0, local_groups: 2,
        bbox: [0, 0, 1000, 1000], color: [255, 0, 0],
    },
    {
        id: 1, parent_id: 0, name: '(root)_glue_logic', type: 'VISUAL_DEBUG',
        odb_id: 11, insts: 200, macros: 0, groups: 1, area: 8.0,
        local_insts: 100, local_macros: 0, local_groups: 1,
        bbox: [0, 0, 500, 1000], color: [0, 255, 0],
    },
    {
        id: 2, parent_id: 1, name: '(root)_glue_logic_leaf',
        type: 'VISUAL_DEBUG', odb_id: 12, insts: 100, macros: 0, groups: 0,
        area: 4.0, local_insts: 100, local_macros: 0, local_groups: 0,
        bbox: [0, 0, 250, 1000], color: [0, 0, 255],
    },
    {
        id: 3, parent_id: 0, name: 'macro_cluster', type: 'VISUAL_DEBUG',
        odb_id: 13, insts: 100, macros: 1, groups: 0, area: 4.5,
        local_insts: 100, local_macros: 1, local_groups: 0,
        bbox: [500, 0, 1000, 1000], color: [255, 255, 0],
    },
];

function createMockApp({ ...overrides } = {}) {
    const sent = [];
    const app = {
        sent,
        websocketManager: {
            readyPromise: Promise.resolve(),
            request(msg) {
                sent.push(msg);
                if (msg.type === 'group_hierarchy') {
                    return Promise.resolve({ nodes: NODES });
                }
                return Promise.resolve({ ok: 1, count: 0 });
            },
        },
        designScale: 1,
        designMaxDXDY: 1000,
        designOriginX: 0,
        designOriginY: 0,
        showDbu: false,
        getDbuPerMicron() { return 1000; },
        map: { fitBounds(bounds) { app.fitted = bounds; } },
        ...overrides,
    };
    return app;
}

function makeContainer() {
    return { element: document.createElement('div') };
}

function rowNames(widget) {
    return [...widget._table.querySelectorAll('tbody tr')]
        .map(tr => tr.children[0].textContent);
}

describe('ClustersWidget', () => {
    it('creates the expected DOM structure', () => {
        const app = createMockApp();
        const widget = new ClustersWidget(makeContainer(), app, () => {});

        assert.ok(widget._table);
        assert.equal(widget._updateBtn.textContent, 'Update');
    });

    // Closing the Hierarchy tab hands the overlay back: the flag stays as the
    // user left it, and an empty map is what stops the layer drawing.
    it('clears its color map on the way out', async () => {
        const app = createMockApp();
        const widget = new ClustersWidget(makeContainer(), app, () => {});
        await widget.update();
        assert.notEqual(
            app.sent.filter(m => m.type === 'set_group_colors').at(-1).colors,
            '', 'painting something to begin with');

        await widget.clearOverlay();

        assert.equal(
            app.sent.filter(m => m.type === 'set_group_colors').at(-1).colors,
            '');
    });

    // Straight from the constructor, with no _render() of our own: the panel has
    // to say what it wants as soon as it is opened.
    it('asks for an Update before anything is loaded', () => {
        const app = createMockApp();
        const widget = new ClustersWidget(makeContainer(), app, () => {});
        const body = widget._table.querySelector('tbody').textContent;
        assert.match(body, /Click "Update"/);
        assert.doesNotMatch(body, /has no instance groups/);
        assert.ok(widget._table.querySelector('thead th'),
                  'column headers are drawn before the first load');
    });

    // The common case for an ODB from a normal flow run: the load worked and
    // the design simply has no dbGroups.  Repeating "click Update" there told
    // the user to do what they had just done.
    it('says the design has no instance groups once an empty load returns',
       async () => {
           const app = createMockApp();
           app.websocketManager.request = (msg) => {
               app.sent.push(msg);
               if (msg.type === 'group_hierarchy') {
                   return Promise.resolve({ nodes: [] });
               }
               return Promise.resolve({ ok: 1 });
           };
           const widget = new ClustersWidget(makeContainer(), app, () => {});
           await widget.update();

           assert.equal(widget._statusLabel.textContent, '0 groups');
           const body = widget._table.querySelector('tbody').textContent;
           assert.match(body, /has no instance groups/);
           assert.match(body, /keep_clustering_data/);
           assert.doesNotMatch(body, /Click "Update"/);
       });

    it('loads the tree and collapses non-root clusters by default', async () => {
        const app = createMockApp();
        const widget = new ClustersWidget(makeContainer(), app, () => {});
        await widget.update();

        assert.equal(widget._statusLabel.textContent, '4 groups');
        // root expanded, its children visible; the grandchild stays hidden
        // because '(root)_glue_logic' has children and is not the root.
        const names = rowNames(widget);
        assert.ok(names.some(n => n.includes('root')));
        assert.ok(names.some(n => n.includes('(root)_glue_logic')));
        assert.ok(names.some(n => n.includes('macro_cluster')));
        assert.ok(!names.some(n => n.includes('_leaf')),
                  'collapsed child cluster hides its subtree');
    });

    it('makes a collapsed cluster lend its color to its subtree', async () => {
        const app = createMockApp();
        const widget = new ClustersWidget(makeContainer(), app, () => {});
        await widget.update();

        // odb_id 12 is under the collapsed odb_id 11, so it paints green.
        assert.deepEqual(widget._groupState.get(12).effectiveColor,
                         [0, 255, 0]);
        // Expanding the parent gives the child its own color back.
        widget._toggleNode(1);
        assert.deepEqual(widget._groupState.get(12).effectiveColor,
                         [0, 0, 255]);
    });

    it('sends the effective colors and skips hidden clusters', async () => {
        const app = createMockApp();
        const widget = new ClustersWidget(makeContainer(), app, () => {});
        await widget.update();

        const colorMsgs = app.sent.filter(m => m.type === 'set_group_colors');
        assert.ok(colorMsgs.length >= 1);
        const parts = colorMsgs.at(-1).colors.split(';');
        assert.equal(parts.length, 4);
        assert.ok(parts.includes('11:0,255,0,100'));

        // Unchecking the root drops every cluster from the color map, since the
        // checkbox model propagates to descendants.  Driven through the model so
        // that the widget's own onChange runs.
        widget._checkModel.check(0, false);
        await waitForMicrotasks();
        const last = app.sent.filter(m => m.type === 'set_group_colors').at(-1);
        assert.equal(last.colors, '');
    });

    // Ticking a checkbox changes checkbox states and nothing else, so the rows
    // must survive it: rebuilding the table per click cost 14 ms at 100 rows and
    // 530 ms at 5000, all of it DOM churn and listener re-registration.
    it('updates checkboxes in place instead of rebuilding the rows', async () => {
        const app = createMockApp();
        const widget = new ClustersWidget(makeContainer(), app, () => {});
        await widget.update();

        const rowsBefore = [...widget._table.querySelectorAll('tbody tr')];
        const rootCb = rowsBefore[0].querySelector('input[type=checkbox]');
        const childCb = rowsBefore[1].querySelector('input[type=checkbox]');
        assert.ok(rootCb.checked && childCb.checked);

        rootCb.checked = false;
        rootCb.dispatchEvent(new window.Event('change', { bubbles: true }));
        await waitForMicrotasks();

        const rowsAfter = [...widget._table.querySelectorAll('tbody tr')];
        assert.deepEqual(rowsAfter, rowsBefore,
                         'the same <tr> elements must still be in the table');
        // The uncheck still propagated to the descendant's checkbox and to the
        // server.
        assert.equal(childCb.checked, false);
        assert.equal(
            app.sent.filter(m => m.type === 'set_group_colors').at(-1).colors,
            '');
    });

    // Ticking a cluster paints nothing while the `_clusters` overlay is off,
    // which reads as a broken panel unless it says so.
    it('warns when the cluster overlay is off, and clears it again', async () => {
        const app = createMockApp({ visibility: { cluster_view: false } });
        const widget = new ClustersWidget(makeContainer(), app, () => {});
        await widget.update();

        widget._checkModel.check(3, false);
        await waitForMicrotasks();
        // Names the one checkbox Display Controls has for both overlays, not
        // the flag: the user has no "Cluster view" row to go and find.
        assert.match(widget._statusLabel.textContent, /Hierarchy view is off/);

        // Nothing else writes this label, so the warning has to take itself
        // down once the overlay is back.
        app.visibility.cluster_view = true;
        widget._checkModel.check(3, true);
        await waitForMicrotasks();
        assert.equal(widget._statusLabel.textContent, '4 groups');
    });

    // The overlay is turned on from Display Controls and from the Source
    // dropdown, neither of which touches a checkbox here; HierarchyPanel calls
    // this so the warning does not outlive what it is warning about.
    it('refreshStatus follows the flag with no checkbox involved', async () => {
        const app = createMockApp({ visibility: { cluster_view: true } });
        const widget = new ClustersWidget(makeContainer(), app, () => {});
        await widget.update();
        assert.equal(widget._statusLabel.textContent, '4 groups');

        app.visibility.cluster_view = false;
        widget.refreshStatus();
        assert.match(widget._statusLabel.textContent, /Hierarchy view is off/);

        app.visibility.cluster_view = true;
        widget.refreshStatus();
        assert.equal(widget._statusLabel.textContent, '4 groups');
    });

    // "0 groups" would talk over the table's own "click Update to load" line.
    it('refreshStatus says nothing before the first load', () => {
        const app = createMockApp({ visibility: { cluster_view: false } });
        const widget = new ClustersWidget(makeContainer(), app, () => {});

        widget.refreshStatus();
        assert.equal(widget._statusLabel.textContent, '');
    });

    // The row used to select its cluster and isolate it: the color map was
    // narrowed to that subtree, so every other ticked cluster stopped painting.
    // Clicking a row does nothing at all now, as in the Verilog Modules view.
    it('leaves every ticked cluster painted when a row is clicked', async () => {
        const app = createMockApp();
        const widget = new ClustersWidget(makeContainer(), app, () => {});
        await widget.update();
        const before = app.sent.filter(m => m.type === 'set_group_colors')
                          .at(-1).colors;
        assert.equal(before.split(';').length, 4, 'all four start out painted');

        for (const tr of widget._table.querySelectorAll('tbody tr')) {
            tr.dispatchEvent(new window.Event('click', { bubbles: true }));
        }
        await waitForMicrotasks();

        assert.deepEqual(app.sent.filter(m => m.type === 'select_group'), []);
        assert.equal(app.sent.filter(m => m.type === 'set_group_colors')
                         .at(-1).colors, before);
        assert.equal(widget._table.querySelector('tr.selected'), null);
    });

    it('zooms to a cluster bbox on double click and ignores empty ones',
       async () => {
           const app = createMockApp();
           const widget = new ClustersWidget(makeContainer(), app, () => {});
           await widget.update();

           widget._zoomToNode(widget._nodeMap.get(3));
           assert.ok(app.fitted, 'fitBounds called for a real bbox');

           app.fitted = null;
           widget._zoomToNode({ bbox: [0, 0, 0, 0] });
           assert.equal(app.fitted, null, 'empty bbox is ignored');
       });
});
