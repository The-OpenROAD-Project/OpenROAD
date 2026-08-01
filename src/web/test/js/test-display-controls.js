// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it, beforeEach } from 'node:test';
import assert from 'node:assert/strict';
import { dom } from './setup-dom.js';

const { layerRangeSet, nonSolidPatterns, populateDisplayControls }
    = await import('../../src/display-controls.js');

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
// Inspector, mirroring the Qt GUI's DisplayControls row selection.  The name
// sits inside the <label> that wraps the visibility checkbox, so the important
// invariant is that a name click selects WITHOUT toggling visibility — only
// the checkbox column toggles.
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
        const rows = container.querySelectorAll('label.vis-leaf-selectable');
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

    // Saved reports have no backend to answer select_layer, so their rows keep
    // the plain <label> behaviour.
    it('static mode leaves rows as plain visibility toggles', () => {
        app.websocketManager.isStaticMode = true;
        const container = render();
        assert.equal(
            container.querySelectorAll('label.vis-leaf-selectable').length, 0);
    });
});
