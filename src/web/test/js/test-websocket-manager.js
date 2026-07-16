// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';

// Mock WebSocket before importing the module.
class MockWebSocket {
    constructor() {
        this.readyState = 1;
        this.sent = [];
        this.binaryType = null;
        this.bufferedAmount = 0; // scheduler reads this for backpressure
        this.closeCount = 0;     // liveness tests assert force-close
        // Auto-fire onopen so the manager considers itself connected.
        queueMicrotask(() => { if (this.onopen) this.onopen(); });
    }
    send(data) { this.sent.push(data); }
    close() { this.closeCount++; this.readyState = 2; }
    static get OPEN() { return 1; }
}
globalThis.WebSocket = MockWebSocket;

const { WebSocketManager } = await import('../../src/websocket-manager.js');

// Helper: build a binary response frame.
// Format: [4B id (big-endian)][1B type][3B reserved][payload]
function buildFrame(id, type, payload) {
    const buf = new ArrayBuffer(8 + payload.length);
    const view = new DataView(buf);
    view.setUint32(0, id);
    view.setUint8(4, type);
    new Uint8Array(buf, 8).set(payload);
    return buf;
}

// Helper: ids of requests actually sent on the socket, in order.
function sentIds(mgr) {
    return mgr.socket.sent.map(s => JSON.parse(s).id);
}

// Helper: deliver a (trivial) JSON reply for a given request id.
function deliverReply(mgr, id) {
    mgr.handleMessage(buildFrame(id, 0, new TextEncoder().encode('{}')));
}

describe('WebSocketManager', () => {
    describe('handleMessage', () => {
        it('resolves JSON response', async () => {
            const mgr = new WebSocketManager('ws://fake');
            const promise = new Promise((resolve, reject) => {
                mgr.pending.set(42, { resolve, reject });
            });
            const json = JSON.stringify({ status: 'ok', value: 123 });
            const payload = new TextEncoder().encode(json);
            mgr.handleMessage(buildFrame(42, 0, payload));
            const result = await promise;
            assert.deepEqual(result, { status: 'ok', value: 123 });
        });

        it('resolves PNG response as Blob', async () => {
            // Blob is not available in Node 18 test runner without setup,
            // but handleMessage uses `new Blob(...)` which exists in Node 18.
            const mgr = new WebSocketManager('ws://fake');
            const promise = new Promise((resolve, reject) => {
                mgr.pending.set(7, { resolve, reject });
            });
            const pngData = new Uint8Array([0x89, 0x50, 0x4E, 0x47]);
            mgr.handleMessage(buildFrame(7, 1, pngData));
            const blob = await promise;
            assert.equal(blob.type, 'image/png');
            assert.equal(blob.size, 4);
        });

        it('rejects error response', async () => {
            const mgr = new WebSocketManager('ws://fake');
            const promise = new Promise((resolve, reject) => {
                mgr.pending.set(99, { resolve, reject });
            });
            const errMsg = new TextEncoder().encode('something failed');
            mgr.handleMessage(buildFrame(99, 2, errMsg));
            await assert.rejects(promise, { message: 'something failed' });
        });

        it('ignores unknown request IDs', () => {
            const mgr = new WebSocketManager('ws://fake');
            // Should not throw.
            const payload = new TextEncoder().encode('{}');
            mgr.handleMessage(buildFrame(999, 0, payload));
            assert.equal(mgr.pending.size, 0);
        });

        it('survives a malformed server-push payload', () => {
            const mgr = new WebSocketManager('ws://fake');
            let pushed = false;
            mgr.onPush = () => { pushed = true; };
            // id=0 push frame with invalid JSON must not throw or call onPush.
            const bad = new TextEncoder().encode('{ not json');
            assert.doesNotThrow(() => mgr.handleMessage(buildFrame(0, 0, bad)));
            assert.equal(pushed, false, 'no push delivered for malformed JSON');
        });
    });

    describe('request', () => {
        it('assigns incrementing IDs', async () => {
            const mgr = new WebSocketManager('ws://fake');
            await mgr.readyPromise;
            const startId = mgr.nextId;
            mgr.request({ type: 'test1' });
            mgr.request({ type: 'test2' });
            assert.equal(mgr.nextId, startId + 2);
        });

        it('rejects when socket not connected', async () => {
            const mgr = new WebSocketManager('ws://fake');
            mgr.socket.readyState = 3; // CLOSED
            await assert.rejects(
                mgr.request({ type: 'test' }),
                { message: 'WebSocket not connected' }
            );
        });
    });

    describe('cancel', () => {
        it('removes pending request', async () => {
            const mgr = new WebSocketManager('ws://fake');
            await mgr.readyPromise;
            mgr.pending.set(5, { resolve: () => {}, reject: () => {} });
            assert.equal(mgr.pending.has(5), true);
            mgr.cancel(5);
            assert.equal(mgr.pending.has(5), false);
        });
    });

    describe('connection state', () => {
        it('isConnected is true after socket opens', async () => {
            const mgr = new WebSocketManager('ws://fake');
            await mgr.readyPromise;
            assert.equal(mgr.isConnected, true);
        });

        it('isConnected is false after socket closes', async () => {
            const mgr = new WebSocketManager('ws://fake');
            await mgr.readyPromise;
            assert.equal(mgr.isConnected, true);
            // Simulate socket close
            mgr.socket.readyState = 3; // CLOSED
            mgr.socket.onclose?.();
            assert.equal(mgr.isConnected, false);
        });

        it('onStatusChange is called when socket opens', async () => {
            let statusChangedCount = 0;
            const mgr = new WebSocketManager('ws://fake', () => {
                statusChangedCount++;
            });
            await mgr.readyPromise;
            assert.ok(statusChangedCount > 0, 'onStatusChange should be called on open');
        });

        it('onStatusChange is called when socket closes', async () => {
            let statusChangedCount = 0;
            const mgr = new WebSocketManager('ws://fake', () => {
                statusChangedCount++;
            });
            await mgr.readyPromise;
            const countAfterOpen = statusChangedCount;
            // Simulate socket close
            mgr.socket.readyState = 3; // CLOSED
            mgr.socket.onclose?.();
            assert.ok(statusChangedCount > countAfterOpen, 'onStatusChange should be called on close');
        });
    });
});

// Regression tests for the request flood that locked up the web view when the
// user rapidly changed selection/zoom: the client used to send every request
// immediately, flooding the socket send buffer and wedging the connection.
describe('WebSocketManager flow control', () => {
    it('caps in-flight requests and queues the rest', async () => {
        const mgr = new WebSocketManager('ws://fake');
        await mgr.readyPromise;

        const N = 200;
        for (let i = 0; i < N; i++) {
            mgr.request({ type: 'tile', n: i }).catch(() => {});
        }

        // The pre-fix code sent all N at once; the scheduler must hold the
        // excess back in the queue.
        assert.ok(mgr.socket.sent.length > 0, 'some requests should be sent');
        assert.ok(mgr.socket.sent.length < N, 'must not send all at once');
        assert.equal(mgr._inFlight, mgr.socket.sent.length,
            'in-flight count tracks what was sent');
        assert.equal(mgr._queue.size, N - mgr.socket.sent.length,
            'the rest are queued');
        // The status indicator must reflect EVERYTHING outstanding, including
        // requests still queued and not yet dispatched to the server.
        assert.equal(mgr.pendingCount, N,
            'pendingCount counts in-flight + queued');
    });

    it('honours the server-announced in-flight limit', async () => {
        const mgr = new WebSocketManager('ws://fake');
        await mgr.readyPromise;

        // Server announces a small window via a config push.
        const cfg = new TextEncoder().encode(
            JSON.stringify({ type: 'config', max_in_flight: 4 }));
        mgr.handleMessage(buildFrame(0, 0, cfg));
        assert.equal(mgr._maxInFlight, 4);

        for (let i = 0; i < 200; i++) {
            mgr.request({ type: 'tile', n: i }).catch(() => {});
        }
        assert.equal(mgr.socket.sent.length, 4,
            'sends only up to the announced window');
        assert.equal(mgr._inFlight, 4);
        assert.equal(mgr._queue.size, 196);
    });

    it('releases queued requests as replies arrive', async () => {
        const mgr = new WebSocketManager('ws://fake');
        await mgr.readyPromise;

        const N = 200;
        for (let i = 0; i < N; i++) {
            mgr.request({ type: 'tile', n: i }).catch(() => {});
        }
        const firstBurst = mgr.socket.sent.length;
        const ids = sentIds(mgr);

        // Answer 10 in-flight requests; each freed slot pulls one off the queue.
        for (let i = 0; i < 10; i++) {
            deliverReply(mgr, ids[i]);
        }
        assert.equal(mgr.socket.sent.length, firstBurst + 10,
            'each reply releases exactly one queued request');
        assert.equal(mgr._inFlight, firstBurst, 'stays at the cap');
        assert.equal(mgr._queue.size, N - firstBurst - 10);
    });

    it('cancelling an in-flight request does not free a wire slot', async () => {
        const mgr = new WebSocketManager('ws://fake');
        await mgr.readyPromise;

        const N = 200;
        for (let i = 0; i < N; i++) {
            mgr.request({ type: 'tile', n: i }).catch(() => {});
        }
        const firstBurst = mgr.socket.sent.length;
        const ids = sentIds(mgr);

        // Simulate every in-flight tile scrolling out of view at once.
        for (const id of ids) {
            mgr.cancel(id);
        }

        // The critical regression: cancelling already-sent requests must NOT
        // pump new ones (the bytes are committed; the server still has to
        // process them). Doing so re-floods the socket — the original bug.
        assert.equal(mgr.socket.sent.length, firstBurst,
            'cancel must not trigger new sends');
        assert.equal(mgr._inFlight, firstBurst,
            'cancel must not free wire slots');

        // The slots free only when the (now stale) replies actually arrive.
        for (const id of ids) {
            deliverReply(mgr, id);
        }
        assert.equal(mgr._inFlight, firstBurst, 'refilled to the cap');
        assert.equal(mgr.socket.sent.length, firstBurst * 2,
            'replies — not cancels — drive the next burst');
    });

    it('cancelling a queued request drops it before it is sent', async () => {
        const mgr = new WebSocketManager('ws://fake');
        await mgr.readyPromise;

        const N = 200;
        const ids = [];
        for (let i = 0; i < N; i++) {
            const p = mgr.request({ type: 'tile', n: i });
            p.catch(() => {});
            ids.push(p.requestId);
        }
        const sentBefore = mgr.socket.sent.length;
        const queuedBefore = mgr._queue.size;

        // Cancel a request that is still queued (one of the last enqueued).
        const queuedId = ids[N - 1];
        assert.ok(mgr._queue.has(queuedId));
        mgr.cancel(queuedId);

        assert.equal(mgr._queue.size, queuedBefore - 1, 'removed from queue');
        assert.equal(mgr.socket.sent.length, sentBefore,
            'a queued cancel sends nothing');
    });

    it('cancelling a queued request settles its promise', async () => {
        const mgr = new WebSocketManager('ws://fake');
        await mgr.readyPromise;

        // Saturate the wire so the next request stays queued.
        const ids = [];
        for (let i = 0; i < 200; i++) {
            const p = mgr.request({ type: 'tile', n: i });
            p.catch(() => {});
            ids.push(p.requestId);
        }
        const queuedId = ids[ids.length - 1];
        assert.ok(mgr._queue.has(queuedId), 'last request is queued');

        // Re-issue a tracked promise for the queued id so we can observe it.
        const entry = mgr._queue.get(queuedId);
        const observed = new Promise((resolve, reject) => {
            entry.resolve = resolve;
            entry.reject = reject;
        });

        mgr.cancel(queuedId);
        // The promise must reject — not hang forever — so callers clean up.
        await assert.rejects(observed, { message: 'Request cancelled' });
    });
});

describe('WebSocketManager liveness', () => {
    it('force-closes when saturated and silent', async () => {
        const mgr = new WebSocketManager('ws://fake');
        await mgr.readyPromise;

        // Advance the clock 100s so "silence" exceeds the dead threshold
        // deterministically (performance.now() is tiny this early in a process).
        const realNow = performance.now.bind(performance);
        const t0 = realNow();
        performance.now = () => t0 + 100000;
        try {
            // Saturated (in-flight at/above the cap), work still queued, and no
            // reply since t0 — i.e. far longer than the dead threshold.
            mgr._inFlight = 1000;
            mgr._queue.set(99999, { msg: {}, resolve() {}, reject() {} });
            mgr._lastRecvAt = t0;

            const dead = mgr.socket;
            const before = dead.closeCount;
            mgr._checkLiveness();
            assert.equal(dead.closeCount, before + 1,
                'a wedged connection is closed');
            assert.equal(dead.onclose, null,
                'the wedged socket is detached so it cannot drive our state');
            assert.notEqual(mgr.socket, dead,
                'we reconnect without waiting for the dead socket to close');
        } finally {
            performance.now = realNow;
        }
    });

    it('does not close an idle (non-saturated) socket', async () => {
        const mgr = new WebSocketManager('ws://fake');
        await mgr.readyPromise;

        // Silent for a long time, but nothing outstanding — this is just idle,
        // not wedged, and a slow lone operation must not be killed.
        mgr._inFlight = 0;
        mgr.socket.bufferedAmount = 0;
        mgr._lastRecvAt = performance.now() - 60000;

        const before = mgr.socket.closeCount;
        mgr._checkLiveness();
        assert.equal(mgr.socket.closeCount, before, 'idle socket left alone');
    });

    it('does not kill a lone slow command (e.g. a timing/STA update)',
       async () => {
        // Emulates a server query that triggers an actual STA update: a single
        // request is on the wire, nothing is queued behind it, and the server
        // is silent for far longer than DEAD_MS while it computes. This must
        // NOT be treated as a dead connection — a lone slow command never
        // saturates the in-flight window (so Signature 2 cannot setup), and its
        // small payload never fills the send buffer (so Signature 1 cannot
        // setup). Only a request flood that saturates the window is a wedge.
        const mgr = new WebSocketManager('ws://fake');
        await mgr.readyPromise;

        const realNow = performance.now.bind(performance);
        const t0 = realNow();
        try {
            // Issue one request; _pump() puts it on the wire and we await the
            // reply. .catch() so the never-settled promise isn't flagged.
            mgr.request({ type: 'timing_report' }).catch(() => {});
            assert.equal(mgr._inFlight, 1, 'one request in flight');
            assert.equal(mgr._queue.size, 0, 'nothing queued behind it');
            // Bytes left the client (the server is busy, not the transport).
            mgr.socket.bufferedAmount = 0;
            mgr._lastRecvAt = t0;

            const dead = mgr.socket;
            const before = dead.closeCount;

            // 120s later the STA update still hasn't replied — way past DEAD_MS.
            performance.now = () => t0 + 120000;
            mgr._checkLiveness();

            assert.equal(mgr.socket, dead, 'connection not abandoned');
            assert.equal(dead.closeCount, before,
                'a lone slow command is not mistaken for a dead connection');
        } finally {
            performance.now = realNow;
        }
    });

    it('force-closes when the send buffer is stuck', async () => {
        const mgr = new WebSocketManager('ws://fake');
        await mgr.readyPromise;

        const realNow = performance.now.bind(performance);
        const t0 = realNow();
        try {
            // Buffer above threshold and not draining across successive checks.
            const dead = mgr.socket;
            dead.bufferedAmount = 4 * 1024 * 1024;
            const before = dead.closeCount;

            performance.now = () => t0;       // first check arms the stuck timer
            mgr._checkLiveness();
            assert.equal(dead.closeCount, before,
                'not yet — needs to persist');

            performance.now = () => t0 + 100000; // 100s later, still not draining
            mgr._checkLiveness();
            assert.equal(dead.closeCount, before + 1,
                'a non-draining send buffer is treated as wedged');
            assert.notEqual(mgr.socket, dead,
                'we reconnect without waiting for the dead socket to close');
        } finally {
            performance.now = realNow;
        }
    });

    it('stops the liveness timer on intentional shutdown', async () => {
        const mgr = new WebSocketManager('ws://fake');
        await mgr.readyPromise;
        assert.notEqual(mgr._livenessTimer, undefined,
            'timer runs while connected');

        // Server-initiated shutdown: the socket closes and must not reconnect,
        // so the recurring liveness check must be cleared too.
        mgr._shutdown = true;
        mgr.socket.readyState = 3; // CLOSED
        mgr.socket.onclose?.();
        assert.equal(mgr._livenessTimer, undefined,
            'liveness timer cleared after shutdown');
    });
});

describe('WebSocketManager.fromCache', () => {
    function makeCache(overrides = {}) {
        return {
            zoom: 1,
            json: {
                tech: { layers: ['M1'], sites: [], has_liberty: false, dbu_per_micron: 1000 },
                bounds: { bounds: [[0, 0], [100, 100]], shapes_ready: true },
                heatmaps: { active: '', heatmaps: [] },
                'timing_report:setup': { paths: [{ slack: -0.1 }] },
                'timing_report:hold': { paths: [{ slack: 0.2 }] },
                'slack_histogram:setup': { bins: [], total_endpoints: 0 },
                chart_filters: { path_groups: [], clocks: [] },
            },
            tiles: {
                'M1/1/0/0': 'iVBORw0KGgo=',
            },
            overlays: {
                setup: ['base64data'],
                hold: [null],
            },
            setPathOverlay: null,
            ...overrides,
        };
    }

    it('creates instance without WebSocket', () => {
        const mgr = WebSocketManager.fromCache(makeCache());
        assert.equal(mgr.isStaticMode, true);
        assert.equal(mgr.socket, null);
    });

    it('readyPromise resolves immediately', async () => {
        const mgr = WebSocketManager.fromCache(makeCache());
        await mgr.readyPromise; // Should not hang.
    });

    it('isConnected is true for cache-backed instances', () => {
        const mgr = WebSocketManager.fromCache(makeCache());
        assert.equal(mgr.isConnected, true);
    });

    it('returns cached JSON for tech', async () => {
        const cache = makeCache();
        const mgr = WebSocketManager.fromCache(cache);
        const result = await mgr.request({ type: 'tech' });
        assert.deepEqual(result.layers, ['M1']);
    });

    it('returns cached JSON for bounds', async () => {
        const cache = makeCache();
        const mgr = WebSocketManager.fromCache(cache);
        const result = await mgr.request({ type: 'bounds' });
        assert.equal(result.shapes_ready, true);
    });

    it('returns setup timing report when is_setup=true', async () => {
        const mgr = WebSocketManager.fromCache(makeCache());
        const result = await mgr.request({ type: 'timing_report', is_setup: true });
        assert.equal(result.paths[0].slack, -0.1);
    });

    it('returns hold timing report when is_setup=false', async () => {
        const mgr = WebSocketManager.fromCache(makeCache());
        const result = await mgr.request({ type: 'timing_report', is_setup: false });
        assert.equal(result.paths[0].slack, 0.2);
    });

    it('returns setup histogram', async () => {
        const mgr = WebSocketManager.fromCache(makeCache());
        const result = await mgr.request({ type: 'slack_histogram', is_setup: true });
        assert.equal(result.total_endpoints, 0);
    });

    it('returns chart_filters', async () => {
        const mgr = WebSocketManager.fromCache(makeCache());
        const result = await mgr.request({ type: 'chart_filters' });
        assert.ok(Array.isArray(result.path_groups));
    });

    it('rejects uncached request types', async () => {
        const mgr = WebSocketManager.fromCache(makeCache());
        await assert.rejects(
            mgr.request({ type: 'select' }),
            { message: /Not available in static mode/ }
        );
    });

    it('returns data URI string for cached tile', async () => {
        const mgr = WebSocketManager.fromCache(makeCache());
        const result = await mgr.request({
            type: 'tile', layer: 'M1', z: 1, x: 0, y: 0
        });
        assert.equal(typeof result, 'string');
        assert.ok(result.startsWith('data:image/png;base64,'));
    });

    it('rejects uncached tile', async () => {
        const mgr = WebSocketManager.fromCache(makeCache());
        await assert.rejects(
            mgr.request({ type: 'tile', layer: 'M1', z: 1, x: 9, y: 9 }),
            { message: /Tile not cached/ }
        );
    });

    it('timing_highlight calls setPathOverlay with data URI', async () => {
        let called = null;
        const cache = makeCache();
        cache.setPathOverlay = (v) => { called = v; };
        const mgr = WebSocketManager.fromCache(cache);
        await mgr.request({ type: 'timing_highlight', path_index: 0, is_setup: true });
        assert.ok(called !== null);
        assert.ok(called.startsWith('data:image/png;base64,'));
    });

    it('timing_highlight with index -1 clears overlay', async () => {
        let called = 'not_called';
        const cache = makeCache();
        cache.setPathOverlay = (v) => { called = v; };
        const mgr = WebSocketManager.fromCache(cache);
        await mgr.request({ type: 'timing_highlight', path_index: -1 });
        assert.equal(called, null);
    });

    describe('timing_report slack filtering', () => {
        function makeCacheWithPaths() {
            return makeCache({
                json: {
                    tech: { layers: [], sites: [], has_liberty: false, dbu_per_micron: 1000 },
                    bounds: { bounds: [[0, 0], [100, 100]], shapes_ready: true },
                    heatmaps: { active: '', heatmaps: [] },
                    'timing_report:setup': {
                        paths: [
                            { slack: -0.3, end_pin: 'a' },
                            { slack: -0.1, end_pin: 'b' },
                            { slack: 0.0,  end_pin: 'c' },
                            { slack: 0.1,  end_pin: 'd' },
                            { slack: 0.25, end_pin: 'e' },
                        ],
                    },
                    'timing_report:hold': {
                        paths: [
                            { slack: 0.05, end_pin: 'h0' },
                            { slack: 0.15, end_pin: 'h1' },
                        ],
                    },
                    'slack_histogram:setup': { bins: [], total_endpoints: 0 },
                    chart_filters: { path_groups: [], clocks: [] },
                },
            });
        }

        it('returns all paths when slack range is not provided', async () => {
            const mgr = WebSocketManager.fromCache(makeCacheWithPaths());
            const result = await mgr.request({ type: 'timing_report', is_setup: true });
            assert.equal(result.paths.length, 5);
            // No _originalIndex annotation when not filtering.
            assert.equal(result.paths[0]._originalIndex, undefined);
        });

        it('filters paths to the [slack_min, slack_max) range', async () => {
            const mgr = WebSocketManager.fromCache(makeCacheWithPaths());
            const result = await mgr.request({
                type: 'timing_report',
                is_setup: true,
                slack_min: -0.1,
                slack_max: 0.1,
            });
            // -0.1 and 0.0 are in range; 0.1 is exclusive upper bound.
            assert.deepEqual(
                result.paths.map(p => p.end_pin),
                ['b', 'c']);
        });

        it('tags filtered paths with original index for overlay lookup', async () => {
            const mgr = WebSocketManager.fromCache(makeCacheWithPaths());
            const result = await mgr.request({
                type: 'timing_report',
                is_setup: true,
                slack_min: -0.1,
                slack_max: 0.1,
            });
            assert.equal(result.paths[0].end_pin, 'b');
            assert.equal(result.paths[0]._originalIndex, 1);
            assert.equal(result.paths[1].end_pin, 'c');
            assert.equal(result.paths[1]._originalIndex, 2);
        });

        it('returns empty paths when no slack falls in range', async () => {
            const mgr = WebSocketManager.fromCache(makeCacheWithPaths());
            const result = await mgr.request({
                type: 'timing_report',
                is_setup: true,
                slack_min: 10.0,
                slack_max: 20.0,
            });
            assert.equal(result.paths.length, 0);
        });

        it('filters the hold side independently of setup', async () => {
            const mgr = WebSocketManager.fromCache(makeCacheWithPaths());
            const result = await mgr.request({
                type: 'timing_report',
                is_setup: false,
                slack_min: 0.1,
                slack_max: 0.2,
            });
            assert.equal(result.paths.length, 1);
            assert.equal(result.paths[0].end_pin, 'h1');
            assert.equal(result.paths[0]._originalIndex, 1);
        });

        it('does not mutate the cached paths array', async () => {
            const cache = makeCacheWithPaths();
            const mgr = WebSocketManager.fromCache(cache);
            await mgr.request({
                type: 'timing_report',
                is_setup: true,
                slack_min: -0.1,
                slack_max: 0.1,
            });
            const cached = cache.json['timing_report:setup'].paths;
            assert.equal(cached.length, 5);
            // Original cached objects must not be tagged.
            for (const p of cached) {
                assert.equal(p._originalIndex, undefined);
            }
        });
    });
});
