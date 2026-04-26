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
        // Auto-fire onopen so the manager considers itself connected.
        queueMicrotask(() => { if (this.onopen) this.onopen(); });
    }
    send(data) { this.sent.push(data); }
    close() {}
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

    it('returns setup timing report when is_setup=1', async () => {
        const mgr = WebSocketManager.fromCache(makeCache());
        const result = await mgr.request({ type: 'timing_report', is_setup: 1 });
        assert.equal(result.paths[0].slack, -0.1);
    });

    it('returns hold timing report when is_setup=0', async () => {
        const mgr = WebSocketManager.fromCache(makeCache());
        const result = await mgr.request({ type: 'timing_report', is_setup: 0 });
        assert.equal(result.paths[0].slack, 0.2);
    });

    it('returns setup histogram', async () => {
        const mgr = WebSocketManager.fromCache(makeCache());
        const result = await mgr.request({ type: 'slack_histogram', is_setup: 1 });
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
        await mgr.request({ type: 'timing_highlight', path_index: 0, is_setup: 1 });
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
            const result = await mgr.request({ type: 'timing_report', is_setup: 1 });
            assert.equal(result.paths.length, 5);
            // No _originalIndex annotation when not filtering.
            assert.equal(result.paths[0]._originalIndex, undefined);
        });

        it('filters paths to the [slack_min, slack_max) range', async () => {
            const mgr = WebSocketManager.fromCache(makeCacheWithPaths());
            const result = await mgr.request({
                type: 'timing_report',
                is_setup: 1,
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
                is_setup: 1,
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
                is_setup: 1,
                slack_min: 10.0,
                slack_max: 20.0,
            });
            assert.equal(result.paths.length, 0);
        });

        it('filters the hold side independently of setup', async () => {
            const mgr = WebSocketManager.fromCache(makeCacheWithPaths());
            const result = await mgr.request({
                type: 'timing_report',
                is_setup: 0,
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
                is_setup: 1,
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
