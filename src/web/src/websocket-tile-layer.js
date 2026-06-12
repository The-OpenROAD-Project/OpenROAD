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
    function buildTileRequest(coords, layerName, fillPattern, fillColor) {
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
            // Per-layer fill pattern index (see FillPattern in color.h):
            // 1 = solid (default), 0 = kNone/outline-only, 2.. = hatch styles.
            pattern: fillPattern | 0,
            visible_layers: visibleLayers ? [...visibleLayers] : [],
            selectable_layers: selectableLayers ? [...selectableLayers] : [],
            ...vf,
        };
        // Per-layer color override (RGB) from the Layer Config dialog.  Omitted
        // when unset so the server uses its default getLayerColorMap color.
        if (Array.isArray(fillColor) && fillColor.length >= 3) {
            req.layer_color = [fillColor[0] | 0, fillColor[1] | 0, fillColor[2] | 0];
        }
        if (app && app.visibleChiplets instanceof Set) {
            req.visible_chiplets = [...app.visibleChiplets];
        }
        return req;
    }
    return L.GridLayer.extend({
        initialize: function(websocketManager, layerName, options) {
            this._websocketManager = websocketManager;
            this._layerName = layerName;
            // Per-layer fill pattern index (FillPattern in color.h).  Defaults
            // to 1 (solid) — note 0 means "outline only" (kNone), so we must
            // not let an absent option collapse to 0.
            this._fillPattern = (options && options.fillPattern != null)
                ? (options.fillPattern | 0)
                : 1;
            // Per-layer color override (RGB array) or null to use the server
            // default color.
            this._fillColor = (options && Array.isArray(options.fillColor))
                ? options.fillColor
                : null;
            L.GridLayer.prototype.initialize.call(this, options);
        },

        // Change the layer's fill pattern and re-render its tiles in place.
        setFillPattern: function(fillPattern) {
            this._fillPattern = fillPattern | 0;
            this.refreshTiles();
        },

        // Change the layer's color override (RGB array, or null to reset to
        // the server default) and re-render its tiles in place.
        setFillColor: function(rgb) {
            this._fillColor = Array.isArray(rgb) ? rgb : null;
            this.refreshTiles();
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
                buildTileRequest(coords, this._layerName, this._fillPattern,
                                 this._fillColor)
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
                    buildTileRequest(coords, this._layerName, this._fillPattern,
                                     this._fillColor)
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
