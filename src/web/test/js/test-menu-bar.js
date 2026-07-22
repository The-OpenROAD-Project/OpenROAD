// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { createMenuBar } from '../../src/menu-bar.js';

function waitForMicrotasks() {
    return new Promise(resolve => setTimeout(resolve, 0));
}

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

    it('renders Tcl-registered custom items with a hierarchical path', () => {
        document.body.innerHTML = '<div id="menu-bar"></div>';
        const requests = [];
        const app = {
            designScale: 1,
            websocketManager: {
                request(msg) {
                    requests.push(msg);
                    return Promise.resolve({ result: '', is_error: false });
                },
            },
            focusComponent() {},
            customMenu: [
                // New top-level menu (default "Custom Scripts").
                { key: 'action0', path: 'Custom Scripts', text: 'Hello',
                  script: 'puts hi', shortcut: '', echo: false },
                // Nested submenu under a brand-new top-level "Flow".
                { key: 'action1', path: 'Flow/Place', text: 'Global Place',
                  script: 'global_placement', shortcut: '', echo: false },
            ],
        };

        createMenuBar(app);

        const labels = [...document.querySelectorAll('.menu-label')]
            .map(el => el.firstChild.textContent);
        assert.ok(labels.includes('Custom Scripts'), 'default top menu created');
        assert.ok(labels.includes('Flow'), 'path top-level menu created');

        // The Flow menu should contain a submenu "Place" with the leaf item.
        const flowLabel = [...document.querySelectorAll('.menu-label')]
            .find(el => el.firstChild.textContent === 'Flow');
        const submenu = flowLabel.querySelector('.submenu-item');
        assert.ok(submenu, 'submenu row created for nested path');
        const leaf = submenu.querySelector('.menu-dropdown.submenu .menu-item');
        assert.ok(leaf.textContent.includes('Global Place'));

        // Clicking the leaf runs its script through tcl_eval.
        leaf.click();
        const evalReq = requests.find(r => r.type === 'tcl_eval');
        assert.equal(evalReq.cmd, 'global_placement');
    });

    it('rebuildMenuBar re-renders after the custom registry changes', () => {
        document.body.innerHTML = '<div id="menu-bar"></div>';
        const app = {
            designScale: 1,
            websocketManager: { request: () => Promise.resolve({}) },
            focusComponent() {},
            customMenu: [],
        };
        createMenuBar(app);
        let labels = [...document.querySelectorAll('.menu-label')]
            .map(el => el.firstChild.textContent);
        assert.equal(labels.includes('Custom Scripts'), false);

        app.customMenu = [
            { key: 'a', path: 'Custom Scripts', text: 'X', script: 'x',
              shortcut: '', echo: false },
        ];
        app.rebuildMenuBar();
        labels = [...document.querySelectorAll('.menu-label')]
            .map(el => el.firstChild.textContent);
        assert.ok(labels.includes('Custom Scripts'), 'menu appears after rebuild');
    });
});
