// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';

// Mock WebSocket before importing the manager (needed for the cancel test).
class MockWebSocket {
    constructor() {
        this.readyState = 1;
        this.sent = [];
        this.binaryType = null;
        queueMicrotask(() => { if (this.onopen) this.onopen(); });
    }
    send(data) { this.sent.push(data); }
    close() {}
    static get OPEN() { return 1; }
}
globalThis.WebSocket = MockWebSocket;

// The tile-layer module references the Leaflet global `L` only inside
// createWebSocketTileLayer's body, so importing it (for the pure helpers)
// does not require Leaflet.
const { buildMapOptions } = await import('../../src/ui-utils.js');
const { floorClampZoom, buildTileRequest, currentDpr }
    = await import('../../src/websocket-tile-layer.js');
const { WebSocketManager } = await import('../../src/websocket-manager.js');

describe('buildMapOptions', () => {
    it('rests on integer zoom (zoomSnap/zoomDelta = 1)', () => {
        // Integer-zoom rest => tiles displayed 1:1 with no fractional CSS
        // rescale of the pane, which would re-introduce the moiré beat.
        const opts = buildMapOptions(null);
        assert.equal(opts.zoomSnap, 1);
        assert.equal(opts.zoomDelta, 1);
        assert.equal(opts.fadeAnimation, false);
        assert.equal(opts.attributionControl, false);
    });
});

describe('floorClampZoom (upscale-only invariant)', () => {
    // The tile pane displays at scale 2^(realZoom - nativeZoom).  Flooring the
    // real zoom keeps that exponent in [0,1) => scale in [1,2) => the pane is
    // only ever upscaled, never downscaled.  Upscaling a band-limited tile
    // cannot reintroduce the moiré beat; downscaling can.
    for (const real of [0.0, 3.0, 3.4, 3.9, 7.99]) {
        it(`floors real zoom ${real} so display scale stays in [1,2)`, () => {
            const layer = { _map: { getZoom: () => real } };
            const native = floorClampZoom(layer, Math.round(real));
            assert.equal(native, Math.floor(real));
            const displayScale = Math.pow(2, real - native);
            assert.ok(displayScale >= 1 && displayScale < 2,
                `display scale ${displayScale} must be in [1,2)`);
        });
    }

    it('falls back to the passed zoom when detached from a map', () => {
        assert.equal(floorClampZoom({ _map: null }, 5), 5);
    });
});

describe('currentDpr', () => {
    function withDpr(value, fn) {
        const saved = globalThis.window;
        globalThis.window = value === undefined ? {} : { devicePixelRatio: value };
        try { return fn(); } finally { globalThis.window = saved; }
    }

    it('returns the device pixel ratio', () => {
        assert.equal(withDpr(2, currentDpr), 2);
    });
    it('clamps above 3', () => {
        assert.equal(withDpr(8, currentDpr), 3);
    });
    it('defaults to 1 when devicePixelRatio is absent', () => {
        assert.equal(withDpr(undefined, currentDpr), 1);
    });
});

describe('buildTileRequest', () => {
    const ctx = {
        visibility: { stdcells: true, phys_bump: false },
        selectability: { stdcells: true },
        visibleLayers: new Set(['M1', 'M2']),
        selectableLayers: null,
        app: null,
    };

    it('carries z/x/y, layer, visibility flags and the HiDPI dpr', () => {
        const saved = globalThis.window;
        globalThis.window = { devicePixelRatio: 2 };
        try {
            const req = buildTileRequest({ z: 3, x: 1, y: 2 }, 'M1', ctx);
            assert.equal(req.type, 'tile');
            assert.equal(req.layer, 'M1');
            assert.equal(req.z, 3);
            assert.equal(req.x, 1);
            assert.equal(req.y, 2);
            assert.equal(req.dpr, 2);
            assert.deepEqual(req.visible_layers, ['M1', 'M2']);
            assert.equal(req.stdcells, true);
            assert.equal(req.phys_bump, false);
            assert.equal(req.s_stdcells, true);
        } finally {
            globalThis.window = saved;
        }
    });

    it('omits pattern when the layer is solid or unset', () => {
        // No app → no pattern field.
        assert.equal('pattern' in buildTileRequest(
            { z: 0, x: 0, y: 0 }, 'M1', ctx), false);

        // app.layerPatterns present but M1 solid (default) → still omitted.
        const solidCtx = { ...ctx, app: { layerPatterns: { M2: 2 } } };
        assert.equal('pattern' in buildTileRequest(
            { z: 0, x: 0, y: 0 }, 'M1', solidCtx), false);
    });

    it('carries the per-layer fill pattern when non-solid', () => {
        const patCtx = { ...ctx, app: { layerPatterns: { M1: 3 } } };
        const req = buildTileRequest({ z: 0, x: 0, y: 0 }, 'M1', patCtx);
        assert.equal(req.pattern, 3);
    });
});

describe('WebSocketManager.cancel', () => {
    it('notifies the server and drops the pending entry', async () => {
        const mgr = new WebSocketManager('ws://fake');
        await mgr.readyPromise;
        mgr.pending.set(5, { resolve: () => {}, reject: () => {} });
        mgr.socket.sent = [];
        mgr.cancel(5);
        assert.equal(mgr.pending.has(5), false);
        const cancel = mgr.socket.sent
            .map((s) => JSON.parse(s))
            .find((m) => m.type === 'cancel');
        assert.ok(cancel, 'a cancel message must be sent to the server');
        assert.equal(cancel.cancel_id, 5);
        // The fire-and-forget cancel must not itself be tracked as pending.
        assert.equal(mgr.pending.has(cancel.id), false);
    });

    it('sends nothing when there was no pending request to cancel', async () => {
        const mgr = new WebSocketManager('ws://fake');
        await mgr.readyPromise;
        mgr.socket.sent = [];
        mgr.cancel(9999);
        assert.equal(mgr.socket.sent.length, 0);
    });

    it('is a no-op in static (cache) mode', () => {
        const mgr = WebSocketManager.fromCache({
            zoom: 1, json: {}, tiles: {}, overlays: {},
        });
        assert.doesNotThrow(() => mgr.cancel(1));
    });
});
