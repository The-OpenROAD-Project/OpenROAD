// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Leaflet tile layer that fetches tiles via WebSocket.

// `app` (last arg) is read lazily on every request so that
// app.visibleChiplets, populated by display-controls.js after the tech
// metadata arrives, is reflected in tile requests without rebuilding
// the layer.  When null or absent the field is omitted and the server
// renders every chiplet (default).
export function createWebSocketTileLayer(visibility, visibleLayers,
                                         selectability, selectableLayers,
                                         app) {
    // Single source of truth for the tile-request payload.  Both
    // createTile (initial load) and refreshTiles (visibility change)
    // call this so the wire format stays in sync — earlier copies of
    // this snippet drifted when fields like visible_chiplets were
    // added in only one place.
    //
    // Tiles don't actually use selectability for rendering, but we
    // send it on every request so the wire schema stays uniform with
    // selectAt requests.
    function buildTileRequest(coords, layerName) {
        const vf = {};
        for (const [k, v] of Object.entries(visibility)) {
            vf[k] = !!v;
        }
        if (selectability) {
            for (const [k, v] of Object.entries(selectability)) {
                vf['s_' + k] = !!v;
            }
        }
        const req = {
            type: 'tile',
            layer: layerName,
            z: coords.z,
            x: coords.x,
            y: coords.y,
            visible_layers: visibleLayers ? [...visibleLayers] : [],
            selectable_layers: selectableLayers ? [...selectableLayers] : [],
            ...vf,
        };
        if (app && app.visibleChiplets instanceof Set) {
            req.visible_chiplets = [...app.visibleChiplets];
        }
        return req;
    }
    return L.GridLayer.extend({
        initialize: function(websocketManager, layerName, options) {
            this._websocketManager = websocketManager;
            this._layerName = layerName;
            L.GridLayer.prototype.initialize.call(this, options);
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

            // Store the request ID so _removeTile() can cancel it
            // when the tile is discarded (e.g. during zoom).
            tile._websocketRequestId = this._websocketManager.nextId;

            this._websocketManager.request(
                buildTileRequest(coords, this._layerName)
            ).then(data => {
                if (typeof data === 'string') {
                    tile.src = data;  // data URI from cache
                } else {
                    tile.src = URL.createObjectURL(data);
                }
            }).catch(() => {
                // Request was cancelled (e.g. by refreshTiles); ignore
            });

            return tile;
        },

        // Re-request all existing tiles in place (no removal/flash).
        // Use this instead of redraw() for visibility changes.
        refreshTiles: function() {
            if (!this._map) return;

            for (const key in this._tiles) {
                const tileInfo = this._tiles[key];
                if (!tileInfo || !tileInfo.el) continue;

                const tile = tileInfo.el;
                const coords = tileInfo.coords;

                // Cancel any pending request for this tile
                if (tile._websocketRequestId !== undefined) {
                    this._websocketManager.cancel(tile._websocketRequestId);
                }

                tile._websocketRequestId = this._websocketManager.nextId;

                this._websocketManager.request(
                    buildTileRequest(coords, this._layerName)
                ).then(data => {
                    if (tile.src && tile.src.startsWith('blob:')) {
                        URL.revokeObjectURL(tile.src);
                    }
                    if (typeof data === 'string') {
                        tile.src = data;
                    } else {
                        tile.src = URL.createObjectURL(data);
                    }
                }).catch(() => {
                    // Tile refresh failed; keep existing image
                });
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
    function buildOverlayRequest(coords) {
        const req = {
            type: 'overlay_tile',
            z: coords.z,
            x: coords.x,
            y: coords.y,
            debug_renderers: !!visibility.debug_renderers,
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
            L.GridLayer.prototype.initialize.call(this, options);
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
                buildOverlayRequest(coords)
            ).then(data => {
                if (tile._websocketRequestId !== requestId) {
                    return;  // stale response; a newer request superseded this one
                }
                if (typeof data === 'string') {
                    tile.src = data;
                } else {
                    tile.src = URL.createObjectURL(data);
                }
            }).catch(() => {});

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
                    buildOverlayRequest(coords)
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
