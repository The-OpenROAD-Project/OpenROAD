// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';

const {
    createCompositeSource, createImageSource, buildOverlayRequest,
    buildHeatmapRequest, releaseDrawable,
} = await import('../../src/tile-sources.js');
const { LayerExtents } = await import('../../src/layer-extents.js');
const { buildGroupTileRequestFor, tileRequestBuilder }
    = await import('../../src/tile-request.js');

// A manager whose replies the test hands out per layer.  Ids start high and
// are only known from each promise's requestId, as with WebSocketManager.
function makeWm() {
    const pending = new Map();   // id -> { msg, resolve, reject }
    const wm = {
        _next: 1000,
        sent: [],
        cancelled: [],
        promoted: [],
        request(msg, opts = {}) {
            const id = ++this._next;
            wm.sent.push({ id, msg, background: !!opts.background });
            const promise = new Promise((resolve, reject) => {
                pending.set(id, { msg, resolve, reject });
            });
            promise.requestId = id;
            return promise;
        },
        reply(layer, payload) {
            for (const [id, p] of [...pending]) {
                if ((p.msg.layer || p.msg.type) === layer) {
                    pending.delete(id);
                    p.resolve(payload);
                }
            }
        },
        fail(layer) {
            for (const [id, p] of [...pending]) {
                if ((p.msg.layer || p.msg.type) === layer) {
                    pending.delete(id);
                    p.reject(new Error('WebSocket closed'));
                }
            }
        },
        cancel(id) {
            wm.cancelled.push(id);
            const p = pending.get(id);
            if (p) {
                pending.delete(id);
                p.reject(new Error('Request cancelled'));
            }
        },
        promote(id) { wm.promoted.push(id); },
    };
    return wm;
}

// Canvas stand-in that records what is drawn and how many canvases were made;
// snapshots are numbered.
function makeCanvasKit() {
    const kit = { draws: [], snapshots: 0, canvases: 0 };
    kit.makeCanvas = (px) => {
        kit.canvases++;
        return {
            width: px, height: px,
            getContext: () => ({
                globalAlpha: 1,
                clearRect() { kit.draws.push('clear'); },
                drawImage(image) { kit.draws.push(image.layer); },
            }),
        };
    };
    kit.snapshot = (canvas) => ({ image: { snap: ++kit.snapshots },
                                  width: canvas.width, height: canvas.height });
    return kit;
}

const ctx = { visibility: {}, selectability: {}, visibleLayers: new Set(),
              selectableLayers: new Set(), app: null };
const ITEMS = [
    { layer: '_instances', opacity: 1, visible: true },
    { layer: 'metal1', opacity: 1, visible: true },
    { layer: 'metal2', opacity: 0.5, visible: false },
];
const tick = () => new Promise((resolve) => setTimeout(resolve, 0));
const decode = async (payload) => (payload ? { layer: payload } : null);

function composite(over = {}) {
    const wm = makeWm();
    const kit = makeCanvasKit();
    const source = createCompositeSource({
        wm, ctx, getItems: () => ITEMS, dpr: () => 2, tileSize: () => 240,
        decode, makeCanvas: kit.makeCanvas, snapshot: kit.snapshot, ...over,
    });
    return { wm, kit, source };
}

describe('createCompositeSource', () => {
    it('requests every visible layer of the tile, in the server grid', () => {
        const { wm, source } = composite();
        source.load({ z: 3, x: 5, y: 6 });
        assert.deepEqual(wm.sent.map((s) => s.msg.layer), ['_instances', 'metal1']);
        for (const { msg } of wm.sent) {
            assert.equal(msg.type, 'tile');
            assert.deepEqual([msg.z, msg.x, msg.y], [3, 5, 6]);
            assert.equal(msg.tile_px, 480);
        }
    });

    // Disconnected: the tile keeps what it showed rather than going blank.
    it('rejects when every request fails', async () => {
        const { wm, source } = composite();
        const handle = source.load({ z: 3, x: 0, y: 0 });
        wm.fail('_instances');
        wm.fail('metal1');
        await assert.rejects(handle.promise, /failed/);
    });

    it('draws what arrived when only some requests fail', async () => {
        const { wm, kit, source } = composite();
        const handle = source.load({ z: 3, x: 0, y: 0 });
        wm.fail('_instances');
        wm.reply('metal1', 'metal1');
        assert.ok(await handle.promise);
        assert.deepEqual(kit.draws, ['clear', 'metal1']);
    });

    it('is blank when every layer comes back empty', async () => {
        const { wm, source } = composite();
        const handle = source.load({ z: 3, x: 0, y: 0 });
        wm.reply('_instances', null);
        wm.reply('metal1', null);
        assert.equal(await handle.promise, null);
    });

    it('sends a ring tile at background priority and can promote it', () => {
        const { wm, source } = composite();
        const handle = source.load({ z: 3, x: 0, y: 0 }, { background: true });
        assert.ok(wm.sent.every((s) => s.background));
        handle.promote();
        assert.deepEqual(wm.promoted, wm.sent.map((s) => s.id));
    });

    // Double buffering: the tile is drawn once, with every layer in.
    it('paints once, complete, with double buffering', async () => {
        const { wm, kit, source } = composite({ isDoubleBuffer: () => true });
        const partials = [];
        const handle = source.load({ z: 3, x: 0, y: 0 },
                                   { onPartial: (d) => partials.push(d) });
        wm.reply('metal1', 'metal1');
        await tick();
        assert.equal(partials.length, 0);
        wm.reply('_instances', '_instances');
        const drawable = await handle.promise;
        assert.deepEqual(kit.draws, ['clear', '_instances', 'metal1']);
        assert.deepEqual(drawable.image, { snap: 1 });
        assert.equal(drawable.width, 480);
    });

    it('publishes every layer as it lands without double buffering', async () => {
        const { wm, source } = composite({ isDoubleBuffer: () => false });
        const partials = [];
        const handle = source.load({ z: 3, x: 0, y: 0 },
                                   { onPartial: (d) => partials.push(d) });
        wm.reply('metal1', 'metal1');
        await tick();
        assert.equal(partials.length, 1);
        wm.reply('_instances', '_instances');
        assert.equal(await handle.promise, undefined,
                     'the last partial is the final image');
        assert.equal(partials.length, 2);
    });

    it('resolves an all-empty tile to null', async () => {
        const { wm, source } = composite();
        const handle = source.load({ z: 3, x: 0, y: 0 });
        wm.reply('_instances', null);
        wm.reply('metal1', null);
        assert.equal(await handle.promise, null);
    });

    it('allocates a canvas only once a layer has an image', async () => {
        const empty = composite();
        const h1 = empty.source.load({ z: 3, x: 0, y: 0 });
        empty.wm.reply('_instances', null);
        empty.wm.reply('metal1', null);
        await h1.promise;
        const cancelled = composite();
        const h2 = cancelled.source.load({ z: 3, x: 0, y: 0 });
        h2.cancel();
        await assert.rejects(h2.promise);
        assert.equal(empty.kit.canvases + cancelled.kit.canvases, 0);

        const { wm, kit, source } = composite({ isDoubleBuffer: () => false });
        const h3 = source.load({ z: 3, x: 0, y: 0 }, { onPartial: () => {} });
        wm.reply('metal1', 'metal1');
        wm.reply('_instances', '_instances');
        await h3.promise;
        assert.equal(kit.canvases, 1);
    });

    it('needs no request when no layer is visible', async () => {
        const { wm, source } = composite({ getItems: () => [] });
        const handle = source.load({ z: 3, x: 0, y: 0 });
        assert.equal(wm.sent.length, 0);
        assert.equal(await handle.promise, null);
    });

    it('cancels every request of the tile', async () => {
        const { wm, source } = composite();
        const handle = source.load({ z: 3, x: 0, y: 0 });
        handle.cancel();
        assert.deepEqual(wm.cancelled, wm.sent.map((s) => s.id));
        await assert.rejects(handle.promise);
    });
});

// Multi-chiplet designs list a shared tech layer once per chiplet; the server
// answers every copy with the same image.
describe('createCompositeSource with repeated layers', () => {
    const copies = [
        { layer: '_instances', opacity: 1, visible: true },
        { layer: 'M_TOP', opacity: 1, visible: true },   // chiplet A
        { layer: 'UBM', opacity: 1, visible: true },
        { layer: 'M_TOP', opacity: 1, visible: true },   // chiplet B
        { layer: 'UBM', opacity: 1, visible: false },
    ];

    it('requests and composites each distinct layer once, in order', async () => {
        const { wm, kit, source } = composite({ getItems: () => copies });
        const h = source.load({ z: 1, x: 0, y: 0 });
        assert.deepEqual(wm.sent.map((s) => s.msg.layer),
                         ['_instances', 'M_TOP', 'UBM']);
        for (const layer of ['_instances', 'M_TOP', 'UBM']) {
            wm.reply(layer, layer);
        }
        await h.promise;
        assert.deepEqual(kit.draws.filter((d) => d !== 'clear'),
                         ['_instances', 'M_TOP', 'UBM']);
    });

    it('keeps a layer while any copy of it is visible', () => {
        const items = [{ layer: 'M_TOP', opacity: 1, visible: false },
                       { layer: 'M_TOP', opacity: 1, visible: true }];
        const { wm, source } = composite({ getItems: () => items });
        source.load({ z: 1, x: 0, y: 0 });
        assert.deepEqual(wm.sent.map((s) => s.msg.layer), ['M_TOP']);
    });
});

// The server's layer_extents bound where each layer has shapes; a tile outside
// a layer's box is not asked for that layer (layer-extents.js).
describe('createCompositeSource with layer extents', () => {
    // metal1 lives in the top-left quarter of the zoom-0 grid only.
    function withExtents() {
        const extents = new LayerExtents();
        extents.apply(extents.invalidate(), {
            supported: true, layers: { metal1: [0, 0, 0.25, 0.25] },
        });
        const app = { layerExtents: extents };
        return { extents, ctx: { ...ctx, app } };
    }
    const both = [{ layer: '_instances', opacity: 1, visible: true },
                  { layer: 'metal1', opacity: 1, visible: true }];

    it('does not request a layer with nothing in the tile', () => {
        const { ctx: c } = withExtents();
        const { wm, source } = composite({ ctx: c, getItems: () => both });
        source.load({ z: 1, x: 1, y: 1 });
        assert.deepEqual(wm.sent.map((s) => s.msg.layer), ['_instances']);
        source.load({ z: 1, x: 0, y: 0 });
        assert.deepEqual(wm.sent.slice(1).map((s) => s.msg.layer),
                         ['_instances', 'metal1']);
    });

    it('completes a tile whose layers are all ruled out, without requests',
       async () => {
        const { ctx: c } = withExtents();
        const only = [{ layer: 'metal1', opacity: 1, visible: true }];
        const { wm, source } = composite({ ctx: c, getItems: () => only });
        const h = source.load({ z: 2, x: 3, y: 3 });
        assert.equal(await h.promise, null);
        assert.equal(wm.sent.length, 0);
    });

    it('requests the layer again once the extents are invalidated', () => {
        const { extents, ctx: c } = withExtents();
        const { wm, source } = composite({ ctx: c, getItems: () => both });
        extents.invalidate();
        source.load({ z: 1, x: 1, y: 1 });
        assert.deepEqual(wm.sent.map((s) => s.msg.layer),
                         ['_instances', 'metal1']);
    });
});

describe('createImageSource', () => {
    it('decodes one request into a drawable', async () => {
        const wm = makeWm();
        const source = createImageSource({
            wm, decode: async (p) => (p ? { width: 480, height: 480 } : null),
            buildRequest: (c) => ({ type: 'overlay_tile', ...c }),
        });
        const handle = source.load({ z: 2, x: 1, y: 1 }, { background: true });
        assert.equal(wm.sent[0].background, true);
        wm.reply('overlay_tile', 'png');
        assert.deepEqual((await handle.promise).width, 480);
    });

    it('makes no request when the builder returns null', async () => {
        const wm = makeWm();
        const source = createImageSource({ wm, buildRequest: () => null });
        assert.equal(await source.load({ z: 0, x: 0, y: 0 }).promise, null);
        assert.equal(wm.sent.length, 0);
    });
});

describe('request builders', () => {
    it('builds the overlay request with the visible layers', () => {
        const req = buildOverlayRequest({ z: 1, x: 0, y: 1 }, 1, 240,
            { highlight_selected: false, labels: true },
            { visibleLayerNames: new Set(['m1']), polyDecomp: true });
        assert.equal(req.type, 'overlay_tile');
        assert.equal(req.tile_px, 240);
        assert.equal(req.highlight_selected, false);
        assert.equal(req.poly_decomp, true);
        assert.deepEqual(req.visible_layers, ['m1']);
    });

    it('builds no heat-map request without an active heat map', () => {
        assert.equal(buildHeatmapRequest({ z: 1, x: 0, y: 0 }, 1, 240, {}), null);
        assert.equal(buildHeatmapRequest({ z: 1, x: 0, y: 0 }, 1, 240,
                                         { activeHeatMap: 'rudy' }).name, 'rudy');
    });
});

describe('releaseDrawable', () => {
    it('closes a bitmap and tolerates anything else', () => {
        let closed = false;
        releaseDrawable({ image: { close() { closed = true; } } });
        assert.ok(closed);
        assert.doesNotThrow(() => releaseDrawable(null));
        assert.doesNotThrow(() => releaseDrawable({ image: {} }));
    });
});

describe('tile_group requests', () => {
    const coords = { x: 1, y: 2, z: 3 };
    const gctx = {
        visibility: { stdcells: true, scale_bar: false },
        selectability: { stdcells: true },
        visibleLayers: new Set(['metal1', 'metal2']),
        selectableLayers: new Set(['metal1']),
        app: {
            visibleChiplets: new Set(['chip']),
            layerPatterns: { metal2: 3 },
        },
    };

    it('carries the layers bottom first, with the render fields of tile', () => {
        const group = buildGroupTileRequestFor(
            coords, ['metal1', 'metal2'], gctx, 2, 240);
        const tile = tileRequestBuilder(coords, gctx, 2, 240)('metal1');
        assert.equal(group.type, 'tile_group');
        assert.deepEqual(group.layers, ['metal1', 'metal2']);
        for (const k of ['z', 'x', 'y', 'dpr', 'tile_px', 'visible_layers',
                         'visible_chiplets', 'stdcells', 'scale_bar']) {
            assert.deepEqual(group[k], tile[k], k);
        }
    });

    it('leaves selectability off the wire, while tile keeps it', () => {
        const group = buildGroupTileRequestFor(coords, ['metal1'], gctx, 1, 240);
        assert.equal(group.selectable_layers, undefined);
        assert.equal(group.s_stdcells, undefined);
        const tile = tileRequestBuilder(coords, gctx, 1, 240)('metal1');
        assert.deepEqual(tile.selectable_layers, ['metal1']);
        assert.equal(tile.s_stdcells, true);
    });

    it('sends one pattern per layer, and none when all are solid', () => {
        assert.deepEqual(buildGroupTileRequestFor(
            coords, ['metal1', 'metal2'], gctx, 1, 240).patterns, [1, 3]);
        assert.equal(buildGroupTileRequestFor(
            coords, ['metal1'], gctx, 1, 240).patterns, undefined);
    });
});

// With ?composite=1 the server composites runs of layers (tile_group).
describe('createCompositeSource with server groups', () => {
    const five = ['_instances', 'm1', 'm2', 'm3', 'm4'].map(
        (layer) => ({ layer, opacity: 1, visible: true }));

    it('asks for bottom-up runs of up to K layers, one request each', async () => {
        const { wm, kit, source } = composite({ getItems: () => five,
                                                groupLayers: () => 3 });
        const h = source.load({ z: 1, x: 0, y: 0 });
        assert.deepEqual(wm.sent.map((s) => [s.msg.type, s.msg.layers]),
                         [['tile', undefined],
                          ['tile_group', ['m1', 'm2', 'm3']],
                          ['tile_group', ['m4']]]);
        wm.reply('_instances', 'instances');
        wm.reply('tile_group', 'group');
        const d = await h.promise;
        assert.ok(d);
        assert.deepEqual(kit.draws.filter((x) => x !== 'clear'),
                         ['instances', 'group', 'group']);
    });

    it('groups after dropping repeated and hidden layers', () => {
        const items = [...five, { layer: 'm1', opacity: 1, visible: true },
                       { layer: 'm9', opacity: 1, visible: false }];
        const { wm, source } = composite({ getItems: () => items,
                                           groupLayers: () => 10 });
        source.load({ z: 1, x: 0, y: 0 });
        assert.deepEqual(wm.sent.map((s) => s.msg.layers || s.msg.layer),
                         ['_instances', ['m1', 'm2', 'm3', 'm4']]);
    });

    // The server only composites tech layers.
    it('keeps a pseudo layer in its own request, splitting the runs', () => {
        const items = ['m1', '_mfg_grid', 'm2', 'm3', 'm4', '_gcell_grid'].map(
            (layer) => ({ layer, opacity: 1, visible: true }));
        const { wm, source } = composite({ getItems: () => items,
                                           groupLayers: () => 3 });
        source.load({ z: 1, x: 0, y: 0 });
        assert.deepEqual(wm.sent.map((s) => s.msg.layers || s.msg.layer),
                         [['m1'], '_mfg_grid', ['m2', 'm3', 'm4'],
                          '_gcell_grid']);
    });

    // Runs come from the whole list, not the visible part of it.
    it('hiding a layer changes only the request of its own run', () => {
        const items = ['_instances', 'm1', 'm2', 'm3', 'm4', 'm5', 'm6'].map(
            (layer) => ({ layer, opacity: 1, visible: true }));
        const { wm, source } = composite({ getItems: () => items,
                                           groupLayers: () => 3 });
        source.load({ z: 1, x: 0, y: 0 });
        const before = wm.sent.map((s) => JSON.stringify(s.msg));
        items[2].visible = false;   // m2
        wm.sent.length = 0;
        source.load({ z: 1, x: 0, y: 0 });
        const after = wm.sent.map((s) => JSON.stringify(s.msg));
        assert.deepEqual(wm.sent.map((s) => s.msg.layers || s.msg.layer),
                         ['_instances', ['m1', 'm3'], ['m4', 'm5', 'm6']]);
        assert.equal(after[0], before[0]);
        assert.notEqual(after[1], before[1]);
        assert.equal(after[2], before[2]);
    });

    it('fetches each layer when a layer is translucent', () => {
        const items = [...five.slice(0, 2),
                       { layer: 'm2', opacity: 0.5, visible: true }];
        const { wm, source } = composite({ getItems: () => items,
                                           groupLayers: () => 3 });
        source.load({ z: 1, x: 0, y: 0 });
        assert.deepEqual(wm.sent.map((s) => s.msg.type),
                         ['tile', 'tile', 'tile']);
    });

    it('fetches each layer when groups are off', () => {
        const { wm, source } = composite({ getItems: () => five });
        source.load({ z: 1, x: 0, y: 0 });
        assert.equal(wm.sent.length, 5);
        assert.ok(wm.sent.every((s) => s.msg.type === 'tile'));
    });

    it('cancels its group requests', () => {
        const { wm, source } = composite({ getItems: () => five,
                                           groupLayers: () => 3 });
        const h = source.load({ z: 1, x: 0, y: 0 });
        h.promise.catch(() => {});
        h.cancel();
        assert.deepEqual(wm.cancelled, wm.sent.map((s) => s.id));
    });
});
