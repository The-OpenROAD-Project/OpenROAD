// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it, beforeEach, afterEach } from 'node:test';
import assert from 'node:assert/strict';
import { captureLayout } from '../../src/capture.js';

// capture.js reads `layer instanceof L.GridLayer` / `L.ImageOverlay`; provide a
// stub L whose classes nothing matches (the mock map has no real layers).
globalThis.L = { GridLayer: class {}, ImageOverlay: class {} };

// Stub <canvas> so renderToBlob runs headless (jsdom has no 2d context).
// `ops`, when given, records the drawing calls so a test can assert what the
// capture painted.
function withCanvasStub(fn, ops = []) {
    const orig = document.createElement.bind(document);
    document.createElement = (tag) => {
        if (tag === 'canvas') {
            return {
                width: 0,
                height: 0,
                getContext: () => ({
                    fillRect(...a) { ops.push(['fillRect', ...a]); },
                    fillText(...a) { ops.push(['fillText', ...a]); },
                    drawImage() {},
                    beginPath() {}, roundRect() {}, fill() {},
                    save() {}, restore() {},
                    set fillStyle(v) { ops.push(['fillStyle', v]); },
                    set globalAlpha(_) {},
                    set font(_) {}, set textAlign(_) {}, set textBaseline(_) {},
                }),
                toBlob: (cb) => cb(null),
            };
        }
        return orig(tag);
    };
    return Promise.resolve(fn()).finally(() => { document.createElement = orig; });
}

// jsdom gives every element a zero rect; capture.js skips zero-sized nodes, so
// a test that wants an element drawn has to supply a real one.
function stubRect(el, x, y, width, height) {
    el.getBoundingClientRect = () => ({
        x, y, width, height,
        left: x, top: y, right: x + width, bottom: y + height,
    });
    return el;
}

function makeApp() {
    const container = document.createElement('div');
    document.body.appendChild(container);
    const calls = [];
    return {
        calls,
        map: {
            getCenter: () => ({ lat: 1, lng: 2 }),
            getZoom: () => 5,
            getContainer: () => container,
            eachLayer() {},  // no layers → tilesPending stays 0
            fitBounds(b, opts) { calls.push(['fitBounds', b, opts]); },
            setView(c, z, opts) { calls.push(['setView', c, z, opts]); },
        },
        fitBounds: [[0, 0], [10, 10]],
    };
}

describe('captureLayout entire → fit whole design then restore', () => {
    beforeEach(() => { document.body.innerHTML = ''; });
    afterEach(() => { document.body.innerHTML = ''; });

    it('fits the design without animation and restores the prior view', async () => {
        const app = makeApp();
        await withCanvasStub(() => captureLayout(app, { entire: true }));

        const fit = app.calls.find((c) => c[0] === 'fitBounds');
        assert.ok(fit, 'fitBounds was called');
        assert.equal(fit[1], app.fitBounds, 'fits the whole-design bounds');
        assert.equal(fit[2].animate, false, 'no animation (avoids mid-fit capture)');

        const restore = app.calls.find((c) => c[0] === 'setView');
        assert.ok(restore, 'view is restored after capture');
        assert.deepEqual(restore[1], { lat: 1, lng: 2 });
        assert.equal(restore[2], 5);
    });

    it('visible scope neither fits nor moves the view', async () => {
        const app = makeApp();
        await withCanvasStub(() => captureLayout(app, { entire: false }));
        assert.equal(app.calls.length, 0, 'no fitBounds/setView for the visible capture');
    });
});

// Ruler distance labels are L.divIcon <div>s in a marker pane, not SVG, so the
// SVG-pane pass cannot see them.  Before this was handled the capture kept a
// ruler's line and ticks but dropped the measurement it exists to show.
describe('captureLayout includes the HTML ruler labels', () => {
    beforeEach(() => { document.body.innerHTML = ''; });
    afterEach(() => { document.body.innerHTML = ''; });

    function appWithLabel(text) {
        const app = makeApp();
        const container = app.map.getContainer();
        stubRect(container, 0, 0, 400, 300);
        const pane = document.createElement('div');
        pane.className = 'leaflet-pane';
        container.appendChild(pane);
        const label = document.createElement('div');
        label.className = 'ruler-label';
        label.textContent = text;
        // 30px right and 40px down from the map's top-left corner.
        stubRect(label, 30, 40, 70, 16);
        pane.appendChild(label);
        return app;
    }

    it('paints the label text at its on-screen position', async () => {
        const ops = [];
        await withCanvasStub(
            () => captureLayout(appWithLabel('12.500 um'), { entire: false }),
            ops);

        const text = ops.filter((o) => o[0] === 'fillText');
        assert.equal(text.length, 1, 'the label text is drawn exactly once');
        assert.equal(text[0][1], '12.500 um');
        // x = label.left - map.left + paddingLeft(0 in jsdom); y = vertical centre.
        assert.equal(text[0][2], 30);
        assert.equal(text[0][3], 48);

        // And a box behind it, so the text stays readable over the layout.
        // The first fillRect is the map background; the label's is the second.
        const rects = ops.filter((o) => o[0] === 'fillRect');
        assert.equal(rects.length, 2, 'background + one label box');
        assert.deepEqual(rects[1].slice(1), [30, 40, 70, 16]);
    });

    it('skips a label with no text rather than painting an empty box', async () => {
        const ops = [];
        await withCanvasStub(
            () => captureLayout(appWithLabel(''), { entire: false }), ops);
        assert.equal(ops.filter((o) => o[0] === 'fillText').length, 0);
        assert.equal(ops.filter((o) => o[0] === 'fillRect').length, 1,
                     'only the map background');
    });
});
