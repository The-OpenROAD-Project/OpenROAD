// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Leaflet tile layer that fetches tiles via WebSocket.

export function createWebSocketTileLayer(visibility) {
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

            const vf = {};
            for (const [k, v] of Object.entries(visibility)) {
                vf[k] = v ? 1 : 0;
            }
            this._websocketManager.request({
                type: 'tile',
                layer: this._layerName,
                z: coords.z,
                x: coords.x,
                y: coords.y,
                ...vf,
            }).then(blob => {
                tile.src = URL.createObjectURL(blob);
            }).catch(() => {
                // Request was cancelled (e.g. by refreshTiles); ignore
            });

            return tile;
        },

        // Re-request all existing tiles in place (no removal/flash).
        // Use this instead of redraw() for visibility changes.
        refreshTiles: function() {
            if (!this._map) return;

            const vf = {};
            for (const [k, v] of Object.entries(visibility)) {
                vf[k] = v ? 1 : 0;
            }

            for (const key in this._tiles) {
                const tileInfo = this._tiles[key];
                if (!tileInfo || !tileInfo.el) continue;

                const tile = tileInfo.el;
                const coords = tileInfo.coords;

                // Cancel any pending request for this tile
                if (tile._websocketRequestId !== undefined) {
                    this._websocketManager.cancel(tile._websocketRequestId);
                }

                const requestId = this._websocketManager.nextId;
                tile._websocketRequestId = requestId;

                this._websocketManager.request({
                    type: 'tile',
                    layer: this._layerName,
                    z: coords.z,
                    x: coords.x,
                    y: coords.y,
                    ...vf,
                }).then(blob => {
                    if (tile.src && tile.src.startsWith('blob:')) {
                        URL.revokeObjectURL(tile.src);
                    }
                    tile.src = URL.createObjectURL(blob);
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
