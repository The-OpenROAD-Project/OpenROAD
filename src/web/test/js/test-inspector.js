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

        it('keeps the full property name as a tooltip', () => {
            panel.updateInspector({
                type: 'Tech layer',
                name: 'metal1',
                properties: [
                    { name: 'Wrong way minimum width', value: '0.07 um' },
                ],
            });
            const nameEl = app.inspectorEl.querySelector('.inspector-prop-name');
            assert.equal(nameEl.title, 'Wrong way minimum width');
        });
    });

    describe('table properties', () => {
        const tableData = {
            type: 'Tech layer',
            name: 'metal1',
            properties: [{
                name: 'Two width spacing rules',
                table: {
                    column_headers: ['0 um', '0.2 um\nPRL 0.1 um'],
                    row_headers: ['0 um', '0.2 um\nPRL 0.1 um'],
                    data: [['0.065 um', '0.1 um'], ['0.1 um', '0.2 um']],
                },
            }],
        };

        it('renders a grid with header row and header column', () => {
            panel.updateInspector(tableData);
            const table = app.inspectorEl.querySelector('.inspector-table');
            assert.ok(table, 'table should be rendered');
            // Header row: empty corner cell + one cell per column.
            const headRow = table.querySelectorAll('thead tr th');
            assert.equal(headRow.length, 3);
            assert.equal(headRow[0].textContent, '');
            assert.equal(headRow[1].textContent, '0 um');
            assert.equal(headRow[2].textContent, '0.2 um\nPRL 0.1 um');
            // Body rows: row header + one cell per column.
            const bodyRows = table.querySelectorAll('tbody tr');
            assert.equal(bodyRows.length, 2);
            assert.equal(bodyRows[0].children[0].tagName, 'TH');
            assert.equal(bodyRows[0].children[0].textContent, '0 um');
            assert.equal(bodyRows[0].children[1].textContent, '0.065 um');
            assert.equal(bodyRows[1].children[2].textContent, '0.2 um');
        });

        it('omits the header column when a table has no row headers', () => {
            panel.updateInspector({
                type: 'Tech layer',
                name: 'metal1',
                properties: [{
                    name: 'Grid',
                    table: {
                        column_headers: ['a', 'b'],
                        row_headers: ['', ''],
                        data: [['1', '2'], ['3', '4']],
                    },
                }],
            });
            const table = app.inspectorEl.querySelector('.inspector-table');
            assert.equal(table.querySelectorAll('thead tr th').length, 2);
            assert.equal(table.querySelectorAll('tbody tr')[0].children.length, 2);
        });

        it('puts the table in a collapsible group with a row count', () => {
            panel.updateInspector(tableData);
            const group = app.inspectorEl.querySelector('.inspector-group');
            assert.ok(group, 'table should live in a group');
            assert.equal(
                group.querySelector('.inspector-count').textContent, '(2)');
            const body = group.querySelector('.inspector-group-children');
            assert.equal(body.classList.contains('collapsed'), false);
            group.querySelector('.inspector-group-header').dispatchEvent(
                new dom.window.MouseEvent('click', { bubbles: true }));
            assert.equal(body.classList.contains('collapsed'), true);
        });
    });

    describe('name column width', () => {
        function render() {
            panel.updateInspector({
                type: 'Net',
                name: 'sig',
                properties: [{ name: 'Name', value: 'sig' }],
            });
        }

        it('adds a resizer to the property rows', () => {
            render();
            const grip = app.inspectorEl.querySelector('.inspector-col-resizer');
            assert.ok(grip, 'resizer should be present');
            assert.equal(grip.parentElement.className, 'inspector-rows');
        });

        it('drags the column wider and keeps the width across re-renders', () => {
            render();
            const grip = app.inspectorEl.querySelector('.inspector-col-resizer');
            const before = app.inspectorEl.style.getPropertyValue(
                '--inspector-name-w');
            assert.equal(before, '140px');

            grip.dispatchEvent(new dom.window.MouseEvent(
                'mousedown', { clientX: 140, bubbles: true }));
            document.dispatchEvent(new dom.window.MouseEvent(
                'mousemove', { clientX: 200, bubbles: true }));
            document.dispatchEvent(new dom.window.MouseEvent(
                'mouseup', { bubbles: true }));
            assert.equal(
                app.inspectorEl.style.getPropertyValue('--inspector-name-w'),
                '200px');

            // A new selection rebuilds the panel; the width must survive.
            render();
            assert.equal(
                app.inspectorEl.style.getPropertyValue('--inspector-name-w'),
                '200px');
        });

        it('clamps the dragged width to the allowed range', () => {
            render();
            const grip = app.inspectorEl.querySelector('.inspector-col-resizer');
            grip.dispatchEvent(new dom.window.MouseEvent(
                'mousedown', { clientX: 140, bubbles: true }));
            document.dispatchEvent(new dom.window.MouseEvent(
                'mousemove', { clientX: -500, bubbles: true }));
            document.dispatchEvent(new dom.window.MouseEvent(
                'mouseup', { bubbles: true }));
            assert.equal(
                app.inspectorEl.style.getPropertyValue('--inspector-name-w'),
                '60px');
        });
    });

    // The arrow sits inside the clickable header.  It used to carry its own
    // click listener as well, so a click on the arrow toggled twice (once on
    // the arrow, once as the event bubbled to the header) and the group never
    // moved — even though the arrow is the first thing users click.
    describe('group expand/collapse', () => {
        // A group with >= 10 children starts collapsed; fewer starts open.
        function render(childCount) {
            const children = [];
            for (let i = 0; i < childCount; i++) {
                children.push({ name: 'p' + i, value: String(i) });
            }
            panel.updateInspector({
                type: 'Inst',
                name: 'buf1',
                bbox: [0, 0, 100, 100],
                properties: [{ name: 'Pins', children }],
            });
            const group = app.inspectorEl.querySelector('.inspector-group');
            return {
                arrow: group.querySelector('.vis-arrow'),
                header: group.querySelector('.inspector-group-header'),
                kids: group.querySelector('.inspector-group-children'),
            };
        }

        const click = (el) => el.dispatchEvent(
            new dom.window.MouseEvent('click', { bubbles: true }));

        it('clicking the arrow collapses an expanded group', () => {
            const { arrow, kids } = render(2);
            assert.equal(kids.classList.contains('collapsed'), false);
            assert.equal(arrow.textContent, '▼');
            click(arrow);
            assert.equal(kids.classList.contains('collapsed'), true);
            assert.equal(arrow.textContent, '▶');
        });

        it('clicking the arrow expands a collapsed group', () => {
            const { arrow, kids } = render(12);
            assert.equal(kids.classList.contains('collapsed'), true);
            assert.equal(arrow.textContent, '▶');
            click(arrow);
            assert.equal(kids.classList.contains('collapsed'), false);
            assert.equal(arrow.textContent, '▼');
        });

        it('arrow clicks toggle once, not twice', () => {
            const { arrow, kids } = render(2);
            click(arrow);
            click(arrow);
            assert.equal(kids.classList.contains('collapsed'), false);
            assert.equal(arrow.textContent, '▼');
        });

        it('clicking elsewhere in the header still toggles', () => {
            const { header, arrow, kids } = render(2);
            click(header.querySelector('.inspector-prop-name'));
            assert.equal(kids.classList.contains('collapsed'), true);
            assert.equal(arrow.textContent, '▶');
        });
    });
});
