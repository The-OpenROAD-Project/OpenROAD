// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it, beforeEach } from 'node:test';
import assert from 'node:assert/strict';
import { dom } from './setup-dom.js';

const { layerRangeSet, nonSolidPatterns, populateDisplayControls }
    = await import('../../src/display-controls.js');
const { beginSelection } = await import('../../src/ui-utils.js');

// 10 layers: Metal1, Via1, Metal2, Via2, ... Metal5, Via5
const COUNT = 10;

describe('layerRangeSet', () => {
    it('show only selected (center only)', () => {
        const s = layerRangeSet(3, 0, 0, COUNT);
        assert.deepEqual(s, new Set([3]));
    });

    it('range ±1 in the middle', () => {
        const s = layerRangeSet(4, 1, 1, COUNT);
        assert.deepEqual(s, new Set([3, 4, 5]));
    });

    it('range ±2 in the middle', () => {
        const s = layerRangeSet(5, 2, 2, COUNT);
        assert.deepEqual(s, new Set([3, 4, 5, 6, 7]));
    });

    it('range down only (lower=1, upper=0)', () => {
        const s = layerRangeSet(4, 1, 0, COUNT);
        assert.deepEqual(s, new Set([3, 4]));
    });

    it('range up only (lower=0, upper=1)', () => {
        const s = layerRangeSet(4, 0, 1, COUNT);
        assert.deepEqual(s, new Set([4, 5]));
    });

    it('clamps at lower bound', () => {
        const s = layerRangeSet(0, 2, 2, COUNT);
        assert.deepEqual(s, new Set([0, 1, 2]));
    });

    it('clamps at upper bound', () => {
        const s = layerRangeSet(9, 2, 2, COUNT);
        assert.deepEqual(s, new Set([7, 8, 9]));
    });

    it('single layer total', () => {
        const s = layerRangeSet(0, 1, 1, 1);
        assert.deepEqual(s, new Set([0]));
    });

    it('range down at first layer returns only first', () => {
        const s = layerRangeSet(0, 1, 0, COUNT);
        assert.deepEqual(s, new Set([0]));
    });

    it('range up at last layer returns only last', () => {
        const s = layerRangeSet(9, 0, 1, COUNT);
        assert.deepEqual(s, new Set([9]));
    });
});

describe('nonSolidPatterns', () => {
    it('drops solid (value 1) entries and keeps the rest', () => {
        const out = nonSolidPatterns(
            { metal1: 1, metal2: 2, metal3: 0, metal4: 4 });
        assert.deepEqual(out, { metal2: 2, metal3: 0, metal4: 4 });
    });

    it('returns an empty object when everything is solid', () => {
        assert.deepEqual(nonSolidPatterns({ a: 1, b: 1 }), {});
    });

    it('tolerates null/undefined input', () => {
        assert.deepEqual(nonSolidPatterns(null), {});
        assert.deepEqual(nonSolidPatterns(undefined), {});
    });
});

// Clicking a layer's name selects it and shows its properties in the
// Inspector, mirroring the Qt GUI's DisplayControls row selection.  The
// invariant that goes with it: a click anywhere in the row that is not on a
// checkbox must never change visibility — only the checkbox column toggles.
describe('layer row selection', () => {
    let app, requests, inspected, focused, techData;

    // Minimal stand-ins: the tree only ever calls addTo/removeLayer and
    // refreshTiles on these.
    class FakeTileLayer {
        constructor(wm, name, opts) {
            this.name = name;
            this.options = opts || {};
        }
        addTo() { return this; }
        refreshTiles() {}
    }
    class FakeHeatMapLayer {
        constructor() {}
    }

    beforeEach(() => {
        requests = [];
        inspected = [];
        focused = [];
        window.sessionStorage.clear();
        app = {
            displayControlsEl: document.createElement('div'),
            allLayers: [],
            visibleLayers: new Set(),
            visibleLayerNames: new Set(),
            selectableLayers: new Set(),
            layerPatterns: {},
            visibleChiplets: null,
            hasLiberty: false,
            showDbu: false,
            map: { hasLayer: () => false, removeLayer() {} },
            websocketManager: {
                request(msg) {
                    requests.push(msg);
                    return Promise.resolve({ type: 'Layer', name: msg.layer });
                },
            },
            updateInspector: (data) => inspected.push(data),
            focusComponent: (name) => focused.push(name),
            refreshOverlay: () => {},
        };
        techData = {
            layers: ['metal1', 'metal2', 'nimplant'],
            sites: [],
            chiplets: [{ path: 'top', name: 'top', parent: null, depth: 0 }],
            layer_hierarchy: {
                name: 'top',
                type: 'block',
                path: 'top',
                layers: [
                    { name: 'metal1', color: [1, 2, 3] },
                    { name: 'metal2', color: [4, 5, 6] },
                ],
                instances: [{
                    name: 'Implant',
                    type: 'category',
                    layers: [{ name: 'nimplant', color: [7, 8, 9] }],
                    instances: [],
                }],
            },
        };
    });

    function render() {
        populateDisplayControls(app, {}, {}, FakeTileLayer, techData,
                                () => {}, FakeHeatMapLayer);
        return app.displayControlsEl;
    }

    // Layer rows are the leaves carrying a color swatch, which distinguishes
    // them from the VisTree rows rendered into the same panel.
    function layerRow(container, name) {
        const rows = container.querySelectorAll('.vis-leaf-selectable');
        return Array.from(rows).find(
            r => r.querySelector('.vis-name').textContent === name);
    }

    const click = (el) => el.dispatchEvent(
        new dom.window.MouseEvent('click', { bubbles: true, cancelable: true }));

    // Let the request promise's .then chain run.
    const flush = () => new Promise(r => setTimeout(r, 0));

    it('sends select_layer with the layer name and owning chiplet', async () => {
        const container = render();
        const row = layerRow(container, 'metal2');
        assert.ok(row, 'metal2 row should render');
        click(row.querySelector('.vis-name'));
        await flush();

        assert.equal(requests.length, 1);
        assert.equal(requests[0].type, 'select_layer');
        assert.equal(requests[0].layer, 'metal2');
        assert.equal(requests[0].chiplet, 'top');
    });

    // Category folders (Implant/Other/Backside) have no chiplet path of their
    // own; their layers belong to the enclosing chiplet's tech.
    it('layers under a category inherit the parent chiplet path', async () => {
        const container = render();
        click(layerRow(container, 'nimplant').querySelector('.vis-name'));
        await flush();

        assert.equal(requests[0].layer, 'nimplant');
        assert.equal(requests[0].chiplet, 'top');
    });

    it('clicking the name does not toggle visibility', async () => {
        const container = render();
        const row = layerRow(container, 'metal1');
        const cb = row.querySelector('input.vis-cb');
        assert.equal(cb.checked, true);

        click(row.querySelector('.vis-name'));
        await flush();

        assert.equal(cb.checked, true, 'name click must not toggle the box');
        assert.ok(app.visibleLayerNames.has('metal1'));
    });

    // The row used to be a <label> wrapping the visibility checkbox, so a
    // click that missed the 13px box — the swatch, the indent spacer, the
    // row's padding — flipped the layer.
    it('is not a label, so nothing in the row implicitly toggles', () => {
        const container = render();
        assert.equal(layerRow(container, 'metal1').tagName, 'DIV');
    });

    it('clicking the row or the indent spacer does not toggle visibility',
       async () => {
           const container = render();
           const row = layerRow(container, 'metal1');
           const cb = row.querySelector('input.vis-cb');

           click(row);
           click(row.querySelector('.vis-arrow'));
           await flush();

           assert.equal(cb.checked, true);
           assert.ok(app.visibleLayerNames.has('metal1'));
       });

    it('clicking the color swatch selects rather than toggling', async () => {
        const container = render();
        const row = layerRow(container, 'metal1');
        const cb = row.querySelector('input.vis-cb');

        click(row.querySelector('.layer-color'));
        await flush();

        assert.equal(cb.checked, true);
        assert.equal(requests.length, 1);
        assert.equal(requests[0].layer, 'metal1');
    });

    it('clicking the visibility checkbox still toggles', () => {
        const container = render();
        const cb = layerRow(container, 'metal1').querySelector('input.vis-cb');
        cb.checked = false;
        cb.dispatchEvent(new dom.window.Event('change'));
        assert.equal(app.visibleLayerNames.has('metal1'), false);
        assert.equal(requests.length, 0, 'toggling must not select');
    });

    it('marks the clicked row and moves the mark on the next click',
       async () => {
           const container = render();
           const m1 = layerRow(container, 'metal1');
           const m2 = layerRow(container, 'metal2');

           click(m1.querySelector('.vis-name'));
           await flush();
           assert.ok(m1.classList.contains('vis-row-selected'));

           click(m2.querySelector('.vis-name'));
           await flush();
           assert.equal(m1.classList.contains('vis-row-selected'), false);
           assert.ok(m2.classList.contains('vis-row-selected'));
       });

    it('feeds the response to the inspector and focuses it', async () => {
        const container = render();
        click(layerRow(container, 'metal1').querySelector('.vis-name'));
        await flush();

        assert.equal(inspected.length, 1);
        assert.equal(inspected[0].name, 'metal1');
        assert.deepEqual(focused, ['Inspector']);
    });

    // Saved reports have no backend to answer select_layer, so their rows are
    // not selectable: the name is inert and the checkbox does the toggling,
    // exactly as on a live row.
    it('static mode leaves rows unselectable', () => {
        app.websocketManager.isStaticMode = true;
        const container = render();
        assert.equal(
            container.querySelectorAll('.vis-leaf-selectable').length, 0);
    });

    it('static mode still toggles visibility from the checkbox', () => {
        app.websocketManager.isStaticMode = true;
        const container = render();
        const rows = container.querySelectorAll('.vis-leaf');
        const row = Array.from(rows).find(
            r => r.querySelector('.vis-name')?.textContent === 'metal1');
        assert.ok(row, 'metal1 row should render in static mode');

        const cb = row.querySelector('input.vis-cb');
        cb.checked = false;
        cb.dispatchEvent(new dom.window.Event('change'));
        assert.equal(app.visibleLayerNames.has('metal1'), false);

        // The name is inert without a backend: no request, no toggle.
        click(row.querySelector('.vis-name'));
        assert.equal(requests.length, 0);
        assert.equal(cb.checked, false);
    });

    // The layer row is one of several panels that can own the selection.  When
    // another one takes over — a canvas click, Inspector navigation, the
    // fanout chart — the row would otherwise keep claiming a selection the
    // server no longer holds.
    it('drops the highlight when another panel takes the selection',
       async () => {
           const container = render();
           const row = layerRow(container, 'metal1');

           click(row.querySelector('.vis-name'));
           await flush();
           assert.ok(row.classList.contains('vis-row-selected'));

           beginSelection(app);
           assert.equal(row.classList.contains('vis-row-selected'), false);
       });

    // The server answers on a thread pool, so a layer request issued before a
    // canvas click can still land after it.  Matching only against the row
    // would accept it; the shared token rejects it.
    it('a response superseded by another panel is dropped', async () => {
        const pending = [];
        app.websocketManager.request = (msg) => {
            requests.push(msg);
            return new Promise(resolve => pending.push(
                () => resolve({ type: 'Layer', name: msg.layer })));
        };

        const container = render();
        click(layerRow(container, 'metal1').querySelector('.vis-name'));

        // Another panel selects something while metal1 is still in flight.
        beginSelection(app);
        pending[0]();
        await flush();

        assert.deepEqual(inspected, []);
        assert.deepEqual(focused, []);
    });

    // Clicking through several layers leaves one request in flight per click.
    // They can land in any order, so the response has to be matched against
    // the row that is still selected before it touches the Inspector.
    it('a stale response does not overwrite a newer selection', async () => {
        const pending = [];
        app.websocketManager.request = (msg) => {
            requests.push(msg);
            return new Promise(resolve => pending.push(
                () => resolve({ type: 'Layer', name: msg.layer })));
        };

        const container = render();
        click(layerRow(container, 'metal1').querySelector('.vis-name'));
        click(layerRow(container, 'metal2').querySelector('.vis-name'));
        assert.equal(pending.length, 2);

        // metal2 (the live selection) answers first, metal1 straggles in.
        pending[1]();
        pending[0]();
        await flush();

        assert.equal(inspected.length, 1);
        assert.equal(inspected[0].name, 'metal2');
    });

    it('a failed request drops the selection highlight', async () => {
        app.websocketManager.request = () => Promise.reject(new Error('boom'));
        const container = render();
        const row = layerRow(container, 'metal1');

        const realError = console.error;
        console.error = () => {};
        try {
            click(row.querySelector('.vis-name'));
            await flush();
        } finally {
            console.error = realError;
        }

        assert.equal(row.classList.contains('vis-row-selected'), false);
    });

    // A failure for a row the user already clicked past must not clear the
    // highlight the newer click put somewhere else.
    it('a failure for a superseded row leaves the newer highlight',
       async () => {
           const pending = [];
           app.websocketManager.request = (msg) => {
               requests.push(msg);
               return new Promise((resolve, reject) => pending.push({
                   ok: () => resolve({ type: 'Layer', name: msg.layer }),
                   fail: () => reject(new Error('boom')),
               }));
           };

           const container = render();
           const m1 = layerRow(container, 'metal1');
           const m2 = layerRow(container, 'metal2');
           click(m1.querySelector('.vis-name'));
           click(m2.querySelector('.vis-name'));

           const realError = console.error;
           console.error = () => {};
           try {
               pending[1].ok();
               pending[0].fail();
               await flush();
           } finally {
               console.error = realError;
           }

           assert.ok(m2.classList.contains('vis-row-selected'));
           assert.equal(m1.classList.contains('vis-row-selected'), false);
       });
});

describe('routing layer opacity', () => {
    // Layer transparency is baked into the tile pixels: the server paints
    // layer shapes at the palette alpha of 180/255, the same alpha the Qt GUI
    // brush carries (displayControls.cpp).  A pane opacity below 1 would apply
    // that transparency a second time — 0.7 * 180/255 = 0.49 instead of 0.71 —
    // and the browser would render the layout ~30% darker than the Qt GUI.
    // Pinned on both paths because the legacy panes and the merged draw list
    // set the value independently.
    let created, mergedItems, app, techData;

    class RecordingTileLayer {
        constructor(wm, name, opts) {
            created.push({ name, opts: opts || {} });
            this.name = name;
            this.options = opts || {};
        }
        addTo() { return this; }
        refreshTiles() {}
    }
    class RecordingMergedLayer {
        constructor(wm, items, opts) {
            mergedItems.push(...items);
            this.options = opts || {};
        }
        addTo() { return this; }
        refreshTiles() {}
        setItems() {}
    }

    beforeEach(() => {
        created = [];
        mergedItems = [];
        window.sessionStorage.clear();
        app = {
            displayControlsEl: document.createElement('div'),
            allLayers: [],
            visibleLayers: new Set(),
            visibleLayerNames: new Set(),
            selectableLayers: new Set(),
            layerPatterns: {},
            visibleChiplets: null,
            hasLiberty: false,
            showDbu: false,
            map: {
                hasLayer: () => false,
                removeLayer() {},
                getContainer: () => document.createElement('div'),
                on() {},
            },
            websocketManager: { request: () => Promise.resolve({}) },
            updateInspector: () => {},
            focusComponent: () => {},
            refreshOverlay: () => {},
        };
        techData = {
            layers: ['metal1', 'metal2'],
            sites: [],
            chiplets: [{ path: 'top', name: 'top', parent: null, depth: 0 }],
            layer_hierarchy: {
                name: 'top',
                type: 'block',
                path: 'top',
                layers: [
                    { name: 'metal1', color: [1, 2, 3] },
                    { name: 'metal2', color: [4, 5, 6] },
                ],
                instances: [],
            },
        };
    });

    const render = () => populateDisplayControls(
        app, {}, {}, RecordingTileLayer, techData, () => {}, class {});

    it('creates legacy per-layer panes fully opaque', () => {
        render();
        const routing = created.filter(l => l.name.startsWith('metal'));
        assert.ok(routing.length >= 2, 'metal layers should get panes');
        for (const layer of routing) {
            assert.equal(layer.opts.opacity, 1, layer.name);
        }
    });

    it('gives merged draw items an opacity of 1', () => {
        app.mergeTiles = true;
        app.MergedTileLayer = RecordingMergedLayer;
        render();
        const routing = mergedItems.filter(i => i.layer.startsWith('metal'));
        assert.ok(routing.length >= 2, 'metal layers should reach the merge');
        for (const item of routing) {
            assert.equal(item.opacity, 1, item.layer);
        }
    });
});

// ─── Per-renderer display controls ──────────────────────────────────────────
// The rows Qt's DisplayControls builds from Renderer::getDisplayControls().
// Server state, so the panel fetches them and posts changes back.

describe('renderer display controls', () => {
    const wait = () => new Promise((r) => setTimeout(r, 0));

    class FakeTileLayer {
        constructor() {}
        addTo() { return this; }
        redraw() {}
        setZIndex() { return this; }
        setOpacity() { return this; }
        on() { return this; }
        remove() {}
    }
    class FakeHeatMapLayer {
        constructor() {}
    }

    const CONTROLS = [
        { group: 'Detailed Router', name: 'Maze search',
          path: 'Detailed Router/Maze search', visible: true,
          exclusivity: [] },
        { group: 'Detailed Router', name: 'Graph edges',
          path: 'Detailed Router/Graph edges', visible: false,
          exclusivity: [] },
        { group: 'PDN', name: 'Vias', path: 'PDN/Vias', visible: true,
          exclusivity: [] },
        { group: '', name: 'Loose', path: 'Loose', visible: false,
          exclusivity: [] },
    ];

    function makeApp(controls) {
        const requests = [];
        const app = {
            displayControlsEl: document.createElement('div'),
            allLayers: [],
            visibleLayers: new Set(),
            visibleLayerNames: new Set(),
            selectableLayers: new Set(),
            layerPatterns: {},
            visibleChiplets: null,
            hasLiberty: false,
            showDbu: false,
            map: { hasLayer: () => false, removeLayer() {} },
            websocketManager: {
                request(msg) {
                    requests.push(msg);
                    if (msg.type === 'renderer_controls') {
                        return Promise.resolve({ controls });
                    }
                    return Promise.resolve({});
                },
            },
            updateInspector() {},
            focusComponent() {},
            refreshOverlay() {},
        };
        return { app, requests };
    }

    const techData = {
        layers: ['metal1'],
        sites: [],
        chiplets: [{ path: 'top', name: 'top', parent: null, depth: 0 }],
        layer_hierarchy: {
            name: 'top', type: 'block', path: 'top',
            layers: [{ name: 'metal1', color: [1, 2, 3] }],
            instances: [],
        },
    };

    function render(app, visibility) {
        populateDisplayControls(app, visibility, {}, FakeTileLayer, techData,
                                () => {}, FakeHeatMapLayer);
        return app.displayControlsEl.querySelector('.renderer-controls');
    }

    // Groups only carry a name; a leaf carries the control's own name.
    function leafNames(container) {
        return Array.from(container.querySelectorAll('.vis-leaf'))
            .map(r => r.querySelector('.vis-name').textContent);
    }

    it('is not fetched while the Renderers overlay is off', async () => {
        const { app, requests } = makeApp(CONTROLS);
        const el = render(app, { debug_renderers: false });
        await wait();
        assert.equal(
            requests.filter(r => r.type === 'renderer_controls').length, 0,
            'no request for a list that is empty by construction');
        assert.equal(el.children.length, 0);
    });

    it('renders a group per renderer group, merging shared names',
       async () => {
        const { app } = makeApp(CONTROLS);
        const el = render(app, { debug_renderers: true });
        await wait();

        const groupNames = Array.from(el.querySelectorAll('.vis-group-header'))
            .map(h => h.querySelector('.vis-name').textContent);
        assert.deepEqual(groupNames, ['Detailed Router', 'PDN']);
        // Both Detailed Router controls land under the one group.
        const routerGroup = el.querySelectorAll('.vis-group')[0];
        assert.deepEqual(leafNames(routerGroup),
                         ['Maze search', 'Graph edges']);
        // A control with no group name sits at the top level.
        assert.ok(leafNames(el).includes('Loose'));
    });

    // The rows sit in the same panel as the layer rows and the VisTree
    // leaves, which both prepend a hidden 14px triangle to the name column.
    // Without it these names hang left of every other leaf name.
    it('gives its rows the same anatomy as the panel\'s other leaves',
       async () => {
        const { app } = makeApp(CONTROLS);
        const el = render(app, { debug_renderers: true });
        await wait();

        const leaves = [...el.querySelectorAll('.vis-leaf')];
        assert.ok(leaves.length > 0);
        for (const row of leaves) {
            const first = row.firstElementChild;
            assert.equal(first.className, 'vis-arrow',
                         'leaf must start with the name-column spacer');
            assert.equal(first.style.visibility, 'hidden');
        }
    });

    it('reflects each control\'s server-side visibility', async () => {
        const { app } = makeApp(CONTROLS);
        const el = render(app, { debug_renderers: true });
        await wait();

        const byName = {};
        for (const row of el.querySelectorAll('.vis-leaf')) {
            byName[row.querySelector('.vis-name').textContent]
                = row.querySelector('.vis-cb').checked;
        }
        assert.deepEqual(byName, {
            'Maze search': true, 'Graph edges': false,
            'Vias': true, 'Loose': false,
        });
    });

    it('posts set_renderer_control with the path the server gave', async () => {
        const { app, requests } = makeApp(CONTROLS);
        const el = render(app, { debug_renderers: true });
        await wait();

        const row = Array.from(el.querySelectorAll('.vis-leaf')).find(
            r => r.querySelector('.vis-name').textContent === 'Graph edges');
        const cb = row.querySelector('.vis-cb');
        cb.checked = true;
        cb.dispatchEvent(new dom.window.Event('change', { bubbles: true }));
        await wait();

        const set = requests.find(r => r.type === 'set_renderer_control');
        assert.deepEqual(set, { type: 'set_renderer_control',
                                path: 'Detailed Router/Graph edges',
                                value: true });
    });

    // The server broadcasts renderer_controls_changed to every session,
    // including the sender, and main.js re-reads and redraws from that — so
    // the click itself must not also re-read, or every toggle costs three
    // round trips and two full-layer redraws.
    it('does not re-read locally after a change', async () => {
        const { app, requests } = makeApp(CONTROLS);
        const el = render(app, { debug_renderers: true });
        await wait();
        const before = requests.filter(
            r => r.type === 'renderer_controls').length;

        const cb = el.querySelector('.vis-leaf .vis-cb');
        cb.dispatchEvent(new dom.window.Event('change', { bubbles: true }));
        await wait();
        await wait();

        assert.equal(
            requests.filter(r => r.type === 'renderer_controls').length,
            before, 'the push is the single trigger for the re-read');
        assert.equal(
            requests.filter(r => r.type === 'set_renderer_control').length, 1);
    });

    // Qt's parent row drives its children; ticking it must move the ones that
    // are off, and only those.
    it('the group checkbox applies to every child that differs', async () => {
        const { app, requests } = makeApp(CONTROLS);
        const el = render(app, { debug_renderers: true });
        await wait();

        const routerHeader = el.querySelector('.vis-group-header');
        const groupCb = routerHeader.querySelector('.vis-cb');
        assert.equal(groupCb.checked, false, 'mixed children');
        assert.equal(groupCb.indeterminate, true);

        groupCb.checked = true;
        groupCb.dispatchEvent(new dom.window.Event('change',
                                                   { bubbles: true }));
        await wait();

        const sets = requests.filter(r => r.type === 'set_renderer_control');
        assert.deepEqual(sets, [{ type: 'set_renderer_control',
                                  path: 'Detailed Router/Graph edges',
                                  value: true }]);
        // And no local re-read: the broadcast drives it.
        assert.equal(
            requests.filter(r => r.type === 'renderer_controls').length, 1,
            'only the initial fetch');
    });

    // refreshRendererControls owns the "overlay is on" rule, so switching it
    // off clears the rows instead of leaving a stale list behind.
    it('clears itself when the Renderers overlay goes off', async () => {
        const { app } = makeApp(CONTROLS);
        const visibility = { debug_renderers: true };
        const el = render(app, visibility);
        await wait();
        assert.ok(el.querySelectorAll('.vis-leaf').length > 0);

        visibility.debug_renderers = false;
        await app.refreshRendererControls();
        assert.equal(el.children.length, 0);
    });

    it('renders nothing when no renderer is registered', async () => {
        const { app } = makeApp([]);
        const el = render(app, { debug_renderers: true });
        await wait();
        assert.equal(el.children.length, 0);
    });

    // The server leaves the HeatMapRenderer controls out of the list, so the
    // panel has exactly one "Heat Maps" group — the settings one below.
    it('shows no Heat Maps group of its own', async () => {
        const { app } = makeApp([
            { group: 'Heat Maps', name: 'Pin Density',
              path: 'Heat Maps/Pin Density', visible: false, exclusivity: [''] },
        ]);
        const el = render(app, { debug_renderers: true });
        await wait();
        // Nothing is filtered client-side; this documents that a payload
        // carrying one would still render, so the filtering has to stay on the
        // server where every client sees the same list.
        assert.equal(el.querySelectorAll('.vis-group').length, 1);

        // What the server actually sends: no heat-map rows at all.
        const { app: app2 } = makeApp(
            CONTROLS.filter(c => c.group !== 'Heat Maps'));
        const el2 = render(app2, { debug_renderers: true });
        await wait();
        const groups = Array.from(el2.querySelectorAll('.vis-group-header'))
            .map(h => h.querySelector('.vis-name').textContent);
        assert.equal(groups.includes('Heat Maps'), false);
    });

    // The panel ends with the Background row, so every group — the heat map
    // settings included — sits above it.
    it('puts the heat map settings above the Background row', async () => {
        const { app } = makeApp([]);
        populateDisplayControls(app, { debug_renderers: true }, {},
                               FakeTileLayer, techData, () => {},
                               FakeHeatMapLayer);
        await wait();

        const children = Array.from(app.displayControlsEl.children);
        const heatIdx = children.findIndex(
            c => c.classList.contains('heatmap-controls'));
        const bgIdx = children.findIndex(
            c => c.classList.contains('bg-color-row'));
        assert.ok(heatIdx >= 0, 'heat map group present');
        assert.ok(bgIdx >= 0, 'background row present');
        assert.ok(heatIdx < bgIdx,
                  'heat maps must come before the Background footer');
        assert.equal(bgIdx, children.length - 1,
                     'Background closes the panel');
    });
});
