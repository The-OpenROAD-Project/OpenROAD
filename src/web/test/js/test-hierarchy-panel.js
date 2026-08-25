// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { waitForMicrotasks } from './setup-dom.js';
import { beforeEach, describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { HierarchyPanel } from '../../src/hierarchy-panel.js';
import { deleteCookie, setCookie } from '../../src/theme.js';

const SOURCE_COOKIE = 'or_hierarchy_source';

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

// Stands in for GoldenLayout's container, whose 'destroy' the panel listens
// for.  `close()` fires it the way closing the tab would.
function makeContainer() {
    const handlers = {};
    return {
        element: document.createElement('div'),
        on: (event, fn) => { handlers[event] = fn; },
        close: () => handlers.destroy && handlers.destroy(),
    };
}

// The view that is showing, by its wrapper's display style.
function visibleViews(panel) {
    return [...panel._widgets].filter(([, w]) => w.element.style.display !== 'none')
        .map(([name]) => name);
}

describe('HierarchyPanel', () => {
    // The panel now remembers its source in a cookie, and the whole file
    // shares one jsdom: without this, the first case to switch to clusters
    // decides where every later panel starts.
    beforeEach(() => deleteCookie(SOURCE_COOKIE));

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
        assert.equal(toolbar.firstChild, panel._picker);
    });

    // The dropdown names where each tree comes from, and the label says what
    // the dropdown picks (review of #11122): "Instances" and "Clusters" said
    // neither.
    it('names the sources and labels the dropdown', () => {
        const panel = new HierarchyPanel(makeContainer(), createMockApp(),
                                         () => {});
        const labels = [...panel._select.options].map(o => o.textContent);
        assert.deepEqual(labels, ['Verilog Modules', 'Instance Groups']);
        assert.equal(panel._picker.firstChild.textContent, 'Source:');
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
        assert.equal(panel.activeWidget().toolbar.firstChild, panel._picker);
        assert.equal(instancesToolbar.contains(panel._picker), false);
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

    // The DBU toggle re-renders the rows.  Doing it on a hidden view is what
    // breaks the table: makeResizableHeaders measures the headers, gets 0 from
    // a table that is not laid out, and locks the columns at that width.
    it('re-renders the hidden view only once it is shown', () => {
        const panel = new HierarchyPanel(makeContainer(), createMockApp(),
                                         () => {});
        const renders = new Map();
        for (const [name, widget] of panel._widgets) {
            renders.set(name, 0);
            const inner = widget._render.bind(widget);
            widget._render = () => {
                renders.set(name, renders.get(name) + 1);
                inner();
            };
        }

        panel.refresh();
        assert.equal(renders.get('instances'), 1, 'the visible view renders');
        assert.equal(renders.get('clusters'), 0, 'the hidden one must wait');

        panel.selectView('clusters');
        assert.equal(renders.get('clusters'), 1, 'and renders once shown');
        // Only what refresh() marked: switching back is not a re-render.
        panel.selectView('instances');
        panel.selectView('clusters');
        assert.equal(renders.get('clusters'), 1);
    });

    it('registers itself on the app for the View menu', () => {
        const app = createMockApp();
        const panel = new HierarchyPanel(makeContainer(), app, () => {});
        assert.equal(app.hierarchyPanel, panel);
    });

    // The source decides which overlay the Display Controls checkbox turns on,
    // so a reload that forgot it painted the wrong tree: someone on Instance
    // Groups came back to module coloring.
    describe('remembered source', () => {
        it('starts on the source the last session left', () => {
            setCookie(SOURCE_COOKIE, 'clusters');
            const panel = new HierarchyPanel(makeContainer(), createMockApp(),
                                             () => {});
            assert.equal(panel.activeView(), 'clusters');
            assert.deepEqual(visibleViews(panel), ['clusters']);
        });

        it('records each switch', () => {
            const panel = new HierarchyPanel(makeContainer(), createMockApp(),
                                             () => {});
            panel.selectView('clusters');
            assert.match(document.cookie, /or_hierarchy_source=clusters/);
            panel.selectView('instances');
            assert.match(document.cookie, /or_hierarchy_source=instances/);
        });

        // selectView ignores names it does not know, so an unchecked value
        // here would leave the panel with no active view at all.
        it('falls back when the cookie names a view that is gone', () => {
            setCookie(SOURCE_COOKIE, 'gone');
            const panel = new HierarchyPanel(makeContainer(), createMockApp(),
                                             () => {});
            assert.equal(panel.activeView(), 'instances');
            assert.ok(panel.activeWidget(), 'a view is showing');
        });
    });

    // Display Controls has one checkbox for both color overlays; the source
    // showing here decides which of the two it turns on.  Two sources coloring
    // the same instance at once is the thing this prevents: the module colors
    // paint under the routing and the cluster colors over it.
    describe('overlay flags', () => {
        function withOverlay(hierarchyView) {
            const app = createMockApp();
            const refreshed = [];
            app.visibility = { ui_hierarchy_view: hierarchyView };
            app.modulesLayer = {
                refreshTiles: () => refreshed.push('_modules') };
            app.clustersLayer = {
                refreshTiles: () => refreshed.push('_clusters') };
            const panel = new HierarchyPanel(makeContainer(), app, () => {});
            return { app, panel, refreshed };
        }

        it('follows the active source while the overlay is on', () => {
            const { app, panel, refreshed } = withOverlay(true);
            assert.equal(app.visibility.module_view, true);
            assert.equal(app.visibility.cluster_view, false);

            panel.selectView('clusters');
            assert.equal(app.visibility.cluster_view, true);
            assert.equal(app.visibility.module_view, false);

            panel.selectView('instances');
            assert.equal(app.visibility.module_view, true);
            assert.equal(app.visibility.cluster_view, false);

            // Both overlays repaint on every switch: one starts drawing and
            // the other has to stop.
            assert.deepEqual(refreshed.slice(-2), ['_modules', '_clusters']);
        });

        it('leaves both off whatever the source, while it is off', () => {
            const { app, panel } = withOverlay(false);
            for (const view of ['clusters', 'instances', 'clusters']) {
                panel.selectView(view);
                assert.equal(app.visibility.module_view, false);
                assert.equal(app.visibility.cluster_view, false);
            }
        });

        // Repainting is not free -- refreshTiles cancels and re-issues every
        // visible tile of both overlay layers.  Restoring the source the panel
        // already starts on, or picking the current entry in the dropdown,
        // moves no flag and must cost nothing.
        it('does not repaint when no flag moved', () => {
            const { panel, refreshed } = withOverlay(true);
            refreshed.length = 0;

            panel.selectView('instances');   // already there

            assert.deepEqual(refreshed, []);
        });

        // Both views report whether their own overlay is on.  With one
        // checkbox for the two, a user on Verilog Modules is the likeliest to
        // tick a module and see nothing happen, so that view has to say why.
        it('each view reports its own gate', async () => {
            const { app, panel } = withOverlay(true);
            for (const [view, gate, count] of [['instances', 'module_view',
                                                '1 modules'],
                                               ['clusters', 'cluster_view',
                                                '1 groups']]) {
                panel.selectView(view);
                const widget = panel.activeWidget();
                await widget.update();
                assert.equal(widget._statusLabel.textContent, count);

                app.visibility[gate] = false;
                widget.refreshStatus();
                assert.match(widget._statusLabel.textContent,
                             /Hierarchy view is off/);

                app.visibility[gate] = true;
                widget.refreshStatus();
                assert.equal(widget._statusLabel.textContent, count);
            }
        });
    });

    // With the tab gone there is no source dropdown and no per-row checkbox
    // left to control what the overlays paint, so the coloring goes with it.
    // The Display Controls checkbox is the user's and stays put; an empty
    // color map is what stops the layers drawing.
    describe('closing the tab', () => {
        function openThenClose() {
            const app = createMockApp();
            const refreshed = [];
            app.visibility = { ui_hierarchy_view: true };
            app.modulesLayer = {
                refreshTiles: () => refreshed.push('_modules') };
            app.clustersLayer = {
                refreshTiles: () => refreshed.push('_clusters') };
            const container = makeContainer();
            const panel = new HierarchyPanel(container, app, () => {});
            return { app, panel, container, refreshed };
        }

        it('clears both views\' color maps, not just the visible one',
           async () => {
            const { app, container } = openThenClose();

            container.close();
            await waitForMicrotasks();

            for (const type of ['set_module_colors', 'set_group_colors']) {
                const last = app.sent.filter(m => m.type === type).at(-1);
                assert.ok(last, type + ' sent');
                assert.equal(last.colors, '', type + ' cleared');
            }
        });

        it('leaves the overlay flags alone', () => {
            const { app, container } = openThenClose();
            container.close();

            assert.equal(app.visibility.ui_hierarchy_view, true,
                         'the Display Controls checkbox is the user\'s');
            assert.equal(app.visibility.module_view, true,
                         'the flag stays a derivation of it and the source');
        });

        it('releases the app registration', () => {
            const { app, panel, container } = openThenClose();
            assert.equal(app.hierarchyPanel, panel);

            container.close();

            assert.equal(app.hierarchyPanel, null);
        });

        // A reopened tab registers before the old one is destroyed, so the
        // outgoing panel must not clear its successor out of the app.
        it('does not unregister a panel that replaced it', () => {
            const { app, container } = openThenClose();
            const replacement = new HierarchyPanel(makeContainer(), app,
                                                   () => {});

            container.close();

            assert.equal(app.hierarchyPanel, replacement);
        });
    });
});
