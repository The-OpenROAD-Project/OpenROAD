// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { waitForMicrotasks } from './setup-dom.js';
import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { HierarchyPanel } from '../../src/hierarchy-panel.js';

// One module and one cluster, enough for each view to render a row.
const MODULE_NODES = [
    {
        id: 0, parent_id: -1, inst_name: 'top', module_name: 'top', odb_id: 1,
        node_kind: 0, insts: 10, macros: 0, modules: 0, area: 1.0,
        local_insts: 10, local_macros: 0, local_modules: 0, color: [255, 0, 0],
    },
];
const CLUSTER_NODES = [
    {
        id: 0, parent_id: -1, name: 'root', type: 'VISUAL_DEBUG', odb_id: 10,
        insts: 10, macros: 0, groups: 0, area: 1.0,
        local_insts: 10, local_macros: 0, local_groups: 0,
        bbox: [0, 0, 100, 100], color: [0, 255, 0],
    },
];

function createMockApp() {
    const sent = [];
    return {
        sent,
        websocketManager: {
            readyPromise: Promise.resolve(),
            request(msg) {
                sent.push(msg);
                if (msg.type === 'module_hierarchy') {
                    return Promise.resolve({ nodes: MODULE_NODES });
                }
                if (msg.type === 'group_hierarchy') {
                    return Promise.resolve({ nodes: CLUSTER_NODES });
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
        map: { fitBounds() {} },
        updateInspector() {},
        refreshOverlay() {},
        visibility: { cluster_view: true },
    };
}

const makeContainer = () => ({ element: document.createElement('div') });

// The view that is showing, by its wrapper's display style.
function visibleViews(panel) {
    return [...panel._widgets].filter(([, w]) => w.element.style.display !== 'none')
        .map(([name]) => name);
}

describe('HierarchyPanel', () => {
    it('starts on the instances view with only that one visible', () => {
        const panel = new HierarchyPanel(makeContainer(), createMockApp(),
                                         () => {});
        assert.equal(panel.activeView(), 'instances');
        assert.deepEqual(visibleViews(panel), ['instances']);
    });

    // The element whose `display` is toggled has to be the one that takes up
    // the panel's space, i.e. a direct child of the container.  With a wrapper
    // in between, hiding the inner element left the wrapper in the flow: two
    // wrappers of `height: 100%` stacked, the second one below the visible area
    // and clipped away by .lm_content's `overflow: hidden` — the Clusters view
    // rendered into thin air.
    //
    // jsdom does no layout (every height is 0), so this is the structural
    // invariant the browser's layout depends on, not a check of what is on
    // screen.
    it('toggles the element that occupies the panel', () => {
        const container = makeContainer();
        const panel = new HierarchyPanel(container, createMockApp(), () => {});
        for (const [name, widget] of panel._widgets) {
            assert.equal(widget.element.parentElement, container.element,
                         name + ' view must be a direct child of the panel');
        }
        panel.selectView('clusters');
        assert.deepEqual(visibleViews(panel), ['clusters']);
    });

    // Both views are one tab now; the selector is what replaces the old
    // Clusters tab, so it has to carry every view the panel knows.
    it('offers one option per view and lives in the active toolbar', () => {
        const panel = new HierarchyPanel(makeContainer(), createMockApp(),
                                         () => {});
        const options = [...panel._select.options].map(o => o.value);
        assert.deepEqual(options, ['instances', 'clusters']);
        // In the toolbar of the visible view, ahead of its Update button.
        const toolbar = panel.activeWidget().toolbar;
        assert.equal(toolbar.firstChild, panel._select);
    });

    it('switches the visible view and moves the selector with it', () => {
        const panel = new HierarchyPanel(makeContainer(), createMockApp(),
                                         () => {});
        const instancesToolbar = panel.activeWidget().toolbar;

        panel.selectView('clusters');

        assert.equal(panel.activeView(), 'clusters');
        assert.deepEqual(visibleViews(panel), ['clusters']);
        assert.equal(panel._select.value, 'clusters');
        // A DOM node has one parent: the selector cannot stay behind in the
        // hidden view's toolbar.
        assert.equal(panel.activeWidget().toolbar.firstChild, panel._select);
        assert.equal(instancesToolbar.contains(panel._select), false);
    });

    it('switches on the dropdown\'s change event', () => {
        const panel = new HierarchyPanel(makeContainer(), createMockApp(),
                                         () => {});
        panel._select.value = 'clusters';
        panel._select.dispatchEvent(new window.Event('change'));
        assert.equal(panel.activeView(), 'clusters');
    });

    // Hiding a view must not throw its data away: switching back has to be
    // free, not another round trip to the server.
    it('keeps a hidden view loaded', async () => {
        const app = createMockApp();
        const panel = new HierarchyPanel(makeContainer(), app, () => {});

        panel.selectView('clusters');
        await panel.activeWidget().update();
        await waitForMicrotasks();
        const loads = app.sent.filter(m => m.type === 'group_hierarchy').length;
        const rowsBefore = panel.activeWidget()._table.querySelectorAll(
            'tbody tr').length;

        panel.selectView('instances');
        panel.selectView('clusters');

        assert.equal(
            app.sent.filter(m => m.type === 'group_hierarchy').length, loads,
            'switching back must not re-request the tree');
        assert.equal(
            panel.activeWidget()._table.querySelectorAll('tbody tr').length,
            rowsBefore);
    });

    // What the View menu calls; an unknown name must not blank the panel.
    it('ignores an unknown view name', () => {
        const panel = new HierarchyPanel(makeContainer(), createMockApp(),
                                         () => {});
        panel.selectView('nope');
        assert.equal(panel.activeView(), 'instances');
        assert.deepEqual(visibleViews(panel), ['instances']);
    });

    it('registers itself on the app for the View menu', () => {
        const app = createMockApp();
        const panel = new HierarchyPanel(makeContainer(), app, () => {});
        assert.equal(app.hierarchyPanel, panel);
    });
});
