// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Leaflet tile layer that fetches tiles via WebSocket.

export function createWSTileLayer(visibility) {
    return L.GridLayer.extend({
        initialize: function(wsManager, layerName, options) {
            this._wsManager = wsManager;
            this._layerName = layerName;
            L.GridLayer.prototype.initialize.call(this, options);
        },

        createTile: function(coords, done) {
            const tile = document.createElement('img');
            tile.alt = '';
            tile.setAttribute('role', 'presentation');

            const requestId = this._wsManager.nextId;
            tile._wsRequestId = requestId;

            const vf = {};
            for (const [k, v] of Object.entries(visibility)) {
                vf[k] = v ? 1 : 0;
            }
            this._wsManager.request({
                type: 'tile',
                layer: this._layerName,
                z: coords.z,
                x: coords.x,
                y: coords.y,
                ...vf,
            }).then(blob => {
                tile.onload = () => {
                    URL.revokeObjectURL(tile.src);
                    done(null, tile);
                };
                tile.onerror = () => {
                    done(new Error('tile image load error'), tile);
                };
                tile.src = URL.createObjectURL(blob);
            }).catch(err => {
                done(err, tile);
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
                if (tile._wsRequestId !== undefined) {
                    this._wsManager.cancel(tile._wsRequestId);
                }

                const requestId = this._wsManager.nextId;
                tile._wsRequestId = requestId;

                this._wsManager.request({
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
                if (tile.el._wsRequestId !== undefined) {
                    this._wsManager.cancel(tile.el._wsRequestId);
                }
                if (tile.el.src && tile.el.src.startsWith('blob:')) {
                    URL.revokeObjectURL(tile.el.src);
                }
            }
            L.GridLayer.prototype._removeTile.call(this, key);
        }
    });
}
