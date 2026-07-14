// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it, beforeEach } from 'node:test';
import assert from 'node:assert/strict';
import { JSDOM } from 'jsdom';

// Set up minimal DOM before importing inspector.
const dom = new JSDOM('<!DOCTYPE html><html><body></body></html>');
globalThis.document = dom.window.document;
globalThis.window = dom.window;
globalThis.Event = dom.window.Event;
globalThis.L = undefined;  // Leaflet not needed for these tests

const { createInspectorPanel } = await import('../../src/inspector.js');

// Helper to build a minimal app object with mocked websocket.
function makeApp() {
    const requests = [];
    return {
        focusNets: new Set(),
        routeGuideNets: new Set(),
        inspectorEl: document.createElement('div'),
        hoverRects: [],
        highlightRect: null,
        map: null,
        designScale: null,
        designMaxDXDY: null,
        websocketManager: {
            _nextId: 1,
            request(msg) {
                requests.push(msg);
                const p = Promise.resolve({ ok: 1, count: 0 });
                p.requestId = this._nextId++;
                return p;
            },
            cancel() {},
        },
        _requests: requests,
    };
}

// Minimal inspect data for a Net object.
function netData(name) {
    return {
        type: 'Net',
        name,
        bbox: [0, 0, 1000, 1000],
        properties: [{ name: 'Name', value: name }],
    };
}

// Minimal inspect data for an Inst object.
function instData(name) {
    return {
        type: 'Inst',
        name,
        bbox: [0, 0, 500, 500],
        properties: [{ name: 'Name', value: name }],
    };
}

describe('Inspector focus nets', () => {
    let app, panel, redraws;

    beforeEach(() => {
        app = makeApp();
        redraws = 0;
        panel = createInspectorPanel(app, () => { redraws++; });
    });

    describe('updateInspector toolbar buttons', () => {
        it('shows focus button for Net type', () => {
            panel.updateInspector(netData('clk'));
            const btns = app.inspectorEl.querySelectorAll('.inspector-btn');
            // Should have back + zoom + focus buttons
            assert.ok(btns.length >= 3, `expected >=3 buttons, got ${btns.length}`);
            const focusBtn = Array.from(btns).find(b => b.title === 'Focus net');
            assert.ok(focusBtn, 'focus button should be present');
        });

        it('does not show focus button for non-Net type', () => {
            panel.updateInspector(instData('buf1'));
            const btns = app.inspectorEl.querySelectorAll('.inspector-btn');
            // Should have back + zoom buttons, no focus button
            assert.equal(btns.length, 2);
            const focusBtn = Array.from(btns).find(b => b.title === 'Focus net');
            assert.equal(focusBtn, undefined, 'focus button should not be present');
        });

        it('shows de-focus button when net is already focused', () => {
            app.focusNets.add('clk');
            panel.updateInspector(netData('clk'));
            const btns = app.inspectorEl.querySelectorAll('.inspector-btn');
            const defocusBtn = Array.from(btns).find(b => b.title === 'De-focus net');
            assert.ok(defocusBtn, 'de-focus button should be present');
        });

        it('shows clear button when any nets are focused', () => {
            app.focusNets.add('data');
            panel.updateInspector(netData('clk'));
            const btns = app.inspectorEl.querySelectorAll('.inspector-btn');
            const clearBtn = Array.from(btns).find(b => b.title === 'Clear focus nets');
            assert.ok(clearBtn, 'clear button should be present');
        });

        it('does not show clear button when no nets are focused', () => {
            panel.updateInspector(netData('clk'));
            const btns = app.inspectorEl.querySelectorAll('.inspector-btn');
            const clearBtn = Array.from(btns).find(b => b.title === 'Clear focus nets');
            assert.equal(clearBtn, undefined, 'clear button should not be present');
        });
    });

    describe('toggleFocusNet via button click', () => {
        it('adds net to focusNets on focus click', async () => {
            panel.updateInspector(netData('clk'));
            const focusBtn = Array.from(app.inspectorEl.querySelectorAll('.inspector-btn'))
                .find(b => b.title === 'Focus net');
            focusBtn.click();
            // Let promises settle
            await new Promise(r => setTimeout(r, 10));

            assert.ok(app.focusNets.has('clk'));
            assert.equal(app._requests.length, 1);
            assert.equal(app._requests[0].type, 'set_focus_nets');
            assert.equal(app._requests[0].action, 'add');
            assert.equal(app._requests[0].net_name, 'clk');
            assert.equal(redraws, 1);
        });

        it('removes net from focusNets on de-focus click', async () => {
            app.focusNets.add('clk');
            panel.updateInspector(netData('clk'));
            const defocusBtn = Array.from(app.inspectorEl.querySelectorAll('.inspector-btn'))
                .find(b => b.title === 'De-focus net');
            assert.ok(defocusBtn, 'de-focus button should be present');
            defocusBtn.click();
            await new Promise(r => setTimeout(r, 10));

            assert.ok(!app.focusNets.has('clk'));
            assert.equal(app._requests[0].action, 'remove');
            assert.equal(redraws, 1);
        });
    });

    describe('clearFocusNets via button click', () => {
        it('clears all focus nets', async () => {
            app.focusNets.add('clk');
            app.focusNets.add('data');
            panel.updateInspector(netData('clk'));
            const clearBtn = Array.from(
                app.inspectorEl.querySelectorAll('.inspector-btn')
            ).find(b => b.title === 'Clear focus nets');
            assert.ok(clearBtn);
            clearBtn.click();
            await new Promise(r => setTimeout(r, 10));

            assert.equal(app.focusNets.size, 0);
            assert.equal(app._requests[0].action, 'clear');
            assert.equal(redraws, 1);
        });
    });

    describe('placeholder', () => {
        it('shows placeholder when no data', () => {
            panel.updateInspector(null);
            const stub = app.inspectorEl.querySelector('.stub-panel');
            assert.ok(stub, 'placeholder should be shown');
        });

        it('shows placeholder when empty properties', () => {
            panel.updateInspector({ properties: [] });
            const stub = app.inspectorEl.querySelector('.stub-panel');
            assert.ok(stub);
        });
    });

    describe('selection navigation bar', () => {
        it('does not show nav bar for single selection', () => {
            panel.updateInspector({
                ...instData('buf1'),
                selection_count: 1,
                selection_index: 0,
            });
            const nav = app.inspectorEl.querySelector('.inspector-selection-nav');
            assert.equal(nav, null, 'nav bar should not be shown for single selection');
        });

        it('shows nav bar when selection_count > 1', () => {
            panel.updateInspector({
                ...instData('buf1'),
                selection_count: 3,
                selection_index: 1,
            });
            const nav = app.inspectorEl.querySelector('.inspector-selection-nav');
            assert.ok(nav, 'nav bar should be present');
            const label = nav.querySelector('.inspector-selection-label');
            assert.equal(label.textContent, '2 / 3');
        });

        it('does not show nav bar without selection metadata', () => {
            panel.updateInspector(instData('buf1'));
            const nav = app.inspectorEl.querySelector('.inspector-selection-nav');
            assert.equal(nav, null);
        });

        it('prev button sends select_prev request', () => {
            panel.updateInspector({
                ...instData('buf1'),
                selection_count: 2,
                selection_index: 1,
            });
            const nav = app.inspectorEl.querySelector('.inspector-selection-nav');
            const prevBtn = nav.querySelectorAll('.inspector-btn')[0];
            prevBtn.click();
            assert.equal(app._requests.length, 1);
            assert.equal(app._requests[0].type, 'select_prev');
        });

        it('next button sends select_next request', () => {
            panel.updateInspector({
                ...instData('buf1'),
                selection_count: 2,
                selection_index: 0,
            });
            const nav = app.inspectorEl.querySelector('.inspector-selection-nav');
            const btns = nav.querySelectorAll('.inspector-btn');
            const nextBtn = btns[btns.length - 1];
            nextBtn.click();
            assert.equal(app._requests.length, 1);
            assert.equal(app._requests[0].type, 'select_next');
        });

        it('next button refreshes schematic selection for cycled Inst', async () => {
            let schematicRefreshes = 0;
            app.map = { closePopup() {} };
            app.schematicWidget = {
                refresh() { schematicRefreshes++; },
            };
            app.selectedInstanceName = 'buf1';
            app.websocketManager.request = msg => {
                app._requests.push(msg);
                const p = Promise.resolve({
                    ...instData('buf2'),
                    selection_count: 2,
                    selection_index: 1,
                });
                p.requestId = 1;
                return p;
            };

            panel.updateInspector({
                ...instData('buf1'),
                selection_count: 2,
                selection_index: 0,
            });
            const nav = app.inspectorEl.querySelector('.inspector-selection-nav');
            const btns = nav.querySelectorAll('.inspector-btn');
            btns[btns.length - 1].click();

            await new Promise(r => setTimeout(r, 0));

            assert.equal(app._requests[0].type, 'select_next');
            assert.equal(app.selectedInstanceName, 'buf2');
            assert.equal(schematicRefreshes, 1);
            assert.equal(redraws, 1);
        });

        it('label shows correct 1-indexed position', () => {
            panel.updateInspector({
                ...instData('buf1'),
                selection_count: 5,
                selection_index: 3,
            });
            const label = app.inspectorEl.querySelector('.inspector-selection-label');
            assert.equal(label.textContent, '4 / 5');
        });
    });

    describe('property editing', () => {
        function editableData(props) {
            return {
                type: 'Inst',
                name: 'buf1',
                bbox: [0, 0, 100, 100],
                properties: props,
            };
        }

        function blurWith(el, text) {
            el.textContent = text;
            el.dispatchEvent(new dom.window.Event('blur'));
        }

        it('string editor commits via set_property on blur', async () => {
            panel.updateInspector(editableData([
                { name: 'Name', value: 'buf1', editable: true,
                  editor: { type: 'string' } },
            ]));
            const valEl = app.inspectorEl.querySelector('.inspector-editable');
            assert.ok(valEl, 'value should be contentEditable');
            blurWith(valEl, 'renamed');
            await new Promise(r => setTimeout(r, 0));

            assert.equal(app._requests.length, 1);
            assert.equal(app._requests[0].type, 'set_property');
            assert.equal(app._requests[0].name, 'Name');
            assert.equal(app._requests[0].value, 'renamed');
        });

        it('number editor coerces and rejects non-numbers locally', async () => {
            panel.updateInspector(editableData([
                { name: 'Weight', value: '42', editable: true,
                  editor: { type: 'number' } },
            ]));
            const valEl = app.inspectorEl.querySelector('.inspector-editable');
            blurWith(valEl, 'abc');
            await new Promise(r => setTimeout(r, 0));
            assert.equal(app._requests.length, 0, 'NaN must not reach the server');
            assert.equal(valEl.textContent, '42', 'reverts to old value');

            blurWith(valEl, '5');
            await new Promise(r => setTimeout(r, 0));
            assert.equal(app._requests.length, 1);
            assert.strictEqual(app._requests[0].value, 5);
        });

        it('list editor renders a select and commits by option index', async () => {
            panel.updateInspector(editableData([
                { name: 'Orientation', value: 'R0', editable: true,
                  editor: { type: 'list', options: ['R0', 'R90', 'R180'] } },
            ]));
            const select = app.inspectorEl.querySelector('.inspector-editor-select');
            assert.ok(select, 'list editor should render a select');
            assert.equal(select.value, 'R0');
            select.value = 'R90';
            select.dispatchEvent(new dom.window.Event('change'));
            await new Promise(r => setTimeout(r, 0));

            assert.equal(app._requests[0].type, 'set_property');
            assert.equal(app._requests[0].option_index, 1);
            assert.equal(app._requests[0].option_name, 'R90');
            assert.equal(app._requests[0].value, undefined);
        });

        it('bool editor renders True/False and commits a boolean', async () => {
            panel.updateInspector(editableData([
                { name: 'Dont Touch', value: 'False', editable: true,
                  editor: { type: 'bool' } },
            ]));
            const select = app.inspectorEl.querySelector('.inspector-editor-select');
            assert.ok(select);
            select.value = 'True';
            select.dispatchEvent(new dom.window.Event('change'));
            await new Promise(r => setTimeout(r, 0));

            assert.strictEqual(app._requests[0].value, true);
            assert.equal(app._requests[0].option_index, undefined);
        });

        it('linked editable value keeps the link and offers a pencil', () => {
            panel.updateInspector(editableData([
                { name: 'Master', value: 'BUF_X1', value_select_id: 3,
                  editable: true,
                  editor: { type: 'list', options: ['BUF_X1', 'BUF_X2'] } },
            ]));
            const link = app.inspectorEl.querySelector('.inspector-prop-value');
            assert.ok(link.classList.contains('inspector-link'),
                      'value stays navigable');
            const pencil = app.inspectorEl.querySelector('.inspector-edit-btn');
            assert.ok(pencil, 'pencil button should be present');
            pencil.click();
            const select = app.inspectorEl.querySelector('.inspector-editor-select');
            assert.ok(select, 'pencil swaps in the select');
        });

        it('rejected edit shows a toast and re-renders from the response', async () => {
            app.websocketManager.request = msg => {
                app._requests.push(msg);
                const p = Promise.resolve({
                    ok: 0, error: 'value rejected for property: Name',
                    ...editableData([
                        { name: 'Name', value: 'buf1', editable: true,
                          editor: { type: 'string' } },
                    ]),
                });
                p.requestId = 1;
                return p;
            };
            panel.updateInspector(editableData([
                { name: 'Name', value: 'buf1', editable: true,
                  editor: { type: 'string' } },
            ]));
            blurWith(app.inspectorEl.querySelector('.inspector-editable'),
                     'bad name');
            await new Promise(r => setTimeout(r, 0));

            const toast = document.getElementById('or-toast');
            assert.ok(toast, 'toast should exist');
            assert.ok(toast.classList.contains('visible'));
            assert.ok(toast.textContent.includes('rejected'));
            const valEl = app.inspectorEl.querySelector('.inspector-editable');
            assert.equal(valEl.textContent, 'buf1', 'old value restored');

            // Reset the 4s auto-hide timer so the test runner exits promptly.
            const { showToast } = await import('../../src/ui-utils.js');
            showToast('', 1);
        });

        it('client-side editors (ruler) keep their own onPropertyChange', async () => {
            let changed = null;
            panel.updateInspector({
                type: 'Ruler',
                name: 'r1',
                properties: [
                    { name: 'Label', value: 'x', editable: true },
                ],
                onPropertyChange: (name, val) => { changed = [name, val]; },
            });
            blurWith(app.inspectorEl.querySelector('.inspector-editable'),
                     'new label');
            await new Promise(r => setTimeout(r, 0));

            assert.deepEqual(changed, ['Label', 'new label']);
            assert.equal(app._requests.length, 0, 'no set_property sent');
        });
    });

    describe('descriptor actions', () => {
        function actionData(actions) {
            return {
                type: 'Inst',
                name: 'buf1',
                bbox: [0, 0, 100, 100],
                properties: [{ name: 'Name', value: 'buf1' }],
                actions,
            };
        }

        it('renders one toolbar button per action', () => {
            panel.updateInspector(actionData(['Delete', 'Jump']));
            const btns = app.inspectorEl.querySelectorAll('.inspector-action-btn');
            assert.equal(btns.length, 2);
            assert.equal(btns[0].title, 'Delete');
            assert.equal(btns[1].textContent, 'Jump');
        });

        it('Delete asks for confirmation and sends trigger_action', async () => {
            panel.updateInspector(actionData(['Delete']));
            const btn = app.inspectorEl.querySelector('.inspector-action-btn');
            btn.click();
            await new Promise(r => setTimeout(r, 0));

            const modal = document.querySelector('.or-modal');
            assert.ok(modal, 'confirmation modal should open');
            assert.ok(modal.textContent.includes('cannot be undone'));
            assert.equal(app._requests.length, 0, 'nothing sent before confirm');

            const confirmBtn = modal.querySelector('.or-modal-btn-danger');
            confirmBtn.click();
            await new Promise(r => setTimeout(r, 0));

            assert.equal(app._requests.length, 1);
            assert.equal(app._requests[0].type, 'trigger_action');
            assert.equal(app._requests[0].name, 'Delete');
            assert.equal(document.querySelector('.or-modal'), null,
                         'modal closes');
        });

        it('cancelling the confirmation sends nothing', async () => {
            panel.updateInspector(actionData(['Delete']));
            app.inspectorEl.querySelector('.inspector-action-btn').click();
            await new Promise(r => setTimeout(r, 0));

            const modal = document.querySelector('.or-modal');
            const cancelBtn = modal.querySelector('.or-modal-btn');
            cancelBtn.click();
            await new Promise(r => setTimeout(r, 0));

            assert.equal(app._requests.length, 0);
            assert.equal(document.querySelector('.or-modal'), null);
        });

        it('non-destructive actions send immediately and clear on empty payload', async () => {
            app.websocketManager.request = msg => {
                app._requests.push(msg);
                const p = Promise.resolve({ ok: 1, deleted: 0,
                                            can_navigate_back: 0 });
                p.requestId = 1;
                return p;
            };
            panel.updateInspector(actionData(['Jump']));
            app.inspectorEl.querySelector('.inspector-action-btn').click();
            await new Promise(r => setTimeout(r, 0));

            assert.equal(app._requests[0].type, 'trigger_action');
            assert.equal(app._requests[0].name, 'Jump');
            const stub = app.inspectorEl.querySelector('.stub-panel');
            assert.ok(stub, 'empty payload clears the inspector');
        });
    });

    describe('highlight groups', () => {
        const PALETTE = Array.from({ length: 16 }, (_, i) => [i, 2 * i, 3 * i, 100]);

        function hlData(group) {
            return {
                type: 'Inst', name: 'buf1', bbox: [0, 0, 100, 100],
                highlight_group: group,
                properties: [{ name: 'Name', value: 'buf1' }],
            };
        }

        it('shows the button only for server payloads with a group field', () => {
            app.techData = { highlight_colors: PALETTE };
            panel.updateInspector(hlData(-1));
            assert.ok(app.inspectorEl.querySelector('.inspector-highlight-btn'));

            // Client-side panels (ruler) have no highlight_group → no button.
            panel.updateInspector({
                type: 'Ruler', name: 'r1',
                properties: [{ name: 'Name', value: 'r1' }],
                onPropertyChange: () => {},
            });
            assert.equal(
                app.inspectorEl.querySelector('.inspector-highlight-btn'), null);
        });

        it('opens a 16-swatch picker and sends highlight with the index', async () => {
            app.techData = { highlight_colors: PALETTE };
            panel.updateInspector(hlData(-1));
            app.inspectorEl.querySelector('.inspector-highlight-btn').click();

            const swatches = document.querySelectorAll(
                '#highlight-picker .highlight-swatch');
            assert.equal(swatches.length, 16);
            assert.equal(document.querySelector(
                '#highlight-picker .highlight-remove'), null,
                'no Remove when not highlighted');

            swatches[4].click();
            await new Promise(r => setTimeout(r, 0));
            assert.equal(app._requests[0].type, 'highlight');
            assert.equal(app._requests[0].group, 4);
            assert.equal(document.getElementById('highlight-picker'), null,
                'picker closes after choosing');
        });

        it('badges the current group and offers Remove highlight', async () => {
            app.techData = { highlight_colors: PALETTE };
            panel.updateInspector(hlData(5));
            const btn = app.inspectorEl.querySelector('.inspector-highlight-btn');
            assert.ok(btn.style.borderColor, 'badge shows the group color');
            btn.click();

            const removeBtn = document.querySelector(
                '#highlight-picker .highlight-remove');
            assert.ok(removeBtn, 'Remove offered when already in a group');
            removeBtn.click();
            await new Promise(r => setTimeout(r, 0));
            assert.equal(app._requests[0].type, 'unhighlight');
            assert.equal(document.getElementById('highlight-picker'), null);
        });
    });

    describe('properties rendering', () => {
        it('renders leaf properties', () => {
            panel.updateInspector({
                type: 'Net',
                name: 'sig',
                bbox: [0, 0, 100, 100],
                properties: [
                    { name: 'Name', value: 'sig' },
                    { name: 'Type', value: 'Signal' },
                ],
            });
            const props = app.inspectorEl.querySelectorAll('.inspector-prop');
            assert.equal(props.length, 2);
            assert.equal(props[0].querySelector('.inspector-prop-name').textContent, 'Name');
            assert.equal(props[0].querySelector('.inspector-prop-value').textContent, 'sig');
        });

        it('renders group with children', () => {
            panel.updateInspector({
                type: 'Inst',
                name: 'buf1',
                bbox: [0, 0, 100, 100],
                properties: [{
                    name: 'Pins',
                    children: [
                        { name: 'A', value: 'connected' },
                        { name: 'Z', value: 'connected' },
                    ],
                }],
            });
            const groups = app.inspectorEl.querySelectorAll('.inspector-group');
            assert.equal(groups.length, 1);
            const kids = groups[0].querySelector('.inspector-group-children');
            assert.equal(kids.children.length, 2);
        });
    });
});

// ─── Selection animation (Qt selectionAnimation parity) ────────────────────

function makeLeafletMock() {
    const layers = [];
    globalThis.L = {
        rectangle(bounds, opts) {
            const layer = {
                bounds,
                opts: { ...opts },
                styleHistory: [],
                addTo(map) { map._layers.add(this); return this; },
                setStyle(st) {
                    this.styleHistory.push({ ...st });
                    Object.assign(this.opts, st);
                },
            };
            layers.push(layer);
            return layer;
        },
    };
    return layers;
}

function makeMapMock() {
    return {
        _layers: new Set(),
        hasLayer(l) { return this._layers.has(l); },
        removeLayer(l) { this._layers.delete(l); },
        closePopup() {},
    };
}

describe('animateSelection', () => {
    let app, panel, layers, timers, realSetInterval, realClearInterval;

    beforeEach(() => {
        app = makeApp();
        app.map = makeMapMock();
        app.designScale = 1;
        app.designMaxDXDY = 100000;
        app.designOriginX = 0;
        app.designOriginY = 0;
        app.hoverHighlightPane = 'hoverPane';
        layers = makeLeafletMock();
        panel = createInspectorPanel(app, () => {}, () => {});

        // Deterministic timers: capture the interval callback and tick
        // manually.
        timers = { cb: null, cleared: 0 };
        realSetInterval = globalThis.setInterval;
        realClearInterval = globalThis.clearInterval;
        globalThis.setInterval = (cb) => { timers.cb = cb; return 42; };
        globalThis.clearInterval = () => { timers.cleared++; };
        delete globalThis.matchMedia;
    });

    function restoreTimers() {
        globalThis.setInterval = realSetInterval;
        globalThis.clearInterval = realClearInterval;
    }

    it('creates the layer and cycles weight 1→2→3 with brush flash on 1', () => {
        try {
            panel.animateSelection([0, 0, 100, 100]);
            assert.equal(layers.length, 1);
            const layer = layers[0];
            assert.ok(app.map.hasLayer(layer));
            assert.equal(layer.opts.weight, 1);
            assert.ok(Math.abs(layer.opts.fillOpacity - 0.39) < 1e-9);

            timers.cb();  // state 1 → weight 2, no fill
            assert.equal(layer.opts.weight, 2);
            assert.equal(layer.opts.fillOpacity, 0);
            timers.cb();  // state 2 → weight 3
            assert.equal(layer.opts.weight, 3);
            timers.cb();  // state 3 → weight 1 + flash again
            assert.equal(layer.opts.weight, 1);
            assert.ok(Math.abs(layer.opts.fillOpacity - 0.39) < 1e-9);
        } finally {
            restoreTimers();
        }
    });

    it('finite repeats stop and remove the layer', () => {
        try {
            panel.animateSelection([0, 0, 100, 100], { repeats: 2 });
            const layer = layers[0];
            // maxState = repeats*3 = 6: the 6th tick tears down.
            for (let i = 0; i < 6; i++) timers.cb();
            assert.equal(app.map.hasLayer(layer), false);
            assert.ok(timers.cleared >= 1);
        } finally {
            restoreTimers();
        }
    });

    it('repeats=0 runs until stopSelectionAnimation', () => {
        try {
            panel.animateSelection([0, 0, 100, 100], { repeats: 0 });
            const layer = layers[0];
            for (let i = 0; i < 50; i++) timers.cb();
            assert.ok(app.map.hasLayer(layer), 'still animating after 50 ticks');
            panel.stopSelectionAnimation();
            assert.equal(app.map.hasLayer(layer), false);
            assert.ok(timers.cleared >= 1);
        } finally {
            restoreTimers();
        }
    });

    it('a new animation replaces the previous one', () => {
        try {
            panel.animateSelection([0, 0, 100, 100], { repeats: 0 });
            panel.animateSelection([10, 10, 20, 20], { repeats: 0 });
            assert.equal(layers.length, 2);
            assert.equal(app.map.hasLayer(layers[0]), false);
            assert.ok(app.map.hasLayer(layers[1]));
        } finally {
            restoreTimers();
        }
    });

    it('prefers-reduced-motion falls back to the one-shot pulse', () => {
        try {
            globalThis.matchMedia = () => ({ matches: true });
            panel.animateSelection([0, 0, 100, 100]);
            assert.equal(layers.length, 1);
            assert.equal(layers[0].opts.className, 'selection-pulse');
            assert.equal(timers.cb, null, 'no interval when reduced motion');
        } finally {
            delete globalThis.matchMedia;
            restoreTimers();
        }
    });
});
