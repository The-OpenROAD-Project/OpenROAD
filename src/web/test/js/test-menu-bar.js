// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { waitForMicrotasks } from './setup-dom.js';
import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { createMenuBar, showFindDialog } from '../../src/menu-bar.js';

describe('MenuBar', () => {
    it('keeps the file dialog open when tcl_eval returns is_error', async () => {
        document.body.innerHTML = '<div id="menu-bar"></div>';
        const requests = [];
        const app = {
            designScale: null,
            websocketManager: {
                request(msg) {
                    requests.push(msg);
                    if (msg.type === 'list_dir') {
                        return Promise.resolve({
                            path: '/work',
                            entries: [{ name: 'bad.odb', is_dir: false, size: 10 }],
                        });
                    }
                    if (msg.type === 'tcl_eval') {
                        return Promise.resolve({
                            output: 'cannot read db',
                            result: 'failed',
                            is_error: true,
                        });
                    }
                    return Promise.resolve({});
                },
            },
            focusComponent() {},
        };

        createMenuBar(app);

        const fileMenu = document.querySelector('.menu-label');
        fileMenu.click();
        const openItem = document.querySelector('.menu-item');
        openItem.click();
        await waitForMicrotasks();

        const input = document.querySelector('.fb-path-input');
        const ok = document.querySelector('.modal-buttons .ok');
        input.value = '/work/bad.odb';
        input.dispatchEvent(new Event('input', { bubbles: true }));
        ok.click();
        await waitForMicrotasks();

        const evalReq = requests.find(req => req.type === 'tcl_eval');
        assert.equal(evalReq.cmd, 'read_db /work/bad.odb');
        assert.ok(document.querySelector('.modal-overlay'),
                  'dialog remains open after Tcl error');
        const error = document.querySelector('.modal-error');
        assert.equal(error.style.display, '');
        assert.equal(error.textContent, 'cannot read db');
        assert.equal(ok.disabled, false);
        assert.equal(ok.textContent, 'Open');
    });
});

// The Find dialog is the web counterpart of `select -type ... -name ...
// -highlight`; `Group` is what makes highlighting a whole MPL cluster one
// request instead of one per instance (issue #7959).
describe('Find dialog', () => {
    function createApp(response) {
        const requests = [];
        return {
            requests,
            designScale: 1,
            showDbu: false,
            // The palette the dialog builds its group list and swatch from
            // comes from the server's tech response (Painter::kHighlightColors,
            // alpha included) rather than a copy in the JS.
            techData: {
                highlight_colors: [[0, 255, 0, 100], [255, 255, 0, 100],
                                   [0, 255, 255, 100]],
            },
            websocketManager: {
                request(msg) {
                    requests.push(msg);
                    return Promise.resolve(response(msg));
                },
            },
            updateInspector(data) { this.inspected = data; },
            refreshOverlay() { this.overlayRefreshed = true; },
            focusComponent() {},
        };
    }

    it('sends the search parameters and reports the match count', async () => {
        document.body.innerHTML = '';
        const app = createApp(() => ({
            found: 12, name: '(root)_glue_logic', type: 'Group',
            properties: [], highlight_truncated: false,
        }));

        showFindDialog(app);
        document.querySelector('.find-type').value = 'Group';
        document.querySelector('.find-pattern').value = '(root)_*';
        document.querySelector('.find-case').checked = true;
        const highlight = document.querySelector('.find-highlight');
        highlight.value = '2';
        highlight.dispatchEvent(new Event('change', { bubbles: true }));
        document.querySelector('.modal-buttons .ok').click();
        await waitForMicrotasks();

        const req = app.requests.find(r => r.type === 'find_objects');
        assert.ok(req, 'find_objects request issued');
        // The descriptor type travels as object_type: `type` is the request
        // envelope's own field.
        assert.equal(req.object_type, 'Group');
        assert.equal(req.pattern, '(root)_*');
        assert.equal(req.case_sensitive, true);
        assert.equal(req.is_regexp, false);
        assert.equal(req.highlight_group, 2);

        // The swatch shows the palette entry, alpha and all.
        assert.equal(document.querySelector('.find-swatch').style
                         .backgroundColor,
                     'rgba(0, 255, 255, 0.392)');

        const info = document.querySelector('.modal-error');
        assert.match(info.textContent, /Found 12/);
        assert.equal(app.inspected.type, 'Group');
        assert.ok(app.overlayRefreshed);
    });

    // The server caps how many objects one find may select, so "Found 50000"
    // would read as an exact answer for a search that actually hit the wall.
    it('says the count is a floor when the server truncated', async () => {
        document.body.innerHTML = '';
        const app = createApp(() => ({
            found: 50000, properties: [], found_truncated: true,
            highlight_truncated: true,
        }));

        showFindDialog(app);
        document.querySelector('.find-pattern').value = '*';
        document.querySelector('.modal-buttons .ok').click();
        await waitForMicrotasks();

        const msg = document.querySelector('.modal-error').textContent;
        assert.match(msg, /more than 50000/);
        assert.match(msg, /Highlight truncated/);
    });

    it('reports when nothing matched', async () => {
        document.body.innerHTML = '';
        const app = createApp(() => ({ found: 0, properties: [] }));

        showFindDialog(app);
        document.querySelector('.find-pattern').value = 'nope*';
        document.querySelector('.modal-buttons .ok').click();
        await waitForMicrotasks();

        assert.match(document.querySelector('.modal-error').textContent,
                     /No matching objects/);
    });

    it('clears highlights without dropping the selection', async () => {
        document.body.innerHTML = '';
        const app = createApp(() => ({ ok: 1 }));

        showFindDialog(app);
        document.querySelector('.modal-buttons .clear').click();
        await waitForMicrotasks();

        const req = app.requests.find(r => r.type === 'clear_highlights');
        assert.ok(req);
        assert.equal(req.keep_selection, true);
    });
});
