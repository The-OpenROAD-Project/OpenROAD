// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import {
    createMenuBar,
    canonicalShortcut,
    eventShortcut,
} from '../../src/menu-bar.js';

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
    it('binds a custom item\'s -shortcut to the key', () => {
        document.body.innerHTML = '<div id="menu-bar"></div>';
        const requests = [];
        const app = {
            designScale: 1,
            websocketManager: {
                request(msg) { requests.push(msg); return Promise.resolve({}); },
            },
            focusComponent() {},
            customMenu: [
                { key: 'a', path: '', text: 'Slack', script: 'report_worst_slack',
                  shortcut: 'Ctrl+Shift+H', echo: false },
            ],
        };
        createMenuBar(app);

        document.dispatchEvent(new window.KeyboardEvent('keydown', {
            key: 'H', ctrlKey: true, shiftKey: true, bubbles: true,
        }));
        assert.deepEqual(requests.map(r => r.cmd), ['report_worst_slack']);

        // A near miss (wrong modifier set) must not fire.
        document.dispatchEvent(new window.KeyboardEvent('keydown', {
            key: 'H', ctrlKey: true, bubbles: true,
        }));
        assert.equal(requests.length, 1);
    });

    it('a custom shortcut overrides a built-in key rather than firing both',
       () => {
        document.body.innerHTML = '<div id="menu-bar"></div>';
        const requests = [];
        let builtinRan = false;
        const app = {
            designScale: 1,
            websocketManager: {
                request(msg) { requests.push(msg); return Promise.resolve({}); },
            },
            focusComponent() {},
            customMenu: [
                { key: 'a', path: '', text: 'Mine', script: 'puts mine',
                  shortcut: 'T', echo: false },
            ],
        };
        // createMenuBar registers first, as it does in main.js, so a later
        // capture listener stands in for main.js's built-in chain.
        createMenuBar(app);
        document.addEventListener('keydown', () => { builtinRan = true; }, true);

        document.dispatchEvent(new window.KeyboardEvent('keydown', {
            key: 't', bubbles: true,
        }));
        assert.deepEqual(requests.map(r => r.cmd), ['puts mine']);
        assert.equal(builtinRan, false, 'built-in handler was not also run');
    });

    it('ignores shortcuts while typing in a field', () => {
        document.body.innerHTML
            = '<div id="menu-bar"></div><input id="f"><textarea id="t"></textarea>';
        const requests = [];
        const app = {
            designScale: 1,
            websocketManager: {
                request(msg) { requests.push(msg); return Promise.resolve({}); },
            },
            focusComponent() {},
            customMenu: [
                { key: 'a', path: '', text: 'X', script: 'x', shortcut: 'K',
                  echo: false },
            ],
        };
        createMenuBar(app);

        for (const id of ['f', 't']) {
            document.getElementById(id).dispatchEvent(
                new window.KeyboardEvent('keydown', { key: 'k', bubbles: true }));
        }
        assert.equal(requests.length, 0);
    });

    // setup-dom.js hands every test in this file the same jsdom document, and
    // each createMenuBar leaves its own capture listener on it.  So each
    // shortcut test claims a chord of its own; sharing one would let an
    // earlier test's listener answer first and swallow the keystroke.
    it('a rebuild drops the shortcut of an item that went away', () => {
        document.body.innerHTML = '<div id="menu-bar"></div>';
        const requests = [];
        const app = {
            designScale: 1,
            websocketManager: {
                request(msg) { requests.push(msg); return Promise.resolve({}); },
            },
            focusComponent() {},
            customMenu: [
                { key: 'a', path: '', text: 'X', script: 'x', shortcut: 'J',
                  echo: false },
            ],
        };
        createMenuBar(app);
        document.dispatchEvent(new window.KeyboardEvent('keydown',
                                                        { key: 'j', bubbles: true }));
        assert.equal(requests.length, 1);

        app.customMenu = [];
        app.rebuildMenuBar();
        document.dispatchEvent(new window.KeyboardEvent('keydown',
                                                        { key: 'j', bubbles: true }));
        assert.equal(requests.length, 1, 'removed item no longer bound');
    });

    it('canonicalShortcut normalizes and rejects unbindable specs', () => {
        assert.equal(canonicalShortcut('Ctrl+H'), 'ctrl+h');
        // Modifier order in the spec does not matter; the canonical one is fixed.
        assert.equal(canonicalShortcut('shift+CTRL+k'), 'ctrl+shift+k');
        assert.equal(canonicalShortcut(' Cmd + S '), 'meta+s');
        assert.equal(canonicalShortcut('F'), 'f');
        assert.equal(canonicalShortcut('Escape'), 'escape');
        // Nothing left to bind, or a modifier we do not know: refuse, rather
        // than bind a chord the author did not ask for.
        assert.equal(canonicalShortcut(''), null);
        assert.equal(canonicalShortcut('Ctrl+'), null);
        assert.equal(canonicalShortcut('Ctrl+Shift'), null);
        assert.equal(canonicalShortcut('Hyper+K'), null);
        assert.equal(canonicalShortcut(undefined), null);
    });

    it('eventShortcut matches canonicalShortcut for the same chord', () => {
        const e = new window.KeyboardEvent('keydown',
                                           { key: 'K', ctrlKey: true, shiftKey: true });
        assert.equal(eventShortcut(e), canonicalShortcut('Ctrl+Shift+K'));
    });
});
