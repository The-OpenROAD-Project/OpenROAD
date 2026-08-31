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

// Minimal Leaflet stand-in so the layer class itself can be built, not just the
// pure helpers. Without this, a name the layer body references but never binds
// (an `export ... from` re-export with no local import, say) goes unnoticed:
// importers resolve it, the module body does not, and it only fails at runtime
// inside Leaflet's _setView.
globalThis.L = {
    GridLayer: {
        prototype: {
            initialize(opts) { this.options = { ...(opts || {}) }; },
            _clampZoom(z) { return z; },
            _removeTile() {},
        },
        extend(proto) {
            const Base = globalThis.L.GridLayer.prototype;
            function Layer(...args) { this._tiles = {}; this.initialize(...args); }
            Layer.prototype = Object.create(Base);
            Object.assign(Layer.prototype, proto);
            return Layer;
        },
    },
};

// The tile-layer module references the Leaflet global `L` only inside
// createWebSocketTileLayer's body, so importing it (for the pure helpers)
// does not require Leaflet.
const { buildMapOptions } = await import('../../src/ui-utils.js');
const { floorClampZoom, buildTileRequest, currentDpr,
        createWebSocketTileLayer, createOverlayTileLayer }
    = await import('../../src/websocket-tile-layer.js');
const { TILE_SIZE_CSS, buildTileRequestFor, watchDevicePixelRatio }
    = await import('../../src/tile-request.js');
const { WebSocketManager } = await import('../../src/websocket-manager.js');

// The seam the tile size exists to prevent: a tile whose CSS size is not a whole
// number of device pixels has a fractional PITCH, so its boundaries cannot all
// sit on the device grid however the panes are placed, and the browser
// antialiases the rest into their neighbours.
describe('TILE_SIZE_CSS', () => {
    // Every ratio seen in the wild: display scaling (125/150/166/175/200/250%),
    // and those combined with the browser zoom steps that produce eighths and
    // tenths.
    const RATIOS = [1, 1.1, 1.2, 1.25, 1.3333333333333333, 1.375, 1.5,
                    1.6666666269302368, 1.75, 1.8333333333333333, 2, 2.5, 3];

    // Tolerance, not equality: a browser reports 5/3 as the float32
    // 1.6666666269302368, so no integer CSS size is bit-exactly whole in device
    // px. The browser quantizes layout to 1/64 device px anyway, so landing
    // inside that is as exact as the platform can represent.
    const kLayoutUnit = 1 / 64;

    it('is a whole number of device pixels at every real ratio', () => {
        for (const dpr of RATIOS) {
            const device = TILE_SIZE_CSS * dpr;
            assert.ok(Math.abs(device - Math.round(device)) < kLayoutUnit,
                      `${TILE_SIZE_CSS} CSS px at dpr ${dpr} is ${device}`);
        }
    });

    it('is why 256 could not stay', () => {
        // 256 * 5/3 = 426.67, a third of a pixel off: the pitch itself is
        // fractional, so only every third boundary can land on the grid however
        // the panes are placed. That is the reported bug.
        const device = 256 * 1.6666666269302368;
        assert.ok(Math.abs(device - Math.round(device)) > 0.3,
                  `256 CSS px at 5/3 is ${device}`);
    });

    it('keeps the decoded pixel count within a few percent of 256', () => {
        // Tiles get smaller and more numerous; total bytes are what the memory
        // budget cares about, and they must not grow.
        const ratio = (256 / TILE_SIZE_CSS) ** 2;  // tiles per old tile
        const bytes = ratio * TILE_SIZE_CSS ** 2 / 256 ** 2;
        assert.ok(Math.abs(bytes - 1) < 0.01, `decoded bytes x${bytes}`);
    });
});

describe('tile_px on the wire', () => {
    it('is the exact device square the tile is displayed in', () => {
        const req = buildTileRequestFor({ x: 1, y: 2, z: 3 }, 'metal1',
                                        { visibility: {} }, 1.6666666269302368,
                                        TILE_SIZE_CSS);
        assert.equal(req.tile_px, 400);  // 240 * 5/3, exactly
        assert.equal(Number.isInteger(req.tile_px), true);
    });

    it('follows the tile size it is given, not a fixed 256', () => {
        const req = buildTileRequestFor({ x: 0, y: 0, z: 0 }, 'metal1',
                                        { visibility: {} }, 2, 240);
        assert.equal(req.tile_px, 480);
    });
});

describe('watchDevicePixelRatio', () => {
    // Without this, tiles rasterized at the old ratio stay on screen stretched
    // into the new box — a tile fetched at 1.25 and shown at 1.6667 is a 33%
    // upscale, which smears every tile edge into its neighbour.
    function fakeWindow(dpr) {
        const queries = [];
        return {
            devicePixelRatio: dpr,
            queries,
            matchMedia(query) {
                const mql = { query, handlers: [], removed: [] };
                mql.addEventListener = (name, fn) => {
                    assert.equal(name, 'change');
                    mql.handlers.push(fn);
                };
                mql.removeEventListener = (name, fn) => {
                    assert.equal(name, 'change');
                    mql.removed.push(fn);
                };
                queries.push(mql);
                return mql;
            },
        };
    }

    it('watches the ratio in force', () => {
        const saved = globalThis.window;
        globalThis.window = fakeWindow(1.25);
        try {
            watchDevicePixelRatio(() => {});
            assert.equal(globalThis.window.queries.length, 1);
            assert.ok(globalThis.window.queries[0].query.includes('1.25dppx'));
        } finally {
            globalThis.window = saved;
        }
    });

    it('reports the new ratio and watches for the next one', () => {
        const saved = globalThis.window;
        const win = fakeWindow(1.25);
        globalThis.window = win;
        try {
            const seen = [];
            watchDevicePixelRatio((dpr) => seen.push(dpr));
            win.devicePixelRatio = 1.6666666269302368;
            win.queries[0].handlers[0]();
            assert.deepEqual(seen, [1.6666666269302368]);
            // A new query: the old one only ever matches the old ratio, so
            // without this a second change would go unnoticed.
            assert.equal(win.queries.length, 2);
            assert.ok(win.queries[1].query.includes('1.6666666269302368dppx'));
        } finally {
            globalThis.window = saved;
        }
    });

    it('stops reporting once unsubscribed', () => {
        const saved = globalThis.window;
        const win = fakeWindow(1.25);
        globalThis.window = win;
        try {
            const seen = [];
            const stop = watchDevicePixelRatio((dpr) => seen.push(dpr));
            stop();
            win.queries[0].handlers[0]();
            assert.deepEqual(seen, []);
        } finally {
            globalThis.window = saved;
        }
    });

    it('detaches the pending listener on unsubscribe', () => {
        // `once` retires a handler that has fired; the one still waiting on the
        // current ratio has not, and would outlive a viewer that is torn down.
        const saved = globalThis.window;
        const win = fakeWindow(1.25);
        globalThis.window = win;
        try {
            const stop = watchDevicePixelRatio(() => {});
            assert.equal(win.queries[0].removed.length, 0);
            stop();
            assert.deepEqual(win.queries[0].removed,
                             [win.queries[0].handlers[0]]);
        } finally {
            globalThis.window = saved;
        }
    });

    it('is a no-op where matchMedia is unavailable', () => {
        const saved = globalThis.window;
        globalThis.window = { devicePixelRatio: 2 };
        try {
            assert.equal(typeof watchDevicePixelRatio(() => {}), 'function');
        } finally {
            globalThis.window = saved;
        }
    });
});

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

    // Options-menu preferences (2.15).  With none supplied the map must keep
    // behaving as it always has: wheel zooms, arrows pan by Leaflet's 80 px.
    it('defaults to wheel-zoom on and the default arrow step', () => {
        const opts = buildMapOptions(null);
        assert.equal(opts.scrollWheelZoom, true);
        assert.equal(opts.keyboardPanDelta, 80);
    });

    it('carries the wheel-zoom preference through', () => {
        assert.equal(buildMapOptions(null, { wheelZoom: false })
                         .scrollWheelZoom, false);
        assert.equal(buildMapOptions(null, { wheelZoom: true })
                         .scrollWheelZoom, true);
    });

    it('clamps the arrow step it is given', () => {
        assert.equal(buildMapOptions(null, { arrowStep: 250 })
                         .keyboardPanDelta, 250);
        assert.equal(buildMapOptions(null, { arrowStep: 1 })
                         .keyboardPanDelta, 10);
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

    it('keeps the ratio exact, because sizes are computed from it', () => {
        // NOT rounded: the request's pixel count comes from this, and rounding
        // first asks for 401 px to fill a 400 px box (see test-dpr.js). The
        // two-decimal rounding applies to the payload's `dpr` field alone.
        assert.equal(withDpr(1.75, currentDpr), 1.75);
        assert.equal(withDpr(4 / 3, currentDpr), 4 / 3);
        assert.equal(withDpr(1.6666666269302368, currentDpr),
                     1.6666666269302368);
    });

    it('still clamps, because the clamp bounds tile memory', () => {
        // A tile is rendered at (tileSize*dpr*supersample)^2 bytes.
        assert.equal(withDpr(8, currentDpr), 3);
        assert.equal(withDpr(0.5, currentDpr), 1);
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

describe('the layer body must resolve every name it references', () => {
    // Regression test for a real breakage: floorClampZoom was re-exported with
    // `export { floorClampZoom } from './tile-request.js'`, which does NOT
    // create a local binding. Importers still resolved the name, and the
    // existing tests all imported it directly, so everything looked fine — but
    // _clampZoom referenced an undefined name and the map failed to load with
    // "ReferenceError: floorClampZoom is not defined" from inside GridLayer's
    // _setView. Building the class and calling through it is what catches this.
    function build() {
        const Layer = createWebSocketTileLayer(
            { stdcells: true }, new Set(['metal1']), null, null, null);
        return new Layer({ nextId: 1, request: () => new Promise(() => {}),
                           cancel() {} }, 'metal1', {});
    }

    it('resolves _clampZoom through the real layer', () => {
        const layer = build();
        layer._map = { getZoom: () => 3.7 };
        assert.doesNotThrow(() => layer._clampZoom(4));
        assert.equal(layer._clampZoom(4), 3);
    });

    it('falls back to the passed zoom with no map', () => {
        const layer = build();
        layer._map = null;
        assert.equal(layer._clampZoom(5), 5);
    });

    it('builds the overlay layer class too', () => {
        assert.doesNotThrow(() => createOverlayTileLayer({}, null));
    });

    // createTile builds an <img>; this file runs without jsdom, so stand one up
    // for the duration. Only what the layer touches needs to exist.
    function withFakeDocument(fn) {
        const saved = globalThis.document;
        globalThis.document = {
            createElement: () => ({ setAttribute() {} }),
        };
        try {
            return fn();
        } finally {
            globalThis.document = saved;
        }
    }

    // Build an overlay layer whose requests are captured, with `tileSize` as
    // Leaflet would report it.
    function overlayRequests(tileSize) {
        const sent = [];
        const OverlayLayer = createOverlayTileLayer({}, null);
        const layer = new OverlayLayer(
            { nextId: 1,
              request: (msg) => { sent.push(msg); return new Promise(() => {}); },
              cancel() {} },
            {});
        // Leaflet derives this from the tileSize option; stub it so createTile
        // can run without a map.
        layer.getTileSize = () => ({ x: tileSize, y: tileSize });
        withFakeDocument(() => layer.createTile({ x: 1, y: 2, z: 3 }, () => {}));
        return sent;
    }

    // The wiring, not the helper: tileSizeFields can be perfect and still not be
    // called. An overlay sized differently from the layer tiles under it is
    // rescaled by the browser and slides off the shapes it annotates.
    it('sends the sizing fields on an overlay request', () => {
        const sent = overlayRequests(240);
        assert.equal(sent.length, 1);
        assert.equal(sent[0].type, 'overlay_tile');
        assert.equal(sent[0].tile_px, Math.round(240 * currentDpr()));
        assert.ok(sent[0].dpr > 0);
    });

    it('sizes an overlay request from its own tile size', () => {
        // A static report's layers are 256; the overlay must follow them.
        const sent = overlayRequests(256);
        assert.equal(sent[0].tile_px, Math.round(256 * currentDpr()));
    });
});
