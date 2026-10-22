// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Where the layout view's tiles come from.  Each source turns a tile (z, x, y)
// in the server's grid into a drawable `{ image, width, height }`, or null for
// a tile with nothing in it, and implements the TileGrid source contract:
// `load(coords, {background, onPartial}) -> {promise, cancel, promote}` and
// `release(drawable)`.

import {
    buildGroupTileRequestFor, tileDevicePx, tileRequestBuilder, tileSizeFields,
} from './tile-request.js';
import { closeAll, mergeOntoContext, renderMergedTile } from './tile-merge.js';
import { tileMayHaveContent } from './layer-extents.js';

// Decode a tile payload: a Blob from the server, or a data URI from a saved
// report.  Straight alpha and no colour conversion, so the pixels reach the
// GPU as the server wrote them.
function decodeTilePayload(payload) {
    if (payload == null) {
        return Promise.resolve(null);
    }
    if (typeof payload === 'string') {
        return new Promise((resolve) => {
            const img = document.createElement('img');
            img.onload = () => resolve(img);
            img.onerror = () => resolve(null);
            img.src = payload;
        });
    }
    return createImageBitmap(payload, { premultiplyAlpha: 'none',
                                        colorSpaceConversion: 'none' })
        .catch(() => null);
}

export function releaseDrawable(drawable) {
    closeAll([drawable && drawable.image]);
}

function imageSize(image) {
    return {
        width: image.naturalWidth || image.width,
        height: image.naturalHeight || image.height,
    };
}

function asDrawable(image) {
    return image ? { image, ...imageSize(image) } : null;
}

function defaultCanvas(px) {
    if (typeof OffscreenCanvas === 'function') {
        return new OffscreenCanvas(px, px);
    }
    const canvas = document.createElement('canvas');
    canvas.width = px;
    canvas.height = px;
    return canvas;
}

// A 2D canvas holding a copy of `src`.
export function copyToCanvas2d(src) {
    const copy = document.createElement('canvas');
    copy.width = src.width;
    copy.height = src.height;
    copy.getContext('2d').drawImage(src, 0, 0);
    return copy;
}

// Snapshot of the composite.  transferToImageBitmap empties the canvas, which
// is fine: every paint recomposites from scratch.
function defaultSnapshot(canvas) {
    if (typeof canvas.transferToImageBitmap === 'function') {
        return asDrawable(canvas.transferToImageBitmap());
    }
    return asDrawable(copyToCanvas2d(canvas));
}

const NOTHING = Object.freeze({
    promise: Promise.resolve(null), cancel() {}, promote() {},
});

// Issue one request through `wm`, tracking its id for cancel and promote.
function tracked(wm, ids, msg, background) {
    const promise = wm.request(msg, { background });
    if (promise.requestId != null) {
        ids.push(promise.requestId);
    }
    return promise;
}

// The load handle over the requests in `ids`; `onCancel` runs before they are
// cancelled.
function loadHandle(wm, ids, promise, onCancel) {
    return {
        promise,
        cancel() {
            onCancel();
            for (const id of ids) {
                wm.cancel(id);
            }
        },
        promote() {
            for (const id of ids) {
                wm.promote?.(id);
            }
        },
    };
}

// Layers per group when the server composites runs of layers (tile_group).
export const DEFAULT_GROUP_LAYERS = 3;

// Each tech layer's run: the slot's whole list cut into runs of up to `perRun`
// between pseudo layers, so a toggle changes the request of its own run only.
// Pseudo layers ('_instances', ...) map to null: the server groups tech
// layers only.
function runOf(all, perRun) {
    const runs = new Map();
    let segment = 0;
    let n = 0;
    for (const item of all) {
        if (!item || runs.has(item.layer)) {
            continue;
        }
        if (item.layer.startsWith('_')) {
            runs.set(item.layer, null);
            segment++;
            n = 0;
            continue;
        }
        runs.set(item.layer, `${segment}:${Math.floor(n++ / perRun)}`);
    }
    return runs;
}

// One tile_group request per run present in the tile, bottom up, and a
// 'tile' request for each pseudo layer.
function groupRequests(all, items, perRun, build, coords, ctx, dpr, tileSize) {
    const runs = runOf(all, perRun);
    const parts = [];
    let run = null;
    const flush = () => {
        if (run) {
            parts.push({ opacity: 1, msg: buildGroupTileRequestFor(
                coords, run.layers, ctx, dpr, tileSize) });
            run = null;
        }
    };
    for (const item of items) {
        const id = runs.get(item.layer);
        if (id == null) {
            flush();
            parts.push({ opacity: 1, msg: build(item.layer) });
            continue;
        }
        if (run && run.id !== id) {
            flush();
        }
        run = run || { id, layers: [] };
        run.layers.push(item.layer);
    }
    flush();
    return parts;
}

// The layout layers of a slot, composited into one image per tile in z-order
// (`getItems()` returns [{layer, opacity, visible}], bottom-most first).  With
// double buffering the tile is drawn once, complete; otherwise every layer that
// lands is published as a partial composite.  `groupLayers()` > 0 asks the
// server to composite runs of that many layers (tile_group).
export function createCompositeSource({
    wm, ctx, getItems, dpr = () => 1, tileSize = () => 240,
    isDoubleBuffer = () => true, decode = decodeTilePayload,
    makeCanvas = defaultCanvas, snapshot = defaultSnapshot,
    groupLayers = () => 0,
}) {
    return {
        load({ z, x, y }, { background = false, onPartial } = {}) {
            const all = getItems() || [];
            // A layer with nothing in this tile is not requested at all
            // (layer-extents.js).
            const visible = all.filter(
                (item) => item && item.visible !== false
                          && tileMayHaveContent(ctx, item.layer, { z, x, y }));
            const d = dpr();
            const t = tileSize();
            // One request per layer: every chiplet lists its own copy of a
            // shared tech layer, and the server answers each copy with the
            // same image, which would be composited once per copy.
            const seen = new Set();
            const items = visible.filter((item) => !seen.has(item.layer)
                                                   && seen.add(item.layer));
            if (items.length === 0) {
                return NOTHING;
            }
            const build = tileRequestBuilder({ z, x, y }, ctx, d, t);
            // The server composites at full opacity, so one translucent layer
            // sends the whole tile per layer.
            const perRun = groupLayers();
            const parts = perRun > 0
                && items.every((item) => (item.opacity ?? 1) === 1)
                ? groupRequests(all, items, perRun, build, { z, x, y }, ctx,
                                d, t)
                : items.map((item) => ({ opacity: item.opacity,
                                         msg: build(item.layer) }));
            const px = tileDevicePx(t, d);
            // Made on the first draw with an image, so an empty or
            // cancelled tile never allocates one.
            let canvas = null;
            let g = null;
            const ids = [];
            let cancelled = false;
            const progressive = !isDoubleBuffer()
                                && typeof onPartial === 'function';
            const promise = renderMergedTile({
                items: parts,
                request: (part) => tracked(wm, ids, part.msg, background),
                decode,
                draw: (sources) => {
                    if (!canvas) {
                        if (!sources.some((s) => s.image)) {
                            return 0;
                        }
                        canvas = makeCanvas(px);
                        g = canvas.getContext('2d');
                    }
                    const drawn = mergeOntoContext(g, sources, px);
                    if (progressive && drawn > 0 && !cancelled) {
                        onPartial(snapshot(canvas));
                    }
                    return drawn;
                },
                isStale: () => cancelled,
                incremental: progressive,
            }).then((stats) => {
                if (cancelled) {
                    throw new Error('Tile cancelled');
                }
                // Disconnected: keep what the tile showed, not a blank.
                if (stats.failed === stats.requested) {
                    throw new Error('Tile requests failed');
                }
                if (stats.drawn === 0) {
                    return null;
                }
                // Progressive: the last partial already holds every layer,
                // and undefined tells the grid to keep it.
                return progressive ? undefined : snapshot(canvas);
            });
            return loadHandle(wm, ids, promise, () => { cancelled = true; });
        },
        release: releaseDrawable,
    };
}

// One server image per tile: `buildRequest({z, x, y}, dpr, tileSize)` returns
// the request, or null for a tile that needs none.
export function createImageSource({
    wm, buildRequest, dpr = () => 1, tileSize = () => 240,
    decode = decodeTilePayload,
}) {
    return {
        load({ z, x, y }, { background = false } = {}) {
            const msg = buildRequest({ z, x, y }, dpr(), tileSize());
            if (!msg) {
                return NOTHING;
            }
            const ids = [];
            let cancelled = false;
            const promise = tracked(wm, ids, msg, background)
                .then((payload) => decode(payload))
                .then((image) => {
                    if (cancelled) {
                        releaseDrawable(asDrawable(image));
                        throw new Error('Tile cancelled');
                    }
                    return asDrawable(image);
                });
            return loadHandle(wm, ids, promise, () => { cancelled = true; });
        },
        release: releaseDrawable,
    };
}

// The highlight overlay: selection, timing paths, DRC markers, route guides
// and label text, drawn by the server on a transparent tile.
export function buildOverlayRequest({ z, x, y }, dpr, tileSize, visibility, app) {
    const req = {
        type: 'overlay_tile',
        z, x, y,
        ...tileSizeFields(dpr, tileSize),
        debug_renderers: !!visibility.debug_renderers,
        debug_live: !!visibility.debug_live,
        flywires_only: !!visibility.flywires_only,
        poly_decomp: !!(app && app.polyDecomp),
        focused_nets_guides: !!visibility.focused_nets_guides,
        highlight_selected: visibility.highlight_selected !== false,
        draw_labels: visibility.labels !== false,
    };
    if (app && app.visibleLayerNames) {
        req.visible_layers = [...app.visibleLayerNames];
    }
    return req;
}

// The active heat map, or no request at all when none is shown.
export function buildHeatmapRequest({ z, x, y }, dpr, tileSize, app) {
    const name = app && app.activeHeatMap;
    if (!name) {
        return null;
    }
    return { type: 'heatmap_tile', name, z, x, y,
             ...tileSizeFields(dpr, tileSize) };
}
