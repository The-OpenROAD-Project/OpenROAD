// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// WebSocket manager with request/response tracking and auto-reconnect.
// Supports a cache mode for static reports where pre-computed responses
// are served from window.__STATIC_CACHE__ without a WebSocket connection.

// Flow control. Rapidly changing the selection/zoom can enqueue thousands of
// tile requests; sending them all at once floods the socket's send buffer and
// can wedge the connection (the server stops draining, the view freezes). We
// instead keep only a bounded number of requests on the wire at a time and
// hold the rest in a client-side queue where they can still be cancelled.
//
// The window counts SENT-but-not-yet-answered requests, decremented only when
// the server replies (resolved or stale) — never on cancel. Cancelling stops
// us tracking a reply but does not recall the bytes, so it must not free wire
// capacity; otherwise cancellation churn re-floods the socket. This bounds the
// send rate to the server's actual response rate.
//
// The effective limit is announced by the server on connect (a {type:"config",
// max_in_flight} push, scaled to the machine size) so it isn't a magic client
// constant and can grow if the server becomes multi-threaded. This is only the
// fallback used until that message arrives.
const DEFAULT_MAX_IN_FLIGHT = 48;
// Hard backpressure cap: don't push more while this much is still unsent. With
// the in-flight cap this should rarely bind.
const MAX_BUFFERED_BYTES = 512 * 1024;
// Liveness detection — two independent signatures of a wedged connection, both
// chosen so a legitimately slow operation never trips them:
//   1. The send buffer is stuck above STUCK_BYTES and not draining for
//      STUCK_MS — the browser can't even hand bytes to the OS. A slow render
//      keeps the buffer near 0.
//   2. We are saturated (in-flight at the cap) with work still queued, yet have
//      heard nothing back for DEAD_MS. A slow render is one slow reply among
//      fast ones, so replies keep flowing; total silence while saturated and
//      backed up means the server stopped servicing us entirely. This only ever
//      arms during a request flood (the window is full with more queued) — a
//      lone slow command (e.g. a timing query that triggers an STA update)
//      never saturates the window, so it cannot trip this. DEAD_MS is also set
//      well beyond any plausible single render so a saturated-but-slow burst
//      can't be mistaken for a dead server.
// On either, we force-close the socket so onclose reconnects and recovers.
const STUCK_BYTES = 256 * 1024;
const STUCK_MS = 12000;
const DEAD_MS = 30000;
const LIVENESS_INTERVAL_MS = 3000;

export class WebSocketManager {
    constructor(url, onStatusChange) {
        this.url = url;
        this.socket = null;
        this.nextId = 1;
        this.pending = new Map(); // id -> {resolve, reject} — sent, still tracked
        // Requests waiting for an in-flight slot (insertion-ordered = FIFO).
        // Not yet on the wire, so cancel() can drop them for free.
        this._queue = new Map(); // id -> {msg, resolve, reject}
        // Requests sent but not yet answered by the server (the real wire
        // window). Incremented on send, decremented when ANY reply for a sent
        // id arrives — even a stale one for a cancelled request.
        this._inFlight = 0;
        this._maxInFlight = DEFAULT_MAX_IN_FLIGHT; // updated by server "config"
        this._lastRecvAt = 0;     // perf.now() of the last message of any kind
        this._bufStuckSince = 0;  // liveness: when bufferedAmount got stuck
        this._lastBuffered = 0;
        this.reconnectDelay = 1000;
        this.readyPromise = null;
        this.readyResolve = null;
        this.onStatusChange = onStatusChange || (() => {});
        this.onPush = null; // callback for server-push notifications
        this._cache = null;
        this._isConnected = false;
        this._shutdown = false; // set by server "shutdown" message
        this._livenessTimer = undefined; // set by _startLivenessMonitor
        this._startLivenessMonitor();
        this.connect();
    }

    get isConnected() {
        return this._isConnected || (this.socket && this.socket.readyState === WebSocket.OPEN);
    }

    // Total requests the user is still waiting on: those on the wire plus those
    // held back by the scheduler and not yet dispatched.
    get pendingCount() {
        return this.pending.size + (this._queue ? this._queue.size : 0);
    }

    // Create a cache-backed instance (no WebSocket connection).
    static fromCache(cache, onStatusChange) {
        const mgr = Object.create(WebSocketManager.prototype);
        mgr.url = null;
        mgr.socket = null;
        mgr.nextId = 1;
        mgr.pending = new Map();
        mgr._queue = new Map(); // unused in cache mode, but keeps cancel() safe
        mgr._inFlight = 0;
        mgr._maxInFlight = DEFAULT_MAX_IN_FLIGHT;
        mgr._lastRecvAt = 0;
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
            this._lastRecvAt = (typeof performance !== 'undefined')
                ? performance.now() : 0;
            this.readyResolve();
            this.onStatusChange();
            this._pump(); // flush anything queued before the socket opened
        };

        this.socket.onmessage = (event) => {
            this.handleMessage(event.data);
        };

        this.socket.onclose = () => this._handleClose();

        this.socket.onerror = (err) => {
            console.error('WebSocket error:', err);
        };
    }

    // Tear down after the socket is gone (rejecting outstanding work) and
    // schedule a reconnect unless the server told us to shut down. Invoked by
    // socket.onclose and, for a wedged socket, directly by _forceReconnect().
    _handleClose() {
        this._isConnected = false;
        this._inFlight = 0;
        this._bufStuckSince = 0;
        this.onStatusChange();
        for (const [id, handler] of this.pending) {
            handler.reject(new Error('WebSocket closed'));
        }
        this.pending.clear();
        // Drop anything still queued — it never reached the wire. Reject so
        // callers settle; tile callers .catch() and re-request on reconnect.
        for (const [id, entry] of this._queue) {
            entry.reject(new Error('WebSocket closed'));
        }
        this._queue.clear();
        if (this._shutdown) {
            console.log('WebSocket closed (server stopped)');
            this._stopLivenessMonitor(); // no socket will reopen
            return; // don't reconnect after intentional shutdown
        }
        console.log('WebSocket closed, reconnecting...');
        setTimeout(() => this.connect(), this.reconnectDelay);
        this.reconnectDelay = Math.min(this.reconnectDelay * 2, 30000);
    }

    // Abandon a wedged socket and reconnect now, without waiting for close()
    // to fire onclose. When the send buffer is stuck, close() only starts its
    // handshake after the already-buffered bytes drain (which they aren't), so
    // onclose may not arrive until the TCP timeout — minutes away. Detach the
    // dead socket's handlers so it can no longer drive our state, run the same
    // teardown onclose would, then let _handleClose() schedule the reconnect.
    _forceReconnect() {
        const dead = this.socket;
        this.socket = null;
        if (dead) {
            dead.onopen = null;
            dead.onmessage = null;
            dead.onclose = null;
            dead.onerror = null;
            try {
                dead.close(); // best effort; we no longer depend on it
            } catch (e) {
                /* already gone */
            }
        }
        this._handleClose();
    }

    handleMessage(data) {
        // Binary frame: [4B id][1B type][3B reserved][payload...]
        const view = new DataView(data);
        const id = view.getUint32(0);   // big-endian
        const type = view.getUint8(4);  // 0=JSON, 1=PNG, 2=error

        this._lastRecvAt = (typeof performance !== 'undefined')
            ? performance.now() : 0;

        // id=0 is a server-push notification (not a response to a request).
        // It still counts as liveness (above) but frees no in-flight slot.
        if (id === 0 && type === 0) {
            const payload = data.slice(8);
            let msg;
            try {
                msg = JSON.parse(new TextDecoder().decode(payload));
            } catch (e) {
                // A malformed push must not break the message loop.
                console.error('Failed to parse server push JSON:', e);
                return;
            }
            // The server announces the in-flight window sized to its machine.
            // Consume it here rather than forwarding to onPush.
            if (msg && msg.type === 'config') {
                if (Number.isFinite(msg.max_in_flight)) {
                    this._maxInFlight = Math.max(
                        1, Math.min(4096, Math.floor(msg.max_in_flight)));
                    this._pump(); // a larger window may let more go out now
                }
                return;
            }
            if (this.onPush) {
                this.onPush(msg);
            }
            return;
        }

        // Any reply for a sent id frees a wire slot — including a stale reply
        // for a request we already cancelled. This keeps the scheduler moving
        // and bounds the send rate to the server's response rate.
        if (this._inFlight > 0) {
            this._inFlight--;
        }

        const handler = this.pending.get(id);
        if (!handler) {
            this._pump(); // a slot freed even though we no longer track it
            return; // stale response (e.g. tile scrolled away / cancelled)
        }
        this.pending.delete(id);
        this._pump(); // let the next queued request go out
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
        // Reject immediately when disconnected (preserves prior behaviour and
        // avoids the queue growing during an outage). Callers .catch() and
        // re-issue on reconnect.
        if (!this.socket || this.socket.readyState !== WebSocket.OPEN) {
            return Promise.reject(new Error('WebSocket not connected'));
        }
        let resolveFn, rejectFn;
        const promise = new Promise((resolve, reject) => {
            resolveFn = resolve;
            rejectFn = reject;
        });
        promise.requestId = id;
        // Enqueue rather than send directly; _pump() releases it onto the wire
        // once an in-flight slot is available. While queued it is cancellable.
        this._queue.set(id, { msg, resolve: resolveFn, reject: rejectFn });
        this._pump();
        return promise;
    }

    // Release queued requests onto the socket up to the in-flight cap, backing
    // off if the send buffer is already deep. Only sent requests enter
    // `pending`; queued ones can still be cancelled for free.
    _pump() {
        if (!this.socket || this.socket.readyState !== WebSocket.OPEN) {
            return;
        }
        while (this._inFlight < this._maxInFlight
               && this._queue.size > 0
               && this.socket.bufferedAmount < MAX_BUFFERED_BYTES) {
            const [id, entry] = this._queue.entries().next().value;
            this._queue.delete(id);
            this.pending.set(id, {
                resolve: entry.resolve,
                reject: entry.reject,
            });
            this.socket.send(JSON.stringify(entry.msg));
            this._inFlight++;
        }
        this.onStatusChange();
    }

    cancel(id) {
        // If still queued it never hit the wire — drop it for free (no server
        // work, no buffered bytes). This is the cancellation that actually
        // matters; cancelling an already-sent request can't recall the bytes.
        // Settle the promise so callers don't leak a forever-pending request.
        const queued = this._queue.get(id);
        if (queued) {
            this._queue.delete(id);
            queued.reject(new Error('Request cancelled'));
            this.onStatusChange();
            return;
        }
        // Already sent: stop tracking the reply, but do NOT free the wire slot
        // or pump — the server still has to process those bytes, and the slot
        // frees only when its (now stale) reply arrives. Freeing it here would
        // let cancellation churn re-flood the socket.
        this.pending.delete(id);
        this.onStatusChange();
    }

    // Liveness: a 3s timer that force-closes a wedged socket so onclose
    // reconnects. Both signatures are guarded so a slow operation never trips
    // them (see the constant definitions above).
    _startLivenessMonitor() {
        if (typeof setInterval === 'undefined') {
            return;
        }
        this._livenessTimer = setInterval(() => this._checkLiveness(),
                                          LIVENESS_INTERVAL_MS);
        // Don't keep a Node process alive for this timer (no-op in browsers).
        if (this._livenessTimer
            && typeof this._livenessTimer.unref === 'function') {
            this._livenessTimer.unref();
        }
    }

    // Stop the liveness timer. Called on intentional shutdown so the interval
    // doesn't keep firing _checkLiveness against a socket we'll never reopen.
    _stopLivenessMonitor() {
        if (this._livenessTimer !== undefined
            && typeof clearInterval !== 'undefined') {
            clearInterval(this._livenessTimer);
            this._livenessTimer = undefined;
        }
    }

    _checkLiveness() {
        if (this._cache) {
            return;
        }
        const s = this.socket;
        if (!s || s.readyState !== WebSocket.OPEN) {
            this._bufStuckSince = 0;
            this._lastBuffered = 0;
            return;
        }
        const buf = s.bufferedAmount;
        const now = (typeof performance !== 'undefined') ? performance.now() : 0;

        // Signature 1: send buffer stuck and not draining.
        let wedged = false;
        let why = '';
        if (buf <= STUCK_BYTES || buf < this._lastBuffered) {
            this._bufStuckSince = 0;
        } else if (this._bufStuckSince === 0) {
            this._bufStuckSince = now;
        } else if (now - this._bufStuckSince > STUCK_MS) {
            wedged = true;
            why = `send buffer stuck at ${buf}B for `
                + `${Math.round(now - this._bufStuckSince)}ms`;
        }
        this._lastBuffered = buf;

        // Signature 2: saturated with work queued, yet silent for DEAD_MS.
        if (!wedged
            && this._inFlight >= this._maxInFlight
            && this._queue.size > 0
            && this._lastRecvAt > 0
            && now - this._lastRecvAt > DEAD_MS) {
            wedged = true;
            why = `saturated (${this._inFlight} in flight, ${this._queue.size} `
                + `queued) but silent for ${Math.round(now - this._lastRecvAt)}ms`;
        }

        if (wedged) {
            console.warn(`WebSocket connection wedged — ${why}; `
                + `reconnecting to recover`);
            this._bufStuckSince = 0;
            // Don't depend on close()/onclose: a stuck send buffer can defer
            // the close handshake indefinitely. Reconnect immediately.
            this._forceReconnect();
        }
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

        // set_module_colors is a no-op in static mode (tiles are pre-rendered).
        if (type === 'set_module_colors') {
            return Promise.resolve({ok: 1, count: 0});
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
