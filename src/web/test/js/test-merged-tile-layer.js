// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it } from 'node:test';
import assert from 'node:assert/strict';

// Minimal Leaflet stand-in. The layer only uses GridLayer.extend, the
// prototype's initialize/_removeTile/_clampZoom, and getTileSize/_tiles/_map,
// so a real Leaflet is not needed to exercise the behaviour that matters.
const removedKeys = [];
globalThis.L = {
    GridLayer: {
        prototype: {
            initialize(opts) { this.options = { ...(this.options || {}),
                                                ...(opts || {}) }; },
            _clampZoom(z) { return z; },
            _removeTile(key) { removedKeys.push(key); delete this._tiles[key]; },
        },
        extend(proto) {
            const Base = globalThis.L.GridLayer.prototype;
            function Layer(...args) {
                this._tiles = {};
                this.initialize(...args);
            }
            Layer.prototype = Object.create(Base);
            Object.assign(Layer.prototype, proto);
            Layer.prototype.getTileSize = function() {
                return { x: 256, y: 256 };
            };
            Layer.prototype.addTo = function(map) {
                this._map = map;
                (map._layers = map._layers || []).push(this);
                return this;
            };
            return Layer;
        },
    },
};

const { createMergedTileLayer, buildMergedPanes, decodeTilePayload }
    = await import('../../src/merged-tile-layer.js');
const { partitionIntoGroups, MERGED_PANE_OPACITY }
    = await import('../../src/tile-merge.js');

// Fake websocket manager: records requests, resolves with a marker payload,
// and records cancels so cancellation can be asserted.
function fakeManager({ hang = false } = {}) {
    const mgr = {
        _next: 1,
        sent: [],
        cancelled: [],
        get nextId() { return this._next++; },
        request(msg) {
            mgr.sent.push(msg);
            if (hang) {
                return new Promise(() => {});
            }
            return Promise.resolve({ payloadFor: msg.layer });
        },
        cancel(id) { mgr.cancelled.push(id); },
    };
    return mgr;
}

// Fake manager that reproduces the real cancellation contract: a request stays
// in flight until it is answered or cancelled, and cancel() REJECTS it (see
// WebSocketManager.cancel).  fakeManager's cancel only records, which cannot
// exercise what a render does after its requests are cancelled.
//
// `nextId` does not increment on read, matching the real manager: it is a plain
// field there, consumed by request().
function cancellableManager() {
    const inFlight = new Map();  // id -> { layer, resolve, reject }
    const mgr = {
        _next: 1,
        sent: [],
        cancelled: [],
        get nextId() { return this._next; },
        request(msg) {
            const id = mgr._next++;
            mgr.sent.push(msg);
            return new Promise((resolve, reject) => {
                inFlight.set(id, { layer: msg.layer, resolve, reject });
            });
        },
        // Answer every in-flight request for one layer.
        reply(layer) {
            for (const [id, entry] of [...inFlight]) {
                if (entry.layer === layer) {
                    inFlight.delete(id);
                    entry.resolve({ payloadFor: layer });
                }
            }
        },
        cancel(id) {
            mgr.cancelled.push(id);
            const entry = inFlight.get(id);
            if (entry) {
                inFlight.delete(id);
                entry.reject(new Error('Request cancelled'));
            }
        },
    };
    return mgr;
}

// Layer whose decodes are recorded, so releasing them can be asserted.
function trackingLayer(mgr, items) {
    const decoded = [];
    const Layer = createMergedTileLayer(CTX, {
        decode: async (payload) => {
            if (!payload) {
                return null;
            }
            const image = { id: payload.payloadFor, closed: false,
                            close() { this.closed = true; } };
            decoded.push(image);
            return image;
        },
        dpr: () => 1,
    });
    const layer = new Layer(mgr, items, { zIndex: 0 });
    return { layer, decoded };
}

// Capture what got drawn, with the alpha in force at draw time.
//
// Rendering is incremental: the canvas is recomposited from scratch on every
// arrival, so there are several paints per tile.  clearRect delimits them, and
// `paints` is one entry per paint.  `last()` is the finished composite, which
// is what most assertions care about.
function stubCanvas2d() {
    const paints = [];
    let current = null;
    const origGetContext
        = globalThis.HTMLCanvasElement.prototype.getContext;
    globalThis.HTMLCanvasElement.prototype.getContext = function() {
        const ctx = {
            _alpha: 1,
            get globalAlpha() { return this._alpha; },
            set globalAlpha(v) { this._alpha = v; },
            cleared: 0,
            clearRect() {
                this.cleared++;
                current = [];
                paints.push(current);
            },
            drawImage(img) {
                if (!current) {
                    current = [];
                    paints.push(current);
                }
                current.push({ id: img.id, alpha: this._alpha });
            },
        };
        this._orCtx = ctx;
        return ctx;
    };
    return {
        paints,
        last: () => (paints.length ? paints[paints.length - 1] : []),
        get drawn() { return paints.length ? paints[paints.length - 1] : []; },
        restore() {
            globalThis.HTMLCanvasElement.prototype.getContext = origGetContext;
        },
    };
}

const CTX = {
    visibility: { stdcells: true },
    selectability: null,
    visibleLayers: new Set(['metal1', 'metal2']),
    selectableLayers: null,
    app: null,
};

function makeLayer(mgr, items, opts = {}) {
    const Layer = createMergedTileLayer(CTX, {
        // Deterministic decode: no real ImageBitmap in jsdom.
        decode: async (payload) => (payload
            ? { id: payload.payloadFor, closed: false,
                close() { this.closed = true; } }
            : null),
        dpr: () => opts.dpr || 1,
    });
    const layer = new Layer(mgr, items, { zIndex: 0,
                                          opacity: MERGED_PANE_OPACITY });
    return layer;
}

const ITEMS = [
    { layer: 'metal1', opacity: 0.7, visible: true },
    { layer: 'metal2', opacity: 0.7, visible: true },
    { layer: '_instances', opacity: 1, visible: true },
];

describe('createMergedTileLayer: one canvas per tile, K layers inside it', () => {
    it('requests every visible item for one tile position', async () => {
        const mgr = fakeManager();
        const stub = stubCanvas2d();
        try {
            const layer = makeLayer(mgr, ITEMS);
            layer._map = {};
            await new Promise((resolve) => {
                layer.createTile({ x: 1, y: 2, z: 3 }, () => resolve());
            });
            assert.deepEqual(mgr.sent.map(m => m.layer),
                             ['metal1', 'metal2', '_instances']);
            // One tile position, one request each — not one pane each.
            for (const m of mgr.sent) {
                assert.equal(m.type, 'tile');
                assert.equal(m.x, 1);
                assert.equal(m.y, 2);
                assert.equal(m.z, 3);
            }
        } finally {
            stub.restore();
        }
    });

    it('draws in z-order at each item opacity, then releases the decodes', async () => {
        const mgr = fakeManager();
        const stub = stubCanvas2d();
        try {
            const layer = makeLayer(mgr, ITEMS);
            layer._map = {};
            await new Promise((resolve) => {
                layer.createTile({ x: 0, y: 0, z: 0 }, () => resolve());
            });
            // The finished composite: every layer, in z-order, at its own
            // opacity.  Earlier paints are subsets of this.
            assert.deepEqual(stub.last(), [
                { id: 'metal1', alpha: 0.7 },
                { id: 'metal2', alpha: 0.7 },
                { id: '_instances', alpha: 1 },
            ]);
        } finally {
            stub.restore();
        }
    });

    it('sizes the backing store in device pixels and the box in CSS pixels', () => {
        // The 1:1 device-pixel mapping is what stops drawImage resampling and
        // reintroducing the moiré beat the <img> path was tuned to avoid.
        const mgr = fakeManager({ hang: true });
        const stub = stubCanvas2d();
        try {
            const layer = makeLayer(mgr, ITEMS, { dpr: 2 });
            layer._map = {};
            const canvas = layer.createTile({ x: 0, y: 0, z: 0 }, () => {});
            assert.equal(canvas.width, 512);
            assert.equal(canvas.height, 512);
            assert.equal(canvas.style.width, '256px');
            assert.equal(canvas.style.height, '256px');
        } finally {
            stub.restore();
        }
    });

    it('sends the forced dpr on the wire', () => {
        const mgr = fakeManager({ hang: true });
        const stub = stubCanvas2d();
        try {
            const layer = makeLayer(mgr, ITEMS, { dpr: 2 });
            layer._map = {};
            layer.createTile({ x: 0, y: 0, z: 0 }, () => {});
            assert.ok(mgr.sent.every(m => m.dpr === 2));
        } finally {
            stub.restore();
        }
    });

    it('skips items that are toggled off, without touching the pane', async () => {
        // The point of the design: a visibility toggle flips a flag and
        // refreshes; no layer is added to or removed from the map, so the pane
        // count — and therefore the memory ceiling — never moves.
        const mgr = fakeManager();
        const stub = stubCanvas2d();
        try {
            const items = [
                { layer: 'metal1', opacity: 0.7, visible: true },
                { layer: 'metal2', opacity: 0.7, visible: false },
                { layer: '_instances', opacity: 1, visible: true },
            ];
            const layer = makeLayer(mgr, items);
            layer._map = {};
            await new Promise((resolve) => {
                layer.createTile({ x: 0, y: 0, z: 0 }, () => resolve());
            });
            assert.deepEqual(mgr.sent.map(m => m.layer),
                             ['metal1', '_instances']);
            assert.deepEqual(stub.last().map(d => d.id),
                             ['metal1', '_instances']);
        } finally {
            stub.restore();
        }
    });

    it('treats a missing visible flag as visible', async () => {
        const mgr = fakeManager();
        const stub = stubCanvas2d();
        try {
            const layer = makeLayer(mgr, [{ layer: 'metal1', opacity: 1 }]);
            layer._map = {};
            await new Promise((resolve) => {
                layer.createTile({ x: 0, y: 0, z: 0 }, () => resolve());
            });
            assert.deepEqual(mgr.sent.map(m => m.layer), ['metal1']);
        } finally {
            stub.restore();
        }
    });
});

describe('refreshTiles: the contract redrawAllLayers depends on', () => {
    it('re-requests every held tile in place', async () => {
        // redrawAllLayers() iterates app.allLayers and calls refreshTiles() —
        // that is how a server design-change refresh, a visibility change and a
        // pattern change all reach the tiles. A merged pane must honour the
        // same contract or those paths silently stop updating.
        const mgr = fakeManager();
        const stub = stubCanvas2d();
        try {
            const layer = makeLayer(mgr, ITEMS);
            layer._map = {};
            const el = layer.createTile({ x: 0, y: 0, z: 0 }, () => {});
            layer._tiles = { '0:0:0': { el, coords: { x: 0, y: 0, z: 0 } } };
            await new Promise(r => setTimeout(r, 0));

            mgr.sent.length = 0;
            layer.refreshTiles();
            await new Promise(r => setTimeout(r, 0));
            assert.deepEqual(mgr.sent.map(m => m.layer),
                             ['metal1', 'metal2', '_instances']);
        } finally {
            stub.restore();
        }
    });

    it('does nothing when the pane is not on a map', () => {
        const mgr = fakeManager();
        const layer = makeLayer(mgr, ITEMS);
        layer._map = null;
        assert.doesNotThrow(() => layer.refreshTiles());
        assert.equal(mgr.sent.length, 0);
    });

    it('cancels the superseded requests instead of leaving them in flight', async () => {
        const mgr = fakeManager({ hang: true });
        const stub = stubCanvas2d();
        try {
            const layer = makeLayer(mgr, ITEMS);
            layer._map = {};
            const el = layer.createTile({ x: 0, y: 0, z: 0 }, () => {});
            layer._tiles = { '0:0:0': { el, coords: { x: 0, y: 0, z: 0 } } };
            const firstIds = [...el._orRequestIds];
            assert.equal(firstIds.length, 3);

            layer.refreshTiles();
            assert.deepEqual(mgr.cancelled, firstIds);
        } finally {
            stub.restore();
        }
    });

    it('reveals a tile refreshed before its first paint, and not before', async () => {
        // done() belongs to the TILE, not to the render that happened to create
        // it: a refresh replaces the render but not the tile.  Leaflet keeps a
        // tile hidden (.leaflet-tile { visibility: hidden }) until done() marks
        // it loaded, so the handshake has to survive being superseded — and it
        // has to be performed by a render that actually PAINTED.  A superseded
        // render reporting done on its way out reveals a canvas still holding
        // its pre-refresh content, which for a new tile is nothing at all.
        const mgr = cancellableManager();
        const stub = stubCanvas2d();
        try {
            const { layer } = trackingLayer(mgr, ITEMS);
            layer._map = {};
            let doneCalls = 0;
            const coords = { x: 0, y: 0, z: 0 };
            const el = layer.createTile(coords, () => { doneCalls++; });
            layer._tiles = { '0:0:0': { el, coords } };
            assert.equal(doneCalls, 0, 'nothing has arrived yet');

            // A checkbox toggle lands before any layer of this tile has
            // arrived: the first render is cancelled and replaced.
            layer.refreshTiles();
            await new Promise(r => setTimeout(r, 0));
            assert.equal(doneCalls, 0,
                         'the superseded render painted nothing to reveal');

            // The replacement render's tiles arrive and paint.
            for (const item of ITEMS) {
                mgr.reply(item.layer);
            }
            await new Promise(r => setTimeout(r, 0));
            assert.equal(doneCalls, 1, 'the painted tile must be revealed');
        } finally {
            stub.restore();
        }
    });

    it('releases the decodes of a render whose requests were cancelled', async () => {
        // The release lives in renderMergedTile's finally, so it only runs if
        // every await settles.  A cancelled request that never settles would
        // strand the sibling layers' ImageBitmaps — unbounded decoded-image
        // growth, which is the failure this whole layer exists to prevent.
        const mgr = cancellableManager();
        const stub = stubCanvas2d();
        try {
            const { layer, decoded } = trackingLayer(mgr, ITEMS);
            layer._map = {};
            const coords = { x: 0, y: 0, z: 0 };
            const el = layer.createTile(coords, () => {});
            layer._tiles = { '0:0:0': { el, coords } };

            // One layer lands; the tile is then refreshed with the other two
            // still on the wire, which cancels them.
            mgr.reply('metal1');
            await new Promise(r => setTimeout(r, 0));
            assert.equal(decoded.length, 1, 'one layer decoded');

            layer.refreshTiles();
            await new Promise(r => setTimeout(r, 0));

            assert.ok(decoded[0].closed,
                      'the arrived decode must be released, not stranded');
        } finally {
            stub.restore();
        }
    });

    it('does not paint a superseded generation', async () => {
        // A refresh during an in-flight render must not let the older render
        // land on the canvas afterwards.
        const stub = stubCanvas2d();
        let release;
        const mgr = fakeManager();
        mgr.request = (msg) => {
            mgr.sent.push(msg);
            return new Promise((resolve) => {
                release = () => resolve({ payloadFor: msg.layer });
            });
        };
        try {
            const layer = makeLayer(mgr, ITEMS);
            layer._map = {};
            const el = layer.createTile({ x: 0, y: 0, z: 0 }, () => {});
            layer._tiles = { '0:0:0': { el, coords: { x: 0, y: 0, z: 0 } } };

            layer._generation++;   // a refresh happened while in flight
            release();
            await new Promise(r => setTimeout(r, 0));
            assert.equal(stub.paints.length, 0,
                         'stale generation must not paint');
        } finally {
            stub.restore();
        }
    });

    it('setItems swaps the draw list and repaints', async () => {
        const mgr = fakeManager();
        const stub = stubCanvas2d();
        try {
            const layer = makeLayer(mgr, ITEMS);
            layer._map = {};
            const el = layer.createTile({ x: 0, y: 0, z: 0 }, () => {});
            layer._tiles = { '0:0:0': { el, coords: { x: 0, y: 0, z: 0 } } };
            await new Promise(r => setTimeout(r, 0));

            mgr.sent.length = 0;
            layer.setItems([{ layer: 'metal9', opacity: 0.5, visible: true }]);
            await new Promise(r => setTimeout(r, 0));
            assert.deepEqual(mgr.sent.map(m => m.layer), ['metal9']);
        } finally {
            stub.restore();
        }
    });
});

describe('_removeTile', () => {
    it('cancels in-flight work and drops the backing store', () => {
        // A pruned tile that keeps its canvas is the same unbounded growth in
        // a different costume.
        const mgr = fakeManager({ hang: true });
        const stub = stubCanvas2d();
        try {
            const layer = makeLayer(mgr, ITEMS);
            layer._map = {};
            const el = layer.createTile({ x: 0, y: 0, z: 0 }, () => {});
            layer._tiles = { '0:0:0': { el, coords: { x: 0, y: 0, z: 0 } } };
            const ids = [...el._orRequestIds];

            removedKeys.length = 0;
            layer._removeTile('0:0:0');
            assert.deepEqual(mgr.cancelled, ids);
            // Flagged so an in-flight render past its request phase declines
            // to paint a detached canvas.
            assert.equal(el._orRemoved, true);
            assert.equal(el.width, 0);
            assert.equal(el.height, 0);
            assert.deepEqual(removedKeys, ['0:0:0'],
                             'must still chain to Leaflet');
        } finally {
            stub.restore();
        }
    });
});

describe('buildMergedPanes', () => {
    it('creates one opaque pane per group, stacked in group order', () => {
        const mgr = fakeManager();
        const Layer = createMergedTileLayer(CTX, { dpr: () => 1 });
        const items = Array.from({ length: 9 }, (_, i) => ({
            layer: 'm' + i, opacity: 0.7, visible: true,
        }));
        const groups = partitionIntoGroups(items, 3);
        const map = {};
        const panes = buildMergedPanes(Layer, mgr, groups, map, 5);

        assert.equal(panes.length, 3);
        panes.forEach((pane, i) => {
            assert.equal(pane.options.zIndex, 5 + i);
            // The double-apply guard: per-layer opacity lives inside the
            // canvas, so the pane must be fully opaque.
            assert.equal(pane.options.opacity, 1);
            assert.equal(pane._map, map);
        });
        // Flattening the panes' items must restore the original z-order.
        assert.deepEqual(panes.flatMap(p => p._items.map(it => it.layer)),
                         items.map(it => it.layer));
    });

    it('tolerates no map (tests and headless construction)', () => {
        const Layer = createMergedTileLayer(CTX, { dpr: () => 1 });
        const panes = buildMergedPanes(Layer, fakeManager(),
                                       [[{ layer: 'm0', opacity: 1 }]], null);
        assert.equal(panes.length, 1);
    });
});

describe('decodeTilePayload', () => {
    it('resolves null for a null payload', async () => {
        assert.equal(await decodeTilePayload(null), null);
    });

    it('loads a data URI through an <img> (static report cache)', async () => {
        // Driven with a stub rather than a real <img>: jsdom never fires load or
        // error for a data URI, so a test that waited on one would hang and be
        // cancelled — which also takes out every test after it.
        const orig = document.createElement.bind(document);
        const fake = { onload: null, onerror: null, src: null };
        document.createElement = (tag) => (tag === 'img' ? fake : orig(tag));
        try {
            const p = decodeTilePayload('data:image/png;base64,AAAA');
            assert.equal(fake.src, 'data:image/png;base64,AAAA');
            fake.onload();
            assert.equal(await p, fake);
        } finally {
            document.createElement = orig;
        }
    });

    it('resolves null when a data URI fails to load', async () => {
        // A failed decode must not reject: renderMergedTile skips a null and
        // still paints the rest of the group.
        const orig = document.createElement.bind(document);
        const fake = { onload: null, onerror: null, src: null };
        document.createElement = (tag) => (tag === 'img' ? fake : orig(tag));
        try {
            const p = decodeTilePayload('data:image/png;base64,AAAA');
            fake.onerror();
            assert.equal(await p, null);
        } finally {
            document.createElement = orig;
        }
    });

    it('resolves null rather than throwing when createImageBitmap rejects', async () => {
        const had = 'createImageBitmap' in globalThis;
        const prev = globalThis.createImageBitmap;
        globalThis.createImageBitmap = () => Promise.reject(new Error('bad'));
        try {
            assert.equal(await decodeTilePayload({ fake: 'blob' }), null);
        } finally {
            if (had) {
                globalThis.createImageBitmap = prev;
            } else {
                delete globalThis.createImageBitmap;
            }
        }
    });

    it('prefers createImageBitmap for a blob, so the decode can be released', async () => {
        // The entire reason this layer exists: an ImageBitmap can be close()d
        // the moment it is drawn, an <img> cannot.
        const had = 'createImageBitmap' in globalThis;
        const prev = globalThis.createImageBitmap;
        let sawBlob = null;
        const bitmap = { close() {} };
        globalThis.createImageBitmap = (blob) => {
            sawBlob = blob;
            return Promise.resolve(bitmap);
        };
        try {
            const blob = { fake: 'blob' };
            assert.equal(await decodeTilePayload(blob), bitmap);
            assert.equal(sawBlob, blob);
        } finally {
            if (had) {
                globalThis.createImageBitmap = prev;
            } else {
                delete globalThis.createImageBitmap;
            }
        }
    });
});

describe('staleness must not be inferred from DOM attachment', () => {
    it('paints a tile that Leaflet has not appended yet', async () => {
        // Leaflet appends the element AFTER createTile returns, so treating
        // "not connected" as stale would drop the very first paint of every
        // tile and the whole map would stay blank. This is a regression test
        // for exactly that: canvas.isConnected was the original signal.
        const mgr = fakeManager();
        const stub = stubCanvas2d();
        try {
            const layer = makeLayer(mgr, [{ layer: 'metal1', opacity: 1,
                                            visible: true }]);
            layer._map = {};
            let el;
            await new Promise((resolve) => {
                el = layer.createTile({ x: 0, y: 0, z: 0 }, () => resolve());
            });
            assert.equal(el.isConnected, false, 'never appended here');
            assert.deepEqual(stub.last().map(d => d.id), ['metal1'],
                             'an unappended tile must still paint');
        } finally {
            stub.restore();
        }
    });

    it('declines to paint a tile that was pruned mid-flight', async () => {
        const stub = stubCanvas2d();
        let release;
        const mgr = fakeManager();
        mgr.request = (msg) => {
            mgr.sent.push(msg);
            return new Promise((resolve) => {
                release = () => resolve({ payloadFor: msg.layer });
            });
        };
        try {
            const layer = makeLayer(mgr, [{ layer: 'metal1', opacity: 1,
                                            visible: true }]);
            layer._map = {};
            const el = layer.createTile({ x: 0, y: 0, z: 0 }, () => {});
            layer._tiles = { '0:0:0': { el, coords: { x: 0, y: 0, z: 0 } } };

            layer._removeTile('0:0:0');
            release();
            await new Promise(r => setTimeout(r, 0));
            assert.equal(stub.paints.length, 0);
        } finally {
            stub.restore();
        }
    });
});

describe('cache-invalidation and refresh paths must reach a merged pane', () => {
    // These mirror the per-layer contract that redrawAllLayers() relies on.
    // The server invalidates its tile cache on a design edit and pushes
    // {"type":"refresh"}; main.js answers by walking app.allLayers and calling
    // refreshTiles() on each. A merged pane is registered in app.allLayers in
    // place of the ~94 per-layer panes, so if it did not honour that method the
    // viewer would silently stop updating after any design edit — with no error
    // anywhere, which is the worst kind of regression.
    //
    // C++ side: TileGeneratorTest.InPlaceDesignEditInvalidatesTileCache and
    // GeomCacheRebuiltAfterDebouncedEdit cover the server dropping its caches.
    // These cover the client actually re-asking for the tiles.

    function mountedLayer(mgr, items) {
        const layer = makeLayer(mgr, items);
        layer._map = {};
        const el = layer.createTile({ x: 0, y: 0, z: 0 }, () => {});
        layer._tiles = { '0:0:0': { el, coords: { x: 0, y: 0, z: 0 } } };
        return { layer, el };
    }

    it('exposes the refreshTiles method redrawAllLayers calls', () => {
        const layer = makeLayer(fakeManager(), ITEMS);
        assert.equal(typeof layer.refreshTiles, 'function');
    });

    it('re-requests after a design-change refresh, not serving stale tiles', async () => {
        const mgr = fakeManager();
        const stub = stubCanvas2d();
        try {
            const { layer } = mountedLayer(mgr, ITEMS);
            await new Promise(r => setTimeout(r, 0));
            const firstRound = mgr.sent.length;
            assert.equal(firstRound, 3);

            // What main.js does on a {"type":"refresh"} push.
            for (const l of [layer]) {
                l.refreshTiles();
            }
            await new Promise(r => setTimeout(r, 0));
            assert.equal(mgr.sent.length, firstRound + 3,
                         'every item must be re-asked for after invalidation');
        } finally {
            stub.restore();
        }
    });

    it('picks up a changed fill pattern on refresh', async () => {
        // setLayerPattern mutates app.layerPatterns and calls refreshTiles.
        // buildTileRequestFor reads the pattern lazily, so the new value has to
        // appear on the wire without the layer being rebuilt.
        const mgr = fakeManager();
        const stub = stubCanvas2d();
        const app = { layerPatterns: {} };
        try {
            const Layer = createMergedTileLayer(
                { ...CTX, app },
                { decode: async () => ({ close() {} }), dpr: () => 1 });
            const layer = new Layer(mgr, [{ layer: 'metal1', opacity: 0.7,
                                            visible: true }], {});
            layer._map = {};
            const el = layer.createTile({ x: 0, y: 0, z: 0 }, () => {});
            layer._tiles = { '0:0:0': { el, coords: { x: 0, y: 0, z: 0 } } };
            await new Promise(r => setTimeout(r, 0));
            assert.equal('pattern' in mgr.sent[0], false, 'solid is omitted');

            app.layerPatterns.metal1 = 3;
            mgr.sent.length = 0;
            layer.refreshTiles();
            await new Promise(r => setTimeout(r, 0));
            assert.equal(mgr.sent[0].pattern, 3);
        } finally {
            stub.restore();
        }
    });

    it('picks up a changed visible_layers set on refresh', async () => {
        // The server filters pin markers and route guides by visible_layers, so
        // a stale set silently renders the wrong thing rather than failing.
        const mgr = fakeManager();
        const stub = stubCanvas2d();
        const visibleLayers = new Set(['metal1']);
        try {
            const Layer = createMergedTileLayer(
                { ...CTX, visibleLayers },
                { decode: async () => ({ close() {} }), dpr: () => 1 });
            const layer = new Layer(mgr, [{ layer: 'metal1', opacity: 0.7,
                                            visible: true }], {});
            layer._map = {};
            const el = layer.createTile({ x: 0, y: 0, z: 0 }, () => {});
            layer._tiles = { '0:0:0': { el, coords: { x: 0, y: 0, z: 0 } } };
            await new Promise(r => setTimeout(r, 0));
            assert.deepEqual(mgr.sent[0].visible_layers, ['metal1']);

            visibleLayers.add('metal2');
            mgr.sent.length = 0;
            layer.refreshTiles();
            await new Promise(r => setTimeout(r, 0));
            assert.deepEqual(mgr.sent[0].visible_layers.sort(),
                             ['metal1', 'metal2']);
        } finally {
            stub.restore();
        }
    });

    it('picks up a changed visible_chiplets set on refresh', async () => {
        const mgr = fakeManager();
        const stub = stubCanvas2d();
        const app = { visibleChiplets: new Set(['top']) };
        try {
            const Layer = createMergedTileLayer(
                { ...CTX, app },
                { decode: async () => ({ close() {} }), dpr: () => 1 });
            const layer = new Layer(mgr, [{ layer: 'metal1', opacity: 0.7,
                                            visible: true }], {});
            layer._map = {};
            const el = layer.createTile({ x: 0, y: 0, z: 0 }, () => {});
            layer._tiles = { '0:0:0': { el, coords: { x: 0, y: 0, z: 0 } } };
            await new Promise(r => setTimeout(r, 0));
            assert.deepEqual(mgr.sent[0].visible_chiplets, ['top']);

            app.visibleChiplets.add('top.mem_0');
            mgr.sent.length = 0;
            layer.refreshTiles();
            await new Promise(r => setTimeout(r, 0));
            assert.deepEqual(mgr.sent[0].visible_chiplets.sort(),
                             ['top', 'top.mem_0']);
        } finally {
            stub.restore();
        }
    });

    it('sends a request payload identical to the per-layer path', async () => {
        // The two layer types must not drift on the wire, or the server's tile
        // cache keys diverge and one path silently misses the cache entirely.
        const { buildTileRequest }
            = await import('../../src/websocket-tile-layer.js');
        const { buildTileRequestFor } = await import('../../src/tile-request.js');
        const app = { layerPatterns: { metal1: 3 },
                      visibleChiplets: new Set(['top']) };
        const ctx = { ...CTX, app };
        const coords = { x: 4, y: 5, z: 6 };
        const legacy = buildTileRequest(coords, 'metal1', ctx);
        const merged = buildTileRequestFor(coords, 'metal1', ctx, legacy.dpr);
        assert.deepEqual(merged, legacy);
    });
});

describe('canvas backing store follows the device pixel ratio', () => {
    // devicePixelRatio changes when the window moves between monitors or the
    // browser zoom changes. A refresh then requests images at the new dpr, so a
    // canvas still sized for the old one would scale every incoming tile into
    // the wrong backing store — existing tiles blurry, and inconsistent with
    // any created afterwards.
    function layerWithDpr(mgr, dprRef) {
        const Layer = createMergedTileLayer(CTX, {
            decode: async (payload) => (payload
                ? { id: payload.payloadFor, close() {} } : null),
            dpr: () => dprRef.value,
        });
        const layer = new Layer(mgr, [{ layer: 'metal1', opacity: 1,
                                        visible: true }], {});
        layer._map = {};
        return layer;
    }

    it('resizes an existing tile when the ratio changes', async () => {
        const stub = stubCanvas2d();
        const dprRef = { value: 1 };
        try {
            const mgr = fakeManager();
            const layer = layerWithDpr(mgr, dprRef);
            const el = layer.createTile({ x: 0, y: 0, z: 0 }, () => {});
            layer._tiles = { '0:0:0': { el, coords: { x: 0, y: 0, z: 0 } } };
            await new Promise(r => setTimeout(r, 0));
            assert.equal(el.width, 256);

            dprRef.value = 2;
            layer.refreshTiles();
            await new Promise(r => setTimeout(r, 0));
            assert.equal(el.width, 512, 'backing store must follow the dpr');
            assert.equal(el.height, 512);
            // The CSS box stays in layout pixels either way.
            assert.equal(el.style.width, '256px');
        } finally {
            stub.restore();
        }
    });

    it('requests at the same ratio it sizes the canvas for', async () => {
        // The two must not drift: the server renders 256*dpr and the canvas
        // draws at its own width, so a mismatch resamples every tile.
        const stub = stubCanvas2d();
        const dprRef = { value: 2 };
        try {
            const mgr = fakeManager();
            const layer = layerWithDpr(mgr, dprRef);
            const el = layer.createTile({ x: 0, y: 0, z: 0 }, () => {});
            await new Promise(r => setTimeout(r, 0));
            assert.equal(mgr.sent[0].dpr, 2);
            assert.equal(el.width, 256 * mgr.sent[0].dpr);
        } finally {
            stub.restore();
        }
    });

    it('leaves the canvas alone when the ratio is unchanged', async () => {
        // Assigning width clears the canvas, so doing it unnecessarily would
        // blank every tile on every refresh.
        const stub = stubCanvas2d();
        const dprRef = { value: 1 };
        try {
            const mgr = fakeManager();
            const layer = layerWithDpr(mgr, dprRef);
            const el = layer.createTile({ x: 0, y: 0, z: 0 }, () => {});
            layer._tiles = { '0:0:0': { el, coords: { x: 0, y: 0, z: 0 } } };
            await new Promise(r => setTimeout(r, 0));

            let writes = 0;
            let w = el.width;
            Object.defineProperty(el, 'width', {
                get: () => w,
                set: (v) => { writes++; w = v; },
                configurable: true,
            });
            layer.refreshTiles();
            await new Promise(r => setTimeout(r, 0));
            assert.equal(writes, 0, 'must not clear the canvas needlessly');
        } finally {
            stub.restore();
        }
    });
});
