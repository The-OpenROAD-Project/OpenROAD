// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { showFindDialog, showGotoDialog } from '../../src/search-nav.js';
import { dbuToLatLng, dbuRectToBounds } from '../../src/coordinates.js';
import { formatDbu, parseDbu, unitLabel } from '../../src/ui-utils.js';

const wait = () => new Promise((r) => setTimeout(r, 0));

// scale=1, maxDXDY=0, origins=0, 1000 dbu/µm keeps the math easy to assert.
function makeApp(overrides = {}) {
    const calls = [];
    const app = {
        designScale: 1, designMaxDXDY: 0, designOriginX: 0, designOriginY: 0,
        showDbu: false,
        getDbuPerMicron: () => 1000,
        // Delegated to the real formatters (see test-ruler.js).
        unitOpts() {
            return { showDbu: this.showDbu,
                     dbuPerMicron: this.getDbuPerMicron() };
        },
        formatDbu(v, addUnits = false) {
            return formatDbu(v, this.unitOpts(), addUnits);
        },
        parseDbu(s) { return parseDbu(s, this.unitOpts()); },
        unitLabel() { return unitLabel(this.unitOpts()); },
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
    // X/Y are prefilled from the current view centre so "Go to" defaults to a
    // no-op, and X is focused AND select()ed for immediate overtyping.  That
    // selection is why the opening shortcut has to preventDefault: its own
    // keystroke would otherwise replace the whole prefilled value.
    it('prefills X/Y from the view centre in microns, focused and selected',
       () => {
        document.body.innerHTML = '';
        const { app } = makeApp({
            map: {
                getZoom() { return 3; },
                // scale=1, maxDXDY=0, origins=0, so lat/lng are the dbu
                // themselves: 12000 x 34000 dbu = 12 x 34 µm.
                getCenter() { return { lat: 34000, lng: 12000 }; },
                setView() {}, fitBounds() {},
            },
        });
        showGotoDialog(app);
        const xIn = document.querySelector('.sn-x');
        const yIn = document.querySelector('.sn-y');
        assert.equal(xIn.value, '12.000');
        assert.equal(yIn.value, '34.000');
        // Size has no sensible default; it stays empty (placeholder only).
        assert.equal(document.querySelector('.sn-size').value, '');
        assert.equal(document.activeElement, xIn, 'X should be focused');
        assert.equal(xIn.selectionStart, 0);
        assert.equal(xIn.selectionEnd, xIn.value.length,
                     'X should be fully selected for overtyping');
    });

    it('follows showDbu: labels, prefill and input are raw DBU', () => {
        document.body.innerHTML = '';
        const { app, calls } = makeApp({
            showDbu: true,
            map: {
                getZoom() { return 3; },
                getCenter() { return { lat: 34000, lng: 12000 }; },
                setView(latlng, z) { calls.push(['setView', latlng, z]); },
                fitBounds(b) { calls.push(['fitBounds', b]); },
            },
        });
        showGotoDialog(app);
        const labels = [...document.querySelectorAll('.sn-row label')]
            .map((l) => l.textContent);
        assert.deepEqual(labels, ['X (DBU)', 'Y (DBU)', 'Size (DBU)']);
        // Prefill is the centre in DBU, not divided down to microns.
        assert.equal(document.querySelector('.sn-x').value, '12000');
        assert.equal(document.querySelector('.sn-y').value, '34000');
        // And what is typed is read back as DBU.
        setInput('.sn-x', '5000');
        setInput('.sn-y', '7000');
        clickOk();
        assert.deepEqual(calls, [['setView', dbuToLatLng(5000, 7000, 1, 0, 0, 0), 3]]);
    });

    it('follows showDbu for the Size window too', () => {
        document.body.innerHTML = '';
        const { app, calls } = makeApp({ showDbu: true });
        showGotoDialog(app);
        setInput('.sn-x', '0');
        setInput('.sn-y', '0');
        setInput('.sn-size', '2000');   // 2000 dbu, not 2000 µm
        clickOk();
        const expected = dbuRectToBounds(-1000, -1000, 1000, 1000, 1, 0, 0, 0);
        assert.deepEqual(calls, [['fitBounds', expected]]);
    });

    it('names the active unit when rejecting a non-number', () => {
        document.body.innerHTML = '';
        const { app } = makeApp({ showDbu: true });
        showGotoDialog(app);
        setInput('.sn-x', 'abc');
        clickOk();
        assert.match(errorText(), /\(DBU\)/);
    });

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
