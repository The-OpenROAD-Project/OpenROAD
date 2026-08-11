// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Leaflet tile layer that fetches tiles via WebSocket.

import {
    buildTileRequestFor, floorClampZoom, nativeDpr, tileSizeCss,
    tileSizeFields, withDeviceExactTileSize,
} from './tile-request.js';

// Imported AND re-exported: `export { x } from '...'` alone re-exports without
// creating a local binding, so _clampZoom below would reference an undefined
// name at runtime while importers still resolved it fine.
export { floorClampZoom };

// The display's real device pixel ratio.  Sizes are computed from this;
// the wire payload rounds it for its cache-key field (see tile-request.js).
export function currentDpr() {
    return nativeDpr();
}

// Kept as a named export because main.js and the tests import it from here.
// Delegates so the per-layer and merged paths cannot drift on the wire format.
export function buildTileRequest(coords, layerName, ctx) {
    return buildTileRequestFor(coords, layerName, ctx, currentDpr(),
                               tileSizeCss());
}

// A 1x1 transparent PNG.  Stands in for a tile the server would render empty
// anyway (see the `gate` option below): no round trip, and the decoded bitmap
// is one pixel instead of (256*dpr)².
const EMPTY_TILE
    = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAAC0lEQVR4nGMAAQAABQABDQottAAAAABJRU5ErkJggg==';

// `app` (last arg) is read lazily on every request so that
// app.visibleChiplets, populated by display-controls.js after the tech
// metadata arrives, is reflected in tile requests without rebuilding
// the layer.  When null or absent the field is omitted and the server
// renders every chiplet (default).
export function createWebSocketTileLayer(visibility, visibleLayers,
                                         selectability, selectableLayers,
                                         app) {
    const ctx = {
        visibility, selectability, visibleLayers, selectableLayers, app,
    };
    return L.GridLayer.extend({
        // Force upscale-only display (see floorClampZoom) so the browser
        // never downscales a band-limited tile back into a moiré beat.
        _clampZoom: function(zoom) {
            return L.GridLayer.prototype._clampZoom.call(
                this, floorClampZoom(this, zoom));
        },

        initialize: function(websocketManager, layerName, options) {
            this._websocketManager = websocketManager;
            this._layerName = layerName;
            L.GridLayer.prototype.initialize.call(
                this, withDeviceExactTileSize(options));
        },

        // A layer whose content is switched by one visibility flag can be left
        // mounted (which is what keeps the toggle honest — the map's layer set
        // is not a second copy of the flag) without paying for it: while the
        // flag is off the server would render an empty tile, so don't ask.
        // The flag is named by whoever creates the layer, via `options.gate`.
        // A name that is not a real visibility flag would read undefined and
        // gate the layer off forever, with nothing on screen and no error to
        // explain it; fail open and say so instead.
        _gatedOff: function() {
            const gate = this.options.gate;
            if (gate == null) return false;
            if (!(gate in ctx.visibility)) {
                if (!this._gateWarned) {
                    this._gateWarned = true;
                    console.warn(`tile layer ${this._layerName}: unknown gate `
                                 + `'${gate}', rendering ungated`);
                }
                return false;
            }
            return !ctx.visibility[gate];
        },

        // Point a tile at a new image, releasing the blob the old one held.
        // Every src assignment goes through here so the revoke cannot be
        // forgotten on one of the paths.
        _setTileSrc: function(tile, src) {
            if (tile.src && tile.src.startsWith('blob:')) {
                URL.revokeObjectURL(tile.src);
            }
            tile.src = src;
        },

        // Ask the server for this tile and show whatever comes back.  Shared by
        // createTile and refreshTiles: the request, the id bookkeeping that lets
        // it be cancelled, and the cache's data-URI-vs-blob answer are the same
        // in both.
        _requestTile: function(tile, coords) {
            tile._websocketRequestId = this._websocketManager.nextId;
            this._websocketManager.request(
                buildTileRequest(coords, this._layerName, ctx)
            ).then(data => {
                this._setTileSrc(
                    tile,
                    typeof data === 'string' ? data : URL.createObjectURL(data));
            }).catch(err => {
                // Cancelled (by refreshTiles) or failed.  `done` is never called
                // and no retry is issued, so a tile that never loaded stays blank
                // until Leaflet evicts it, and one that had an image keeps it.
            });
        },

        createTile: function(coords, done) {
            const tile = document.createElement('img');
            tile.alt = '';
            tile.setAttribute('role', 'presentation');

            // Set up onload/onerror BEFORE any src assignment so that
            // refreshTiles() can set tile.src and still trigger done().
            tile._tileDone = false;
            tile.onload = () => {
                if (tile.src && tile.src.startsWith('blob:')) {
                    URL.revokeObjectURL(tile.src);
                }
                if (!tile._tileDone) {
                    tile._tileDone = true;
                    done(null, tile);
                }
            };
            tile.onerror = () => {
                if (!tile._tileDone) {
                    tile._tileDone = true;
                    done(new Error('tile image load error'), tile);
                }
            };

            if (this._gatedOff()) {
                tile.src = EMPTY_TILE;
                return tile;
            }

            // The request id stored by _requestTile is what lets _removeTile()
            // cancel a tile discarded before it arrives (e.g. during zoom).
            this._requestTile(tile, coords);
            return tile;
        },

        // Re-request all existing tiles in place (no removal/flash).
        // Use this instead of redraw() for visibility changes.
        refreshTiles: function() {
            if (!this._map) return;

            const gated = this._gatedOff();
            for (const key in this._tiles) {
                const tileInfo = this._tiles[key];
                if (!tileInfo || !tileInfo.el) continue;

                const tile = tileInfo.el;
                const coords = tileInfo.coords;

                // Cancel any pending request for this tile
                if (tile._websocketRequestId !== undefined) {
                    this._websocketManager.cancel(tile._websocketRequestId);
                    tile._websocketRequestId = undefined;
                }

                // Switched off: blank the tile in place.  Leaving the previous
                // image up would keep showing an overlay the user just turned
                // off, and asking the server for a tile it renders empty is a
                // round trip for 102 bytes.
                if (gated) {
                    // Only when it is not already blank: redrawAllLayers walks
                    // every layer on any visibility change, so an overlay that
                    // has been off the whole time would re-decode the same
                    // placeholder across its whole tile grid each time.
                    if (tile.src !== EMPTY_TILE) {
                        this._setTileSrc(tile, EMPTY_TILE);
                    }
                    continue;
                }

                this._requestTile(tile, coords);
            }
        },

        _removeTile: function(key) {
            const tile = this._tiles[key];
            if (tile && tile.el) {
                if (tile.el._websocketRequestId !== undefined) {
                    this._websocketManager.cancel(tile.el._websocketRequestId);
                }
                if (tile.el.src && tile.el.src.startsWith('blob:')) {
                    URL.revokeObjectURL(tile.el.src);
                }
            }
            L.GridLayer.prototype._removeTile.call(this, key);
        }
    });
}

// Lightweight tile layer that renders only highlight/overlay shapes
// (selection, hover, timing, DRC, route guides) on a transparent
// background.  Separated from the base tile layers so that highlight
// changes don't trigger a full re-render of all geometry tiles.
export function createOverlayTileLayer(visibility, app) {
    function buildOverlayRequest(coords, tileSize) {
        const req = {
            type: 'overlay_tile',
            z: coords.z,
            x: coords.x,
            y: coords.y,
            // Sized like the layer tiles it is drawn over: a highlight
            // rendered at a different pixel count is rescaled by the browser
            // and no longer sits exactly on the shape it highlights.
            ...tileSizeFields(currentDpr(), tileSize),
            debug_renderers: !!visibility.debug_renderers,
            flywires_only: !!visibility.flywires_only,
        };
        // Pass visible layers so route guides respect layer visibility.
        if (app && app.visibleLayerNames) {
            req.visible_layers = [...app.visibleLayerNames];
        }
        return req;
    }
    return L.GridLayer.extend({
        initialize: function(websocketManager, options) {
            this._websocketManager = websocketManager;
            L.GridLayer.prototype.initialize.call(
                this, withDeviceExactTileSize(options));
        },

        createTile: function(coords, done) {
            const tile = document.createElement('img');
            tile.alt = '';
            tile.setAttribute('role', 'presentation');

            tile._tileDone = false;
            tile.onload = () => {
                if (tile.src && tile.src.startsWith('blob:')) {
                    URL.revokeObjectURL(tile.src);
                }
                if (!tile._tileDone) {
                    tile._tileDone = true;
                    done(null, tile);
                }
            };
            tile.onerror = () => {
                if (!tile._tileDone) {
                    tile._tileDone = true;
                    done(new Error('overlay tile load error'), tile);
                }
            };

            const requestId = this._websocketManager.nextId;
            tile._websocketRequestId = requestId;

            this._websocketManager.request(
                buildOverlayRequest(coords, this.getTileSize().x)
            ).then(data => {
                if (tile._websocketRequestId !== requestId) {
                    return;  // stale response; a newer request superseded this one
                }
                if (typeof data === 'string') {
                    tile.src = data;
                } else {
                    tile.src = URL.createObjectURL(data);
                }
            }).catch(err => {
            });

            return tile;
        },

        refreshTiles: function() {
            if (!this._map) return;

            for (const key in this._tiles) {
                const tileInfo = this._tiles[key];
                if (!tileInfo || !tileInfo.el) continue;

                const tile = tileInfo.el;
                const coords = tileInfo.coords;

                if (tile._websocketRequestId !== undefined) {
                    this._websocketManager.cancel(tile._websocketRequestId);
                }

                const requestId = this._websocketManager.nextId;
                tile._websocketRequestId = requestId;

                this._websocketManager.request(
                    buildOverlayRequest(coords, this.getTileSize().x)
                ).then(data => {
                    if (tile._websocketRequestId !== requestId) {
                        return;  // stale response superseded by a newer refresh
                    }
                    if (tile.src && tile.src.startsWith('blob:')) {
                        URL.revokeObjectURL(tile.src);
                    }
                    if (typeof data === 'string') {
                        tile.src = data;
                    } else {
                        tile.src = URL.createObjectURL(data);
                    }
                }).catch(() => {});
            }
        },

        _removeTile: function(key) {
            const tile = this._tiles[key];
            if (tile && tile.el) {
                if (tile.el._websocketRequestId !== undefined) {
                    this._websocketManager.cancel(tile.el._websocketRequestId);
                    tile.el._websocketRequestId = undefined;
                }
                if (tile.el.src && tile.el.src.startsWith('blob:')) {
                    URL.revokeObjectURL(tile.el.src);
                }
            }
            L.GridLayer.prototype._removeTile.call(this, key);
        }
    });
}
