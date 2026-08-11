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
                if (msg.type === 'select_group') {
                    return Promise.resolve({
                        name: 'root', type: 'Group', properties: [],
                        highlight_truncated: false,
                    });
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
        updateInspector(data) { app.inspected = data; },
        refreshOverlay() { app.overlayRefreshed = true; },
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
        // Registers itself on the app so other panels can reach it.
        assert.equal(app.clustersWidget, widget);
    });

    // Straight from the constructor, with no _render() of our own: the panel has
    // to say what it wants as soon as it is opened.  Rendering it by hand here
    // is what used to hide the fact that a freshly built panel drew nothing at
    // all — not even the column headers.
    it('asks for an Update before anything is loaded', () => {
        const app = createMockApp();
        const widget = new ClustersWidget(makeContainer(), app, () => {});
        const body = widget._table.querySelector('tbody').textContent;
        assert.match(body, /Click "Update"/);
        assert.doesNotMatch(body, /has no clusters/);
        assert.ok(widget._table.querySelector('thead th'),
                  'column headers are drawn before the first load');
    });

    // The common case for an ODB from a normal flow run: the load worked and
    // the design simply has no dbGroups.  Repeating "click Update" there told
    // the user to do what they had just done.
    it('says the design has no clusters once an empty load returns',
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

           assert.equal(widget._statusLabel.textContent, '0 clusters');
           const body = widget._table.querySelector('tbody').textContent;
           assert.match(body, /has no clusters/);
           assert.match(body, /keep_clustering_data/);
           assert.doesNotMatch(body, /Click "Update"/);
       });

    it('loads the tree and collapses non-root clusters by default', async () => {
        const app = createMockApp();
        const widget = new ClustersWidget(makeContainer(), app, () => {});
        await widget.update();

        assert.equal(widget._statusLabel.textContent, '4 clusters');
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

        // Unchecking the root drops every cluster from the color map (the
        // checkbox model propagates to descendants).  Driven through the model
        // so the widget's own onChange runs — copying that callback into the
        // test would let it pass with the widget's wiring broken.
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

    it('selects the dbGroup when a row is clicked', async () => {
        const app = createMockApp();
        const widget = new ClustersWidget(makeContainer(), app, () => {});
        await widget.update();

        const tr = widget._table.querySelector('tbody tr');
        tr.dispatchEvent(new window.Event('click', { bubbles: true }));
        await waitForMicrotasks();

        const select = app.sent.find(m => m.type === 'select_group');
        assert.ok(select, 'select_group request issued');
        assert.equal(select.odb_id, 10);
        assert.ok(tr.classList.contains('selected'));
        assert.equal(app.inspected.type, 'Group');
        assert.ok(app.overlayRefreshed);
    });

    // The panel is the only place a cluster selection can be undone from, so
    // the two gestures that mean "stop showing this" must both release the
    // highlight: clicking the selected row again, and hiding the cluster.
    it('deselects when the selected row is clicked again', async () => {
        const app = createMockApp();
        const widget = new ClustersWidget(makeContainer(), app, () => {});
        await widget.update();

        const tr = widget._table.querySelector('tbody tr');
        tr.dispatchEvent(new window.Event('click', { bubbles: true }));
        await waitForMicrotasks();
        assert.ok(tr.classList.contains('selected'));

        tr.dispatchEvent(new window.Event('click', { bubbles: true }));
        await waitForMicrotasks();

        const deselect = app.sent.filter(m => m.type === 'select_group')
                            .find(m => m.deselect);
        assert.ok(deselect, 'deselect request issued');
        assert.equal(deselect.odb_id, 10);
        assert.equal(tr.classList.contains('selected'), false);
    });

    it('deselects the cluster when it is hidden', async () => {
        const app = createMockApp();
        const widget = new ClustersWidget(makeContainer(), app, () => {});
        await widget.update();

        // Select the (root) cluster, then untick it.
        widget._table.querySelector('tbody tr')
            .dispatchEvent(new window.Event('click', { bubbles: true }));
        await waitForMicrotasks();
        assert.equal(widget._selectedOdbId, 10);

        widget._checkModel.check(0, false);
        await waitForMicrotasks();

        assert.ok(app.sent.some(m => m.type === 'select_group' && m.deselect
                                     && m.odb_id === 10),
                  'hiding the selected cluster releases its highlight');
        assert.equal(widget._selectedOdbId, null);
    });

    it('keeps the selected row marked across a re-render', async () => {
        const app = createMockApp();
        const widget = new ClustersWidget(makeContainer(), app, () => {});
        await widget.update();

        widget._table.querySelector('tbody tr')
            .dispatchEvent(new window.Event('click', { bubbles: true }));
        await waitForMicrotasks();

        // Collapsing a node rebuilds the table.
        widget._toggleNode(1);
        const marked = widget._table.querySelectorAll('tbody tr.selected');
        assert.equal(marked.length, 1);
        assert.match(marked[0].children[0].textContent, /root/);
    });

    // The point of clicking a cluster: its instances — including those owned by
    // its nested clusters — are the only ones the `_clusters` layer paints, in
    // the color of the row's swatch.  The layer keys off each instance's own
    // dbGroup, so the map has to carry the whole subtree, not just the clicked
    // cluster.
    it('isolates the selected cluster subtree in the color map', async () => {
        const app = createMockApp();
        const widget = new ClustersWidget(makeContainer(), app, () => {});
        await widget.update();

        // Row 1 is (root)_glue_logic (odb 11), collapsed over its leaf (odb 12).
        widget._table.querySelectorAll('tbody tr')[1]
            .dispatchEvent(new window.Event('click', { bubbles: true }));
        await waitForMicrotasks();

        const parts = app.sent.filter(m => m.type === 'set_group_colors')
                          .at(-1).colors.split(';').sort();
        // Collapsed, so the leaf inherits its parent's color — the same green
        // the swatch shows.
        assert.deepEqual(parts, ['11:0,255,0,100', '12:0,255,0,100']);

        // No overlay shapes for this selection: the yellow veil would cover the
        // cluster color, and the cap that truncates big clusters does not apply
        // to the tile layer.
        const select = app.sent.filter(m => m.type === 'select_group').at(-1);
        assert.equal(select.no_highlight, true);
    });

    it('restores every cluster when the selection is dropped', async () => {
        const app = createMockApp();
        const widget = new ClustersWidget(makeContainer(), app, () => {});
        await widget.update();

        const tr = widget._table.querySelectorAll('tbody tr')[1];
        tr.dispatchEvent(new window.Event('click', { bubbles: true }));
        await waitForMicrotasks();
        assert.equal(widget._isolatedOdbIds().size, 2);

        tr.dispatchEvent(new window.Event('click', { bubbles: true }));
        await waitForMicrotasks();

        assert.equal(widget._isolatedOdbIds(), null);
        const parts = app.sent.filter(m => m.type === 'set_group_colors')
                          .at(-1).colors.split(';');
        assert.equal(parts.length, 4);
        assert.equal(widget._statusLabel.textContent, '4 clusters');
    });

    // A double click delivers click(detail 1), click(detail 2) and then
    // dblclick.  Acting on the second click undid the first — the cluster ended
    // up zoomed to but deselected, and the layout flashed from the isolated map
    // back to the full one.
    it('keeps the cluster selected through a double click', async () => {
        const app = createMockApp();
        const widget = new ClustersWidget(makeContainer(), app, () => {});
        await widget.update();

        const tr = widget._table.querySelectorAll('tbody tr')[1];
        const click = (detail) => tr.dispatchEvent(
            new window.MouseEvent('click', { bubbles: true, detail }));
        click(1);
        click(2);
        tr.dispatchEvent(
            new window.MouseEvent('dblclick', { bubbles: true, detail: 2 }));
        await waitForMicrotasks();

        assert.equal(widget._selectedOdbId, 11);
        assert.equal(app.sent.filter(m => m.type === 'select_group').length, 1);
        assert.ok(app.fitted, 'the double click still zoomed to the cluster');
    });

    // Switching clusters used to put two color maps in flight: the panel's own
    // selection-reset handler restored the full one, then the click sent the
    // isolated one.
    it('sends one color map per cluster switch', async () => {
        const app = createMockApp();
        const widget = new ClustersWidget(makeContainer(), app, () => {});
        await widget.update();

        const rows = widget._table.querySelectorAll('tbody tr');
        rows[1].dispatchEvent(new window.Event('click', { bubbles: true }));
        await waitForMicrotasks();
        const before
            = app.sent.filter(m => m.type === 'set_group_colors').length;

        rows[2].dispatchEvent(new window.Event('click', { bubbles: true }));
        await waitForMicrotasks();

        const colorMsgs = app.sent.filter(m => m.type === 'set_group_colors');
        assert.equal(colorMsgs.length - before, 1);
        // And the one that was sent is the new cluster's isolated map.
        assert.equal(colorMsgs.at(-1).colors, '13:255,255,0,100');
    });

    // Hiding the selected cluster already released the selection; the isolated
    // map has to go with it, or the layout would keep showing one cluster on its
    // own with no selected row to explain it.
    it('restores the full color map when the selected cluster is hidden',
       async () => {
           const app = createMockApp();
           const widget = new ClustersWidget(makeContainer(), app, () => {});
           await widget.update();

           widget._table.querySelectorAll('tbody tr')[1]
               .dispatchEvent(new window.Event('click', { bubbles: true }));
           await waitForMicrotasks();

           widget._checkModel.check(1, false);
           await waitForMicrotasks();

           assert.equal(widget._isolatedOdbIds(), null);
           // The unticked subtree drops out, and so does root: with only part of
           // its subtree ticked its checkbox goes indeterminate, which
           // CheckboxTreeModel reports as unchecked.  What matters here is that
           // the map is no longer the isolated one — macro_cluster, untouched by
           // the click, paints again.
           const parts = app.sent.filter(m => m.type === 'set_group_colors')
                             .at(-1).colors.split(';').sort();
           assert.deepEqual(parts, ['13:255,255,0,100']);
       });

    // Selecting a cluster paints nothing while the `_clusters` overlay is off,
    // which reads as a broken panel unless it says so.
    it('warns when the cluster overlay is off', async () => {
        const app = createMockApp({ visibility: { cluster_view: false } });
        const widget = new ClustersWidget(makeContainer(), app, () => {});
        await widget.update();

        widget._table.querySelector('tbody tr')
            .dispatchEvent(new window.Event('click', { bubbles: true }));
        await waitForMicrotasks();
        assert.match(widget._statusLabel.textContent, /Cluster view is off/);

        app.visibility.cluster_view = true;
        widget._table.querySelectorAll('tbody tr')[1]
            .dispatchEvent(new window.Event('click', { bubbles: true }));
        await waitForMicrotasks();
        assert.equal(widget._statusLabel.textContent, '4 clusters');
    });

    // The checkbox stays the authority on what may appear: an unticked cluster
    // is not painted just because it was clicked.
    it('says so when the selected cluster is hidden by its checkbox',
       async () => {
           const app = createMockApp();
           const widget = new ClustersWidget(makeContainer(), app, () => {});
           await widget.update();

           widget._checkModel.check(3, false);
           await waitForMicrotasks();

           // Row 2 is macro_cluster (odb 13), the one just unticked.
           widget._table.querySelectorAll('tbody tr')[2]
               .dispatchEvent(new window.Event('click', { bubbles: true }));
           await waitForMicrotasks();

           assert.match(widget._statusLabel.textContent, /hidden/);
           assert.equal(app.sent.filter(m => m.type === 'set_group_colors')
                            .at(-1).colors, '');
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
