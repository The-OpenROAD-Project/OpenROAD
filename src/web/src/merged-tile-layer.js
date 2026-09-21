// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// A Leaflet tile layer that composites several tech layers into ONE tile.
//
// The viewer's default is one pane per (tech layer x chiplet instance) — ~97 on
// a multi-die design — and every pane holds a full grid of tiles, each decoding
// to (256*dpr)^2 * 4 bytes. That measured 582 MB against a browser ceiling of
// ~458 MB, and over the ceiling Chrome discards decoded images: the discarded
// regions paint white and stay white until something forces a full
// invalidation. The symptom tracks decoded image bytes, not pane count: at a
// matched byte level, 86 and 97 content-bearing panes fail identically.
//
// This layer owns a contiguous run of the z-order and rasterizes it into a
// single <canvas> per tile position, so N panes hold N grids instead of 97.
// See tile-merge.js for the budget arithmetic, why the run must be contiguous,
// and why the pane's own opacity must be 1.

import {
    closeAll, mergeOntoContext, mergedPaneOptions, renderMergedTile,
} from './tile-merge.js';
import {
    buildTileRequestFor, floorClampZoom, nativeDpr, tileDevicePx,
    withDeviceExactTileSize,
} from './tile-request.js';

// Decode a websocket tile payload into something drawable.
//
// createImageBitmap is the point: an <img> gives no control over when its
// decoded bitmap is released, and that is the whole problem this layer exists
// to solve. An ImageBitmap can be close()d the moment it has been drawn.
//
// A string payload is a data URI from the static-report cache, which has no
// Blob to hand to createImageBitmap — fall back to an <img> there. Those
// reports are pre-rendered at one zoom and are not where the memory problem
// lives, so the weaker path is acceptable.
export function decodeTilePayload(payload) {
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
    if (typeof createImageBitmap === 'function') {
        return createImageBitmap(payload).catch(() => null);
    }
    // No createImageBitmap (very old browser): fall back to a blob URL, and
    // revoke it once the decode has happened.
    return new Promise((resolve) => {
        const url = URL.createObjectURL(payload);
        const img = new Image();
        img.onload = () => {
            URL.revokeObjectURL(url);
            resolve(img);
        };
        img.onerror = () => {
            URL.revokeObjectURL(url);
            resolve(null);
        };
        img.src = url;
    });
}

// `items` is the ordered draw list this pane owns: [{ layer, opacity, visible }],
// bottom-most first. It is read live on every request, so a visibility toggle
// only has to flip `visible` and call refreshTiles() — no layer is added to or
// removed from the map, and the pane count never changes.
export function createMergedTileLayer(ctx, options = {}) {
    const decode = options.decode || decodeTilePayload;
    const dprOf = options.dpr || nativeDpr;

    return L.GridLayer.extend({
        // Force upscale-only display (see floorClampZoom) so the browser never
        // downscales a band-limited tile back into a moiré beat.
        _clampZoom: function(zoom) {
            return L.GridLayer.prototype._clampZoom.call(
                this, floorClampZoom(this, zoom));
        },

        initialize: function(websocketManager, items, opts) {
            this._websocketManager = websocketManager;
            this._items = items || [];
            // Bumped on every refresh so in-flight renders for the previous
            // generation can tell they have been superseded and drop their work
            // instead of painting it.
            this._generation = 0;
            L.GridLayer.prototype.initialize.call(
                this, withDeviceExactTileSize(opts));
        },

        // Swap the draw list (a layer toggled, reordered, or regrouped after a
        // resize changed the budget) and repaint in place.
        setItems: function(items) {
            this._items = items || [];
            this.refreshTiles();
        },

        visibleItems: function() {
            return this._items.filter(item => item && item.visible !== false);
        },

        createTile: function(coords, done) {
            const canvas = document.createElement('canvas');
            canvas.setAttribute('role', 'presentation');
            const dpr = dprOf();
            this._sizeCanvas(canvas, dpr);
            // Stored on the element rather than held only by this render, so
            // whichever render paints first can complete the handshake.  A
            // refresh that lands before the first paint replaces the render but
            // not the tile, and Leaflet keeps the tile hidden until done() is
            // called — so a done that belonged to the superseded render would
            // leave a painted canvas invisible until the tile was recreated.
            canvas._orDone = done;
            this._renderTile(canvas, coords, dpr);
            return canvas;
        },

        // Backing store in device pixels, CSS box in layout pixels, so the
        // canvas maps 1:1 onto the device grid and drawImage does not resample
        // — the same invariant the <img> path relies on.
        //
        // Re-applied on every render, not just at creation: devicePixelRatio
        // changes when the window moves between monitors or the browser zoom
        // changes, and a refresh then requests images at the new dpr.  A canvas
        // still sized for the old one would scale every incoming tile into the
        // wrong backing store, leaving existing tiles blurry and inconsistent
        // with any created afterwards.
        _sizeCanvas: function(canvas, dpr) {
            const size = this.getTileSize();
            // The same count the request asks the server for, so the image
            // arrives at exactly the backing store's size and drawImage is a
            // straight blit.
            const w = tileDevicePx(size.x, dpr);
            const h = tileDevicePx(size.y, dpr);
            if (canvas.width !== w || canvas.height !== h) {
                // Assigning width/height also clears the canvas, which is what
                // we want here — the caller is about to redraw it.
                canvas.width = w;
                canvas.height = h;
            }
            canvas.style.width = size.x + 'px';
            canvas.style.height = size.y + 'px';
        },

        // Mark the tile loaded, once.  Leaflet reveals it only here
        // (.leaflet-tile { visibility: hidden } until done() adds
        // leaflet-tile-loaded), so every exit from a render has to reach this.
        _finishTile: function(canvas, err) {
            if (canvas._orTileDone || !canvas._orDone) {
                return;
            }
            canvas._orTileDone = true;
            canvas._orDone(err || null, canvas);
        },

        _renderTile: function(canvas, coords, dpr) {
            this._sizeCanvas(canvas, dpr);
            const generation = this._generation;
            const items = this.visibleItems();
            const context = canvas.getContext('2d');
            // Request ids are tracked per tile so _removeTile can cancel the
            // whole group's in-flight work when the tile is pruned.
            canvas._orRequestIds = [];

            // Superseded by a later refresh, or the tile was pruned while the
            // requests were in flight.  Pruning is tracked with an explicit flag
            // rather than canvas.isConnected: Leaflet appends the element AFTER
            // createTile returns, so attachment state conflates "pruned" with
            // "not yet attached" and would drop the very first paint of every
            // tile.
            const isStale = () => this._generation !== generation
                                  || canvas._orRemoved === true;

            renderMergedTile({
                items,
                request: (item) => {
                    const id = this._websocketManager.nextId;
                    canvas._orRequestIds.push(id);
                    return this._websocketManager.request(
                        buildTileRequestFor(coords, item.layer, ctx, dpr,
                                            this.getTileSize().x));
                },
                decode,
                draw: (sources) => mergeOntoContext(context, sources,
                                                    canvas.width),
                release: closeAll,
                isStale,
                // Reported on the FIRST paint, not on completion: Leaflet keeps
                // a tile hidden (.leaflet-tile { visibility: hidden }) until
                // done() adds leaflet-tile-loaded, so deferring it would make
                // every incremental paint invisible and the tile would still
                // appear only once its slowest layer had landed.
                onFirstDraw: () => this._finishTile(canvas),
            }).then(() => {
                // A superseded render must not finish the tile: it painted
                // nothing, so revealing the canvas here would show the state it
                // had before the refresh.  The render that replaced it owns the
                // handshake.
                if (!isStale()) {
                    this._finishTile(canvas);
                }
            }).catch((err) => {
                // Reported as a load error so Leaflet stops waiting on it; the
                // canvas keeps whatever it managed to paint.
                if (!isStale()) {
                    this._finishTile(canvas, err);
                }
            });
        },

        // Re-request every tile in place (no removal, no flash). Used for
        // visibility toggles, pattern changes and the server's design-changed
        // refresh push — the same contract as the per-layer layer's method, so
        // redrawAllLayers() drives both without knowing which it has.
        refreshTiles: function() {
            if (!this._map) {
                return;
            }
            this._generation++;
            for (const key in this._tiles) {
                const info = this._tiles[key];
                if (!info || !info.el) {
                    continue;
                }
                this._cancelTile(info.el);
                const dpr = dprOf();
                this._renderTile(info.el, info.coords, dpr);
            }
        },

        _cancelTile: function(el) {
            const ids = el._orRequestIds;
            if (!ids || !ids.length) {
                return;
            }
            for (const id of ids) {
                this._websocketManager.cancel(id);
            }
            el._orRequestIds = [];
        },

        _removeTile: function(key) {
            const tile = this._tiles[key];
            if (tile && tile.el) {
                // Flagged before the cancel so an in-flight render that is
                // already past its request phase still sees the tile as gone
                // and declines to paint a detached canvas.
                tile.el._orRemoved = true;
                this._cancelTile(tile.el);
                // Drop the backing store now rather than waiting for GC: a
                // pruned tile that keeps its canvas is the same unbounded
                // growth in a different costume.
                tile.el.width = 0;
                tile.el.height = 0;
            }
            L.GridLayer.prototype._removeTile.call(this, key);
        },
    });
}

// Build the N merged panes for an ordered draw list and put them on the map.
//
// `groups` is the output of partitionIntoGroups(): contiguous runs of the
// z-order, bottom-most first. z-indices are assigned from the group index so
// the panes stack in the same order the individual layers used to.
export function buildMergedPanes(MergedLayer, websocketManager, groups, map,
                                 baseZIndex = 0) {
    const panes = [];
    groups.forEach((items, i) => {
        const layer = new MergedLayer(
            websocketManager, items,
            mergedPaneOptions({ zIndex: baseZIndex + i }));
        if (map) {
            layer.addTo(map);
        }
        panes.push(layer);
    });
    return panes;
}
