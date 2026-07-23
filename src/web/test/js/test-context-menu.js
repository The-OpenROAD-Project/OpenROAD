// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { ContextMenu } from '../../src/context-menu.js';

// Build a mock `app` whose map records setZoomAround/fitBounds calls.
function makeApp(overrides = {}) {
    const calls = [];
    const app = {
        map: {
            _zoom: 5,
            options: { zoomDelta: 1 },
            on() {},
            getZoom() { return this._zoom; },
            setZoomAround(latlng, z) { calls.push(['setZoomAround', latlng, z]); },
            fitBounds(b) { calls.push(['fitBounds', b]); },
            getContainer() { return document.body; },
        },
        fitBounds: [[0, 0], [10, 10]],
        lastSelectionBounds: [[1, 1], [2, 2]],
        hasSelection: true,
        selHasInst: true,
        selHasNet: true,
        ...overrides,
    };
    return { app, calls };
}

function disabledLabels() {
    return [...document.querySelectorAll('.cm-item.disabled')]
        .map((el) => el.textContent);
}

// Click the leaf menu item whose text is exactly `label`.
function clickItem(label) {
    const items = [...document.querySelectorAll('.cm-item')];
    const el = items.find(i => i.textContent === label);
    assert.ok(el, `menu item "${label}" not found`);
    el.click();
}

function openMenu(app) {
    document.body.innerHTML = '';
    const cm = new ContextMenu(app);
    cm.show({ originalEvent: { clientX: 5, clientY: 5 } });
    return cm;
}

describe('ContextMenu selection-dependent items (type-aware)', () => {
    it('nothing selected → Select and Highlight parents disabled', () => {
        const { app } = makeApp({ selHasInst: false, selHasNet: false });
        openMenu(app);
        const d = disabledLabels();
        assert.ok(d.includes('Select'), 'Select should be disabled');
        assert.ok(d.includes('Highlight'), 'Highlight should be disabled');
        // A disabled parent must not build its submenu.
        const select = [...document.querySelectorAll('.cm-item')]
            .find((el) => el.textContent === 'Select');
        assert.equal(select.querySelector('.cm-submenu'), null);
    });

    it('instance selected → net/buffer items enabled, Connected Insts disabled', () => {
        const { app } = makeApp({ selHasInst: true, selHasNet: false });
        openMenu(app);
        const d = disabledLabels();
        assert.ok(!d.includes('Select'), 'Select parent enabled');
        assert.ok(d.includes('Connected Insts'), 'Connected Insts needs a net');
        assert.ok(!d.includes('Output Nets'), 'net items enabled for an instance');
    });

    it('net selected → only Connected Insts enabled', () => {
        const { app } = makeApp({ selHasInst: false, selHasNet: true });
        openMenu(app);
        const d = disabledLabels();
        assert.ok(!d.includes('Select'), 'Select parent enabled');
        assert.ok(!d.includes('Connected Insts'), 'Connected Insts enabled for a net');
        assert.ok(d.includes('Output Nets'), 'net items need an instance');
    });
});

describe('ContextMenu has no View submenu', () => {
    it('does not expose Zoom/View items (moved to the on-canvas Fit control)', () => {
        const { app } = makeApp();
        openMenu(app);
        const labels = [...document.querySelectorAll('.cm-item')]
            .map((el) => el.textContent);
        assert.ok(!labels.includes('Zoom In'));
        assert.ok(!labels.includes('Zoom to Selection'));
        assert.ok(!labels.some((l) => l === 'View'));
    });
});

describe('ContextMenu Save submenu', () => {
    it('Save items call the WYSIWYG capture with the right scope', () => {
        const calls = [];
        const { app } = makeApp({ captureLayout: (opts) => calls.push(opts) });
        openMenu(app);
        clickItem('Entire layout');
        clickItem('Visible layout');
        assert.deepEqual(calls, [{ entire: true }, { entire: false }]);
    });
});

describe('ContextMenu Clear submenu', () => {
    function appWithWs() {
        const sent = [];
        const cleared = [];
        const { app } = makeApp({
            websocketManager: { request(m) { sent.push(m); return Promise.resolve({}); } },
            rulerManager: { clearAllRulers() { cleared.push('rulers'); } },
        });
        return { app, sent, cleared };
    }

    it('Selections / Focus nets / Route Guides send the right action', () => {
        for (const [label, action] of [
            ['Selections', 'clear_selections'],
            ['Focus nets', 'clear_focus_nets'],
            ['Route Guides', 'clear_route_guides'],
        ]) {
            const { app, sent } = appWithWs();
            openMenu(app);
            clickItem(label);
            assert.equal(sent[0].type, 'context_action');
            assert.equal(sent[0].action, action);
        }
    });

    it('Rulers clears client-side rulers without a server call', () => {
        const { app, sent, cleared } = appWithWs();
        openMenu(app);
        clickItem('Rulers');
        assert.deepEqual(cleared, ['rulers']);
        assert.equal(sent.length, 0);
    });

    it('All sends clear_all and clears rulers', () => {
        const { app, sent, cleared } = appWithWs();
        openMenu(app);
        clickItem('All');
        assert.equal(sent[0].action, 'clear_all');
        assert.deepEqual(cleared, ['rulers']);
    });
});
