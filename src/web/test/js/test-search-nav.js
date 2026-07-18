// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { showFindDialog, showGotoDialog } from '../../src/search-nav.js';
import { dbuToLatLng, dbuRectToBounds } from '../../src/coordinates.js';

const wait = () => new Promise((r) => setTimeout(r, 0));

// scale=1, maxDXDY=0, origins=0, 1000 dbu/µm keeps the math easy to assert.
function makeApp(overrides = {}) {
    const calls = [];
    const app = {
        designScale: 1, designMaxDXDY: 0, designOriginX: 0, designOriginY: 0,
        getDbuPerMicron: () => 1000,
        map: {
            _zoom: 3,
            getZoom() { return this._zoom; },
            getCenter() { return { lat: 0, lng: 0 }; },
            setView(latlng, z) { calls.push(['setView', latlng, z]); },
            fitBounds(b) { calls.push(['fitBounds', b]); },
        },
        ...overrides,
    };
    return { app, calls };
}

function clickOk() { document.querySelector('.modal-dialog .ok').click(); }
function setInput(sel, val) {
    const el = document.querySelector(sel);
    el.value = val;
    return el;
}
function errorText() {
    const e = document.querySelector('.modal-error');
    return e && e.style.display !== 'none' ? e.textContent : '';
}

describe('Go to Position dialog', () => {
    it('centers the map on the x,y converted from microns', () => {
        document.body.innerHTML = '';
        const { app, calls } = makeApp();
        showGotoDialog(app);
        setInput('.sn-x', '5');   // 5 µm -> 5000 dbu
        setInput('.sn-y', '7');   // 7 µm -> 7000 dbu
        clickOk();
        const expected = dbuToLatLng(5000, 7000, 1, 0, 0, 0);
        assert.deepEqual(calls, [['setView', expected, 3]]);
    });

    it('fits a Size window when Size is provided', () => {
        document.body.innerHTML = '';
        const { app, calls } = makeApp();
        showGotoDialog(app);
        setInput('.sn-x', '10');
        setInput('.sn-y', '10');
        setInput('.sn-size', '4');  // 4 µm window => half = 2000 dbu
        clickOk();
        const expected = dbuRectToBounds(10000 - 2000, 10000 - 2000,
            10000 + 2000, 10000 + 2000, 1, 0, 0, 0);
        assert.deepEqual(calls, [['fitBounds', expected]]);
    });

    it('rejects non-numeric coordinates', () => {
        document.body.innerHTML = '';
        const { app, calls } = makeApp();
        showGotoDialog(app);
        setInput('.sn-x', 'abc');
        clickOk();
        assert.equal(calls.length, 0);
        assert.match(errorText(), /must be numbers/i);
    });
});

describe('Find dialog', () => {
    function findApp(response) {
        const sent = [];
        const { app, calls } = makeApp({
            websocketManager: { request(m) { sent.push(m); return Promise.resolve(response); } },
            updateInspector() {},
            redrawAllLayers() {},
        });
        return { app, calls, sent };
    }

    it('sends a find request and auto-zooms to the result bbox', async () => {
        document.body.innerHTML = '';
        const { app, calls, sent } = findApp(
            { count: 2, selection_count: 2, bbox: [0, 0, 3000, 4000] });
        showFindDialog(app);
        setInput('.sn-pattern', '_4*');
        document.querySelector('.sn-type').value = 'inst';
        clickOk();
        await wait();
        assert.deepEqual(sent, [{ type: 'find', obj_type: 'inst',
            pattern: '_4*', match_case: false }]);
        const expected = dbuRectToBounds(0, 0, 3000, 4000, 1, 0, 0, 0);
        assert.deepEqual(calls, [['fitBounds', expected]]);
        assert.match(errorText(), /2 found/);
    });

    it('reports when nothing is found and does not zoom', async () => {
        document.body.innerHTML = '';
        const { app, calls } = findApp({ count: 0, selection_count: 0 });
        showFindDialog(app);
        setInput('.sn-pattern', 'nope*');
        clickOk();
        await wait();
        assert.equal(calls.length, 0);
        assert.match(errorText(), /No objects found/i);
    });

    it('requires a pattern', () => {
        document.body.innerHTML = '';
        const { app, sent } = findApp({ count: 0 });
        showFindDialog(app);
        clickOk();
        assert.equal(sent.length, 0);
        assert.match(errorText(), /pattern/i);
    });

    it('closes via the X button', () => {
        document.body.innerHTML = '';
        const { app } = findApp({ count: 0 });
        showFindDialog(app);
        assert.ok(document.querySelector('.modal-overlay'), 'dialog open');
        document.querySelector('.modal-close').click();
        assert.equal(document.querySelector('.modal-overlay'), null, 'dialog closed');
    });
});
