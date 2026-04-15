// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it, beforeEach } from 'node:test';
import assert from 'node:assert/strict';
import { DrcWidget } from '../../src/drc-widget.js';

// Mock app with a fake WebSocket manager.
function createMockApp(responses = {}) {
    return {
        websocketManager: {
            request(msg) {
                const type = msg.type;
                if (responses[type]) {
                    return Promise.resolve(responses[type](msg));
                }
                return Promise.resolve({});
            },
        },
        designScale: 1,
        designMaxDXDY: 100000,
        designOriginX: 0,
        designOriginY: 0,
        map: {
            fitBounds() {},
        },
        focusComponent() {},
    };
}

describe('DrcWidget', () => {
    describe('construction', () => {
        it('creates the expected DOM structure', () => {
            const app = createMockApp({
                drc_categories: () => ({ categories: [] }),
            });
            const widget = new DrcWidget(app, () => {});

            assert.ok(widget.element);
            assert.ok(widget.element.classList.contains('drc-widget'));

            // Should have toolbar with select and buttons
            const toolbar = widget.element.querySelector('.drc-toolbar');
            assert.ok(toolbar, 'toolbar exists');

            const select = toolbar.querySelector('.drc-category-select');
            assert.ok(select, 'category select exists');

            const buttons = toolbar.querySelectorAll('.drc-btn');
            assert.ok(buttons.length >= 2, 'load and refresh buttons exist');

            // Should have info bar
            const infoBar = widget.element.querySelector('.drc-info-bar');
            assert.ok(infoBar, 'info bar exists');

            // Should have tree container
            const tree = widget.element.querySelector('.drc-tree-container');
            assert.ok(tree, 'tree container exists');
        });
    });

    describe('category loading', () => {
        it('populates the category dropdown from server response', async () => {
            const app = createMockApp({
                drc_categories: () => ({
                    categories: [
                        { name: 'DRC', count: 5 },
                        { name: 'LVS', count: 2 },
                    ],
                }),
            });
            const widget = new DrcWidget(app, () => {});

            // Wait for async load
            await new Promise(r => setTimeout(r, 50));

            const select = widget.element.querySelector('.drc-category-select');
            const options = select.querySelectorAll('option');
            // Should have (none) + DRC + LVS = 3 options
            assert.equal(options.length, 3);
            assert.equal(options[0].value, '');
            assert.equal(options[1].value, 'DRC');
            assert.equal(options[2].value, 'LVS');
        });
    });

    describe('marker tree rendering', () => {
        it('renders subcategories and markers', async () => {
            let markerRequest = null;
            const app = createMockApp({
                drc_categories: () => ({
                    categories: [{ name: 'DRC', count: 2 }],
                }),
                drc_markers: (msg) => {
                    markerRequest = msg;
                    return {
                        name: 'DRC',
                        total_count: 2,
                        subcategories: [{
                            name: 'MinSpacing',
                            count: 2,
                            markers: [
                                { id: 1, index: 1, name: 'MinSpacing violation',
                                  visited: false, visible: true, waived: false,
                                  bbox: [0, 0, 100, 100], layer: 'metal1' },
                                { id: 2, index: 2, name: 'MinSpacing violation',
                                  visited: true, visible: true, waived: false,
                                  bbox: [200, 0, 300, 100], layer: 'metal2' },
                            ],
                        }],
                    };
                },
            });
            const widget = new DrcWidget(app, () => {});

            // Wait for categories to load
            await new Promise(r => setTimeout(r, 50));

            // Manually trigger category selection
            widget.selectCategory('DRC');

            // Wait for markers to load
            await new Promise(r => setTimeout(r, 50));

            assert.ok(markerRequest, 'marker request was made');
            assert.equal(markerRequest.category, 'DRC');

            // Check tree structure
            const tree = widget.element.querySelector('.drc-tree');
            assert.ok(tree, 'tree rendered');

            const categoryHeaders = tree.querySelectorAll('.drc-category-label');
            assert.ok(categoryHeaders.length >= 1);
            assert.equal(categoryHeaders[0].textContent, 'MinSpacing');

            // Info bar should show count
            const infoBar = widget.element.querySelector('.drc-info-bar');
            assert.ok(infoBar.textContent.includes('2'));
        });

        it('marks unvisited markers as bold', async () => {
            const app = createMockApp({
                drc_categories: () => ({
                    categories: [{ name: 'DRC', count: 1 }],
                }),
                drc_markers: () => ({
                    name: 'DRC',
                    total_count: 1,
                    subcategories: [{
                        name: 'Short',
                        count: 1,
                        markers: [
                            { id: 1, index: 1, name: 'Short violation',
                              visited: false, visible: true, waived: false,
                              bbox: [0, 0, 100, 100] },
                        ],
                    }],
                }),
            });
            const widget = new DrcWidget(app, () => {});
            await new Promise(r => setTimeout(r, 50));
            widget.selectCategory('DRC');
            await new Promise(r => setTimeout(r, 50));

            // Expand the category node to see markers
            const headers = widget.element.querySelectorAll('.drc-tree-header');
            if (headers.length > 0) {
                headers[0].click();
            }

            const markerRows = widget.element.querySelectorAll('.drc-marker-row');
            assert.ok(markerRows.length >= 1, 'at least one marker row');
            assert.ok(markerRows[0].classList.contains('drc-unvisited'),
                'unvisited marker has bold class');
        });
    });

    describe('selectCategory', () => {
        it('updates the dropdown and loads markers', async () => {
            let loadedCategory = null;
            const app = createMockApp({
                drc_categories: () => ({
                    categories: [{ name: 'DRC', count: 3 }],
                }),
                drc_markers: (msg) => {
                    loadedCategory = msg.category;
                    return {
                        name: 'DRC', total_count: 0,
                        subcategories: [],
                    };
                },
            });
            const widget = new DrcWidget(app, () => {});
            await new Promise(r => setTimeout(r, 50));

            widget.selectCategory('DRC');
            await new Promise(r => setTimeout(r, 50));

            assert.equal(loadedCategory, 'DRC');
        });
    });
});
