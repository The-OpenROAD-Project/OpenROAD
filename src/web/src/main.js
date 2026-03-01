// WebSocket manager - multiplexes all requests over a single connection
class WebSocketManager {
    constructor(url) {
        this.url = url;
        this.ws = null;
        this.nextId = 1;
        this.pending = new Map(); // id -> {resolve, reject}
        this.reconnectDelay = 1000;
        this.readyPromise = null;
        this.readyResolve = null;
        this.connect();
    }

    connect() {
        this.readyPromise = new Promise(resolve => {
            this.readyResolve = resolve;
        });

        this.ws = new WebSocket(this.url);
        this.ws.binaryType = 'arraybuffer';

        this.ws.onopen = () => {
            console.log('WebSocket connected');
            this.reconnectDelay = 1000;
            this.readyResolve();
        };

        this.ws.onmessage = (event) => {
            this.handleMessage(event.data);
        };

        this.ws.onclose = () => {
            console.log('WebSocket closed, reconnecting...');
            // Reject all pending requests
            for (const [id, handler] of this.pending) {
                handler.reject(new Error('WebSocket closed'));
            }
            this.pending.clear();
            setTimeout(() => this.connect(), this.reconnectDelay);
            this.reconnectDelay = Math.min(this.reconnectDelay * 2, 30000);
        };

        this.ws.onerror = (err) => {
            console.error('WebSocket error:', err);
        };
    }

    handleMessage(data) {
        // Binary frame: [4B id][1B type][3B reserved][payload...]
        const view = new DataView(data);
        const id = view.getUint32(0);   // big-endian
        const type = view.getUint8(4);  // 0=JSON, 1=PNG, 2=error

        const handler = this.pending.get(id);
        if (!handler) {
            return; // stale response (e.g. tile scrolled away)
        }
        this.pending.delete(id);

        const payload = data.slice(8);

        if (type === 2) {
            handler.reject(new Error(new TextDecoder().decode(payload)));
        } else if (type === 0) {
            handler.resolve(JSON.parse(new TextDecoder().decode(payload)));
        } else if (type === 1) {
            handler.resolve(new Blob([payload], { type: 'image/png' }));
        }
    }

    request(msg) {
        return new Promise((resolve, reject) => {
            if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
                reject(new Error('WebSocket not connected'));
                return;
            }
            const id = this.nextId++;
            msg.id = id;
            this.pending.set(id, { resolve, reject });
            this.ws.send(JSON.stringify(msg));
        });
    }

    // Cancel a pending request (e.g. tile scrolled out of view)
    cancel(id) {
        this.pending.delete(id);
    }
}

// Custom Leaflet TileLayer that loads tiles via WebSocket
const WSTileLayer = L.TileLayer.extend({
    initialize: function(wsManager, layerName, options) {
        this._wsManager = wsManager;
        this._layerName = layerName;
        // Track request IDs per tile key for cancellation
        this._pendingIds = new Map();
        L.TileLayer.prototype.initialize.call(this, '', options);
    },

    createTile: function(coords, done) {
        const tile = document.createElement('img');
        tile.alt = '';
        tile.setAttribute('role', 'presentation');

        const id = this._wsManager.nextId; // peek at next ID
        const key = coords.x + ':' + coords.y + ':' + coords.z;
        this._pendingIds.set(key, id);

        this._wsManager.request({
            type: 'tile',
            layer: this._layerName,
            z: coords.z,
            x: coords.x,
            y: coords.y
        }).then(blob => {
            tile.onload = () => {
                URL.revokeObjectURL(tile.src);
                done(null, tile);
            };
            tile.src = URL.createObjectURL(blob);
            this._pendingIds.delete(key);
        }).catch(err => {
            this._pendingIds.delete(key);
            done(err, tile);
        });

        return tile;
    },

    _removeTile: function(key) {
        // Cancel pending WebSocket request for tiles that scrolled away
        const id = this._pendingIds.get(key);
        if (id !== undefined) {
            this._wsManager.cancel(id);
            this._pendingIds.delete(key);
        }
        // Revoke blob URL if still set
        const tile = this._tiles[key];
        if (tile && tile.el && tile.el.src && tile.el.src.startsWith('blob:')) {
            URL.revokeObjectURL(tile.el.src);
        }
        L.TileLayer.prototype._removeTile.call(this, key);
    }
});

const map = L.map('map', {
    crs: L.CRS.Simple,
    zoom: 1,
    zoomSnap: 0.1,
    zoomDelta: 0.2,
});

const wsUrl = `ws://${window.location.hostname || 'localhost'}:8080/ws`;
const wsManager = new WebSocketManager(wsUrl);
let fitBounds = null;

wsManager.readyPromise.then(async () => {
    try {
        const [layersData, boundsData] = await Promise.all([
            wsManager.request({ type: 'layers' }),
            wsManager.request({ type: 'bounds' })
        ]);

        // --- Load Layers ---
        const overlayLayers = {};
        layersData.layers.forEach((name, index) => {
            const layer = new WSTileLayer(wsManager, name, {
                attribution: name,
                opacity: 0.7,
                zIndex: index + 1
            });
            overlayLayers[name] = layer;
            layer.addTo(map);
        });
        L.control.layers(null, overlayLayers, { collapsed: false }).addTo(map);

        // --- Set Bounds ---
        const designBounds = boundsData.bounds;

        const minY = designBounds[0][0];
        const minX = designBounds[0][1];
        const maxY = designBounds[1][0];
        const maxX = designBounds[1][1];

        const designWidth = maxX - minX;
        const designHeight = maxY - minY;
        const tileSize = 256;

        const scale = tileSize / Math.max(designWidth, designHeight);

        fitBounds = [
            [-minY * scale, minX * scale],
            [-maxY * scale, maxX * scale]
        ];
        map.fitBounds(fitBounds);
    } catch (err) {
        console.error('Failed to load initial data from server:', err);
        map.getPane('mapPane').innerHTML =
            '<div style="color:red; text-align:center; margin-top: 50px; font-family: monospace;">' +
            'Error: Could not load initial data from server.' +
            '</div>';
    }
});

document.addEventListener('keydown', (e) => {
    if (e.key === 'f' && fitBounds) {
        map.fitBounds(fitBounds);
    }
});
