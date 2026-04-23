// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// WebSocket manager with request/response tracking and auto-reconnect.
// Supports a cache mode for static reports where pre-computed responses
// are served from window.__STATIC_CACHE__ without a WebSocket connection.

export class WebSocketManager {
    constructor(url, onStatusChange) {
        this.url = url;
        this.socket = null;
        this.nextId = 1;
        this.pending = new Map(); // id -> {resolve, reject}
        this.reconnectDelay = 1000;
        this.readyPromise = null;
        this.readyResolve = null;
        this.onStatusChange = onStatusChange || (() => {});
        this.onPush = null; // callback for server-push notifications
        this._cache = null;
        this._isConnected = false;
        this.connect();
    }

    get isConnected() {
        return this._isConnected || (this.socket && this.socket.readyState === WebSocket.OPEN);
    }

    // Create a cache-backed instance (no WebSocket connection).
    static fromCache(cache, onStatusChange) {
        const mgr = Object.create(WebSocketManager.prototype);
        mgr.url = null;
        mgr.socket = null;
        mgr.nextId = 1;
        mgr.pending = new Map();
        mgr.reconnectDelay = 0;
        mgr.onStatusChange = onStatusChange || (() => {});
        mgr.onPush = null;
        mgr._cache = cache;
        mgr._isConnected = true; // cache is always "connected"
        mgr.readyPromise = Promise.resolve();
        mgr.readyResolve = null;
        return mgr;
    }

    get isStaticMode() {
        return !!this._cache;
    }

    connect() {
        this.readyPromise = new Promise(resolve => {
            this.readyResolve = resolve;
        });

        this.socket = new WebSocket(this.url);
        this.socket.binaryType = 'arraybuffer';

        this.socket.onopen = () => {
            console.log('WebSocket connected');
            this._isConnected = true;
            this.reconnectDelay = 1000;
            this.readyResolve();
            this.onStatusChange();
        };

        this.socket.onmessage = (event) => {
            this.handleMessage(event.data);
        };

        this.socket.onclose = () => {
            console.log('WebSocket closed, reconnecting...');
            this._isConnected = false;
            this.onStatusChange();
            for (const [id, handler] of this.pending) {
                handler.reject(new Error('WebSocket closed'));
            }
            this.pending.clear();
            setTimeout(() => this.connect(), this.reconnectDelay);
            this.reconnectDelay = Math.min(this.reconnectDelay * 2, 30000);
        };

        this.socket.onerror = (err) => {
            console.error('WebSocket error:', err);
        };
    }

    handleMessage(data) {
        // Binary frame: [4B id][1B type][3B reserved][payload...]
        const view = new DataView(data);
        const id = view.getUint32(0);   // big-endian
        const type = view.getUint8(4);  // 0=JSON, 1=PNG, 2=error

        // id=0 is a server-push notification (not a response to a request)
        if (id === 0 && type === 0) {
            const payload = data.slice(8);
            const msg = JSON.parse(new TextDecoder().decode(payload));
            if (this.onPush) {
                this.onPush(msg);
            }
            return;
        }

        const handler = this.pending.get(id);
        if (!handler) {
            return; // stale response (e.g. tile scrolled away)
        }
        this.pending.delete(id);
        this.onStatusChange();

        const payload = data.slice(8);

        if (type === 2) {
            handler.reject(new Error(new TextDecoder().decode(payload)));
        } else if (type === 0) {
            try {
                handler.resolve(JSON.parse(new TextDecoder().decode(payload)));
            } catch (e) {
                handler.reject(new Error("JSON parse error: " + e.message));
            }
        } else if (type === 1) {
            handler.resolve(new Blob([payload], { type: 'image/png' }));
        }
    }

    request(msg) {
        if (this._cache) {
            return this._cacheRequest(msg);
        }
        const id = this.nextId++;
        msg.id = id;
        const promise = new Promise((resolve, reject) => {
            if (!this.socket || this.socket.readyState !== WebSocket.OPEN) {
                reject(new Error('WebSocket not connected'));
                return;
            }
            this.pending.set(id, { resolve, reject });
            this.socket.send(JSON.stringify(msg));
            this.onStatusChange();
        });
        promise.requestId = id;
        return promise;
    }

    cancel(id) {
        this.pending.delete(id);
        this.onStatusChange();
    }

    // Serve a request from the embedded cache.
    _cacheRequest(msg) {
        const type = msg.type;

        // Tile requests — return a data URI string.
        if (type === 'tile') {
            const key = msg.layer + '/' + msg.z + '/' + msg.x + '/' + msg.y;
            const b64 = this._cache.tiles && this._cache.tiles[key];
            if (b64) {
                return Promise.resolve('data:image/png;base64,' + b64);
            }
            return Promise.reject(new Error('Tile not cached: ' + key));
        }

        // Timing highlight — drive the overlay image.
        if (type === 'timing_highlight') {
            const idx = msg.path_index;
            if (idx >= 0 && this._cache.overlays) {
                const side = msg.is_setup ? 'setup' : 'hold';
                const b64 = this._cache.overlays[side]?.[idx];
                if (this._cache.setPathOverlay) {
                    this._cache.setPathOverlay(
                        b64 ? 'data:image/png;base64,' + b64 : null);
                }
            } else if (this._cache.setPathOverlay) {
                this._cache.setPathOverlay(null);
            }
            return Promise.resolve({});
        }

        // Parameterized JSON lookups.
        let key = type;
        if (type === 'timing_report') {
            key = 'timing_report:' + (msg.is_setup ? 'setup' : 'hold');
        } else if (type === 'slack_histogram') {
            key = 'slack_histogram:' + (msg.is_setup ? 'setup' : 'hold');
        }
        const json = this._cache.json && this._cache.json[key];
        if (json !== undefined) {
            // Filter timing paths by slack range when histogram column clicked.
            // Tag each path with its original index so overlay lookup still
            // works after the array is narrowed down.
            if (type === 'timing_report'
                && msg.slack_min != null && msg.slack_max != null
                && json.paths) {
                const filtered = [];
                json.paths.forEach((p, i) => {
                    if (p.slack >= msg.slack_min && p.slack < msg.slack_max) {
                        filtered.push({ ...p, _originalIndex: i });
                    }
                });
                return Promise.resolve({ ...json, paths: filtered });
            }
            return Promise.resolve(json);
        }

        return Promise.reject(new Error('Not available in static mode: ' + type));
    }
}
