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
function withCanvasStub(fn) {
    const orig = document.createElement.bind(document);
    document.createElement = (tag) => {
        if (tag === 'canvas') {
            return {
                width: 0,
                height: 0,
                getContext: () => ({
                    fillRect() {}, drawImage() {},
                    set fillStyle(_) {}, set globalAlpha(_) {},
                }),
                toBlob: (cb) => cb(null),
            };
        }
        return orig(tag);
    };
    return Promise.resolve(fn()).finally(() => { document.createElement = orig; });
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
