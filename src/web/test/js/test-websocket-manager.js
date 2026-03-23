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
});
