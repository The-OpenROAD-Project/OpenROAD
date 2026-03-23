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
