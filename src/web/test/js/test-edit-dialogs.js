// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it, beforeEach } from 'node:test';
import assert from 'node:assert/strict';
import {
    showGlobalConnectDialog,
    showInsertBufferDialog,
} from '../../src/edit-dialogs.js';

function waitMicrotasks() {
    return new Promise(resolve => setTimeout(resolve, 0));
}

// Build an app whose websocketManager.request is driven by `responder`.
function makeApp(responder) {
    const requests = [];
    return {
        requests,
        scheduleRedrawAllLayers() { this._redrew = true; },
        websocketManager: {
            request(msg) {
                requests.push(msg);
                return Promise.resolve(responder(msg) ?? {});
            },
        },
    };
}

describe('Global Connect dialog', () => {
    beforeEach(() => { document.body.innerHTML = ''; });

    it('lists rules, populates nets, and blocks Add on invalid regex', async () => {
        const app = makeApp(msg => {
            if (msg.type === 'global_connect_info') {
                return {
                    rules: [{ inst: '.*', pin: '^VDD$', net: 'VDD', region: '' }],
                    nets: ['VDD', 'VSS'],
                    regions: [],
                };
            }
            return {};
        });
        showGlobalConnectDialog(app);
        await waitMicrotasks();

        // Existing rule rendered (head row + 1 data row).
        assert.equal(document.querySelectorAll('.gc-row').length, 2);
        // Net dropdown populated with special nets.
        const netSel = document.querySelectorAll('.gc-form select')[0];
        assert.deepEqual([...netSel.options].map(o => o.value), ['VDD', 'VSS']);

        const pin = document.querySelectorAll('.gc-form input')[1];
        const addBtn = [...document.querySelectorAll('.gc-form button')]
            .find(b => b.textContent === 'Add rule');
        // Empty pin → disabled.
        assert.equal(addBtn.disabled, true);
        pin.value = '[';  // invalid regex
        pin.dispatchEvent(new Event('input', { bubbles: true }));
        assert.equal(addBtn.disabled, true);
        pin.value = '^VDD$';
        pin.dispatchEvent(new Event('input', { bubbles: true }));
        assert.equal(addBtn.disabled, false);
    });

    it('Add builds the add_global_connection command', async () => {
        const app = makeApp(msg => {
            if (msg.type === 'global_connect_info') {
                return { rules: [], nets: ['VDD'], regions: [] };
            }
            if (msg.type === 'tcl_eval') return { result: 'Connected 3 pins', is_error: false };
            return {};
        });
        showGlobalConnectDialog(app);
        await waitMicrotasks();

        const pin = document.querySelectorAll('.gc-form input')[1];
        pin.value = '^VDD$';
        pin.dispatchEvent(new Event('input', { bubbles: true }));
        [...document.querySelectorAll('.gc-form button')]
            .find(b => b.textContent === 'Add rule').click();
        await waitMicrotasks();

        const ev = app.requests.find(r => r.type === 'tcl_eval');
        assert.equal(ev.cmd,
            'add_global_connection -net {VDD} -inst_pattern {.*} -pin_pattern {^VDD$} -power');
    });

    it('Apply (force) shows the connected count via global_connect_apply', async () => {
        const app = makeApp(msg => {
            if (msg.type === 'global_connect_info') return { rules: [], nets: ['VDD'], regions: [] };
            if (msg.type === 'global_connect_apply') return { ok: true, had_rules: true, connected: 7 };
            return {};
        });
        showGlobalConnectDialog(app);
        await waitMicrotasks();
        [...document.querySelectorAll('.modal-buttons button')]
            .find(b => b.textContent === 'Apply (force)').click();
        await waitMicrotasks();
        const applyReq = app.requests.find(r => r.type === 'global_connect_apply');
        assert.equal(applyReq.force, true);
        assert.match(document.querySelector('.modal-error').textContent, /Connected 7 pin/);
    });

    it('Apply (force) with no rules shows a friendly message (no ODB-0378)', async () => {
        const app = makeApp(msg => {
            if (msg.type === 'global_connect_info') return { rules: [], nets: [], regions: [] };
            if (msg.type === 'global_connect_apply') return { ok: true, had_rules: false, connected: 0 };
            return {};
        });
        showGlobalConnectDialog(app);
        await waitMicrotasks();
        [...document.querySelectorAll('.modal-buttons button')]
            .find(b => b.textContent === 'Apply (force)').click();
        await waitMicrotasks();
        assert.match(document.querySelector('.modal-error').textContent, /No global-connect rules/);
    });

    it('per-row delete calls global_connect_delete with the rule fields', async () => {
        const app = makeApp(msg => {
            if (msg.type === 'global_connect_info') {
                return { rules: [{ inst: '.*', pin: 'P', net: 'VDD', region: '' }], nets: ['VDD'], regions: [] };
            }
            return {};
        });
        showGlobalConnectDialog(app);
        await waitMicrotasks();
        document.querySelector('.gc-row:not(.gc-head) button').click();
        await waitMicrotasks();
        const del = app.requests.find(r => r.type === 'global_connect_delete');
        assert.equal('index' in del, false);
        assert.deepEqual(
            { inst: del.inst, pin: del.pin, net: del.net, region: del.region },
            { inst: '.*', pin: 'P', net: 'VDD', region: '' });
    });
});

describe('Insert Buffer dialog', () => {
    beforeEach(() => { document.body.innerHTML = ''; });

    it('disables OK and shows a message when the net cannot be buffered', async () => {
        const app = makeApp(() => ({ can_buffer: false, error: 'multiple drivers' }));
        showInsertBufferDialog(app, 'n1');
        await waitMicrotasks();
        assert.match(document.querySelector('.modal-body').textContent, /multiple drivers/);
        const ok = [...document.querySelectorAll('.modal-buttons button')]
            .find(b => b.textContent === 'Insert');
        assert.equal(ok.disabled, true);
    });

    it('sends insert_buffer with the driver by default', async () => {
        const app = makeApp(msg => {
            if (msg.type === 'buffer_info') {
                return {
                    can_buffer: true,
                    drivers: [{ id: 'I:drv/Z', label: 'drv/Z (Driver)' }],
                    loads: [{ id: 'I:ld/A', label: 'ld/A (Load)' }],
                    masters: ['BUF_X1', 'BUF_X2'],
                };
            }
            if (msg.type === 'insert_buffer') return { ok: true, inst: 'buf0' };
            return {};
        });
        showInsertBufferDialog(app, 'n1');
        await waitMicrotasks();

        const ok = [...document.querySelectorAll('.modal-buttons button')]
            .find(b => b.textContent === 'Insert');
        assert.equal(ok.disabled, false);
        ok.click();
        await waitMicrotasks();

        const ins = app.requests.find(r => r.type === 'insert_buffer');
        assert.equal(ins.mode, 'driver');
        assert.deepEqual(ins.pins, ['I:drv/Z']);
        assert.equal(ins.master, 'BUF_X1');
        assert.equal(app.selectedInstanceName, 'buf0');
        assert.equal(app._redrew, true);
    });

    it('switches to loads mode when a load pin is picked', async () => {
        const app = makeApp(msg => {
            if (msg.type === 'buffer_info') {
                return {
                    can_buffer: true,
                    drivers: [{ id: 'I:drv/Z', label: 'drv/Z (Driver)' }],
                    loads: [{ id: 'I:ld/A', label: 'ld/A (Load)' }],
                    masters: ['BUF_X1'],
                };
            }
            if (msg.type === 'insert_buffer') return { ok: true, inst: 'buf0' };
            return {};
        });
        showInsertBufferDialog(app, 'n1');
        await waitMicrotasks();

        // Check the load pin (second checkbox) → driver auto-deselects.
        const boxes = document.querySelectorAll('.buf-pin input');
        boxes[1].checked = true;
        boxes[1].dispatchEvent(new Event('change', { bubbles: true }));
        await waitMicrotasks();

        [...document.querySelectorAll('.modal-buttons button')]
            .find(b => b.textContent === 'Insert').click();
        await waitMicrotasks();

        const ins = app.requests.find(r => r.type === 'insert_buffer');
        assert.equal(ins.mode, 'loads');
        assert.deepEqual(ins.pins, ['I:ld/A']);
    });

    it('does not send x/y and has no position/move UI', async () => {
        const app = makeApp(msg => {
            if (msg.type === 'buffer_info') {
                return {
                    can_buffer: true,
                    drivers: [{ id: 'I:drv/Z', label: 'drv/Z (Driver)' }],
                    loads: [],
                    masters: ['BUF_X1'],
                };
            }
            if (msg.type === 'insert_buffer') return { ok: true, inst: 'buf0' };
            return {};
        });
        showInsertBufferDialog(app, 'n1');
        await waitMicrotasks();
        [...document.querySelectorAll('.modal-buttons button')]
            .find(b => b.textContent === 'Insert').click();
        await waitMicrotasks();

        const ins = app.requests.find(r => r.type === 'insert_buffer');
        assert.equal('x' in ins, false);
        assert.equal('y' in ins, false);
        // No location/move UI remains in the dialog.
        assert.equal(document.querySelectorAll('.buf-dialog input[type="number"]').length, 0);
        assert.equal([...document.querySelectorAll('.buf-dialog button')]
            .some(b => b.textContent === 'Pick location'), false);
    });
});
