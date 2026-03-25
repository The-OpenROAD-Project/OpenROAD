// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// WebSocket manager with request/response tracking and auto-reconnect.

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
        this.connect();
    }

    connect() {
        this.readyPromise = new Promise(resolve => {
            this.readyResolve = resolve;
        });

        this.socket = new WebSocket(this.url);
        this.socket.binaryType = 'arraybuffer';

        this.socket.onopen = () => {
            console.log('WebSocket connected');
            this.reconnectDelay = 1000;
            this.readyResolve();
        };

        this.socket.onmessage = (event) => {
            this.handleMessage(event.data);
        };

        this.socket.onclose = () => {
            console.log('WebSocket closed, reconnecting...');
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
}
