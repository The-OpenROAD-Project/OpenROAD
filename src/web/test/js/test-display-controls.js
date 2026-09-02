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
