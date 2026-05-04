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
});
