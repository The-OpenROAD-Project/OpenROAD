// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it, beforeEach } from 'node:test';
import assert from 'node:assert/strict';
import { createToolbar } from '../../src/toolbar.js';

function makeApp() {
    const requests = [];
    return {
        requests,
        customToolbar: [],
        websocketManager: {
            request(msg) {
                requests.push(msg);
                return Promise.resolve({ result: '', is_error: false });
            },
        },
    };
}

describe('Toolbar', () => {
    beforeEach(() => {
        document.body.innerHTML = '<div id="toolbar"></div>';
    });

    it('renders a button and runs its script via tcl_eval on click', () => {
        const app = makeApp();
        app.customToolbar = [
            { key: 'button0', text: 'Fit', script: 'puts fit', icon: '',
              tooltip: 'Zoom fit', toggle: false, script_off: '', echo: false },
        ];
        createToolbar(app);

        const btn = document.querySelector('#toolbar .toolbar-button');
        assert.ok(btn, 'button rendered');
        assert.equal(btn.title, 'Zoom fit', 'tooltip applied');
        assert.equal(btn.textContent, 'Fit');

        btn.click();
        const evalReq = app.requests.find(r => r.type === 'tcl_eval');
        assert.equal(evalReq.cmd, 'puts fit');
    });

    it('renders an emoji icon and an image icon', () => {
        const app = makeApp();
        app.customToolbar = [
            { key: 'b0', text: 'DRC', script: 'x', icon: '🔍', tooltip: '',
              toggle: false, script_off: '', echo: false },
            { key: 'b1', text: 'Logo', script: 'y',
              icon: 'data:image/png;base64,AAAA', tooltip: '',
              toggle: false, script_off: '', echo: false },
        ];
        createToolbar(app);

        const icons = document.querySelectorAll('#toolbar .toolbar-icon');
        assert.equal(icons.length, 2);
        assert.equal(icons[0].textContent, '🔍', 'emoji icon as text');
        assert.ok(icons[1].querySelector('img'), 'data URI icon as <img>');
        assert.equal(icons[1].querySelector('img').src, 'data:image/png;base64,AAAA');
    });

    it('toggles between script and script_off, tracking active state', () => {
        const app = makeApp();
        app.customToolbar = [
            { key: 'freeze', text: 'Freeze', script: 'set frozen 1',
              icon: '', tooltip: '', toggle: true, script_off: 'set frozen 0',
              echo: false },
        ];
        createToolbar(app);

        const btn = document.querySelector('#toolbar .toolbar-button');
        assert.equal(btn.classList.contains('active'), false);

        btn.click();  // turn on
        assert.equal(btn.classList.contains('active'), true);
        assert.equal(app.requests.at(-1).cmd, 'set frozen 1');

        btn.click();  // turn off
        assert.equal(btn.classList.contains('active'), false);
        assert.equal(app.requests.at(-1).cmd, 'set frozen 0');
    });

    it('echoes the command through app.runTclScript when echo is set', () => {
        const app = makeApp();
        const echoed = [];
        app.runTclScript = (script, echo) => {
            echoed.push({ script, echo });
            return Promise.resolve();
        };
        app.customToolbar = [
            { key: 'b0', text: 'Run', script: 'report_checks', icon: '',
              tooltip: '', toggle: false, script_off: '', echo: true },
        ];
        createToolbar(app);
        document.querySelector('#toolbar .toolbar-button').click();
        assert.deepEqual(echoed, [{ script: 'report_checks', echo: true }]);
        // Should not fall back to the bare request path.
        assert.equal(app.requests.length, 0);
    });

    it('rebuild() re-renders from the updated registry and drops stale toggles', () => {
        const app = makeApp();
        app.customToolbar = [
            { key: 'a', text: 'A', script: 'x', icon: '', tooltip: '',
              toggle: true, script_off: 'y', echo: false },
        ];
        createToolbar(app);
        // Turn A on, then remove it and add a fresh B.
        document.querySelector('.toolbar-button').click();
        assert.ok(app._toolbarToggleState.a);

        app.customToolbar = [
            { key: 'b', text: 'B', script: 'z', icon: '', tooltip: '',
              toggle: false, script_off: '', echo: false },
        ];
        app.rebuildToolbar();

        const buttons = document.querySelectorAll('.toolbar-button');
        assert.equal(buttons.length, 1);
        assert.equal(buttons[0].dataset.key, 'b');
        assert.equal(app._toolbarToggleState.a, undefined,
                     'stale toggle state pruned');
    });
});
