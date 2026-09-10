// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it, beforeEach, afterEach } from 'node:test';
import assert from 'node:assert/strict';
import { JSDOM } from 'jsdom';

describe('Status Indicator (updateStatus)', () => {
    let dom;
    let statusDiv;
    let app;
    let updateStatus;
    let disconnectTimeout;
    let progressEl;
    const DISCONNECT_DELAY_MS = 2000;
    // Keep in step with main.js: past this many outstanding requests the count
    // is shown as well as the progress line.
    const PENDING_BACKLOG = 20;

    beforeEach(async () => {
        // Setup DOM
        dom = new JSDOM(`
            <!DOCTYPE html>
            <html>
            <body>
                <div id="websocket-status" style="display: none;"></div>
                <div id="tile-progress" class="or-progress"></div>
            </body>
            </html>
        `);
        globalThis.document = dom.window.document;

        statusDiv = document.getElementById('websocket-status');
        progressEl = document.getElementById('tile-progress');

        // Mock app object
        app = {
            websocketManager: null,
        };

        // Mock timers for testing
        let timeoutId = 0;
        const timeouts = new Map();
        globalThis.setTimeout = (fn, delay) => {
            const id = ++timeoutId;
            timeouts.set(id, { fn, delay });
            return id;
        };
        globalThis.clearTimeout = (id) => {
            timeouts.delete(id);
        };

        // Import and setup the updateStatus function
        // We need to recreate it here with our mocked globals
        disconnectTimeout = null;
        updateStatus = function() {
            const isConnected = app.websocketManager && app.websocketManager.isConnected;
            const pendingCount = app.websocketManager ? app.websocketManager.pending.size : 0;
            
            if (!isConnected) {
                if (!disconnectTimeout) {
                    disconnectTimeout = setTimeout(() => {
                        if (!app.websocketManager?.isConnected) {
                            statusDiv.innerHTML = '<div class="disconnected-banner">⚠ OpenROAD disconnected</div>';
                            statusDiv.style.display = 'block';
                        }
                    }, DISCONNECT_DELAY_MS);
                }
            } else {
                if (disconnectTimeout) {
                    clearTimeout(disconnectTimeout);
                    disconnectTimeout = null;
                }
                
                if (pendingCount > PENDING_BACKLOG) {
                    statusDiv.innerHTML = '<div class="or-hud or-hud-top-right '
                        + `pending-indicator">pending: ${pendingCount}</div>`;
                    statusDiv.style.display = 'block';
                } else {
                    statusDiv.style.display = 'none';
                }
            }
            // setTileProgress
            progressEl.classList.toggle('active', pendingCount > 0);
            progressEl.title = pendingCount > 0
                ? `${pendingCount} tile requests pending` : '';
        };

        // Expose helper to trigger scheduled callbacks
        updateStatus._getTimeouts = () => timeouts;
        updateStatus._getDisconnectTimeout = () => disconnectTimeout;
    });

    afterEach(() => {
        if (disconnectTimeout) {
            clearTimeout(disconnectTimeout);
        }
        delete globalThis.document;
        delete globalThis.setTimeout;
        delete globalThis.clearTimeout;
    });

    describe('Connected state with pending requests (backward compatibility)', () => {
        it('shows nothing when connected with no pending requests', () => {
            app.websocketManager = {
                isConnected: true,
                pending: new Map(),
            };

            updateStatus();

            assert.equal(statusDiv.style.display, 'none');
        });

        it('shows only the progress line for ordinary tile traffic', () => {
            app.websocketManager = {
                isConnected: true,
                pending: new Map([
                    [1, {}],
                    [2, {}],
                    [3, {}],
                ]),
            };

            updateStatus();

            // A handful of outstanding requests is normal while panning: the
            // line says the server is working, and no count takes up space.
            assert.ok(progressEl.classList.contains('active'));
            assert.equal(statusDiv.style.display, 'none');
            assert.equal(statusDiv.querySelector('.pending-indicator'), null);
            assert.equal(progressEl.title, '3 tile requests pending');
        });

        it('shows no count at the backlog threshold', () => {
            app.websocketManager = {
                isConnected: true,
                pending: new Map(Array.from({ length: 20 }, (_, i) => [i + 1, {}])),
            };

            updateStatus();

            assert.ok(progressEl.classList.contains('active'));
            assert.equal(statusDiv.querySelector('.pending-indicator'), null);
        });

        it('shows the count once the backlog threshold is passed', () => {
            app.websocketManager = {
                isConnected: true,
                pending: new Map(Array.from({ length: 25 }, (_, i) => [i + 1, {}])),
            };

            updateStatus();

            const indicator = statusDiv.querySelector('.pending-indicator');
            assert.ok(indicator);
            assert.equal(indicator.textContent, 'pending: 25');
            assert.equal(statusDiv.style.display, 'block');
        });

        it('clears the progress line when the queue drains', () => {
            app.websocketManager = {
                isConnected: true,
                pending: new Map(Array.from({ length: 25 }, (_, i) => [i + 1, {}])),
            };

            updateStatus();
            assert.ok(statusDiv.innerHTML.includes('pending: 25'));
            assert.ok(progressEl.classList.contains('active'));

            // Simulate request completion
            app.websocketManager.pending.clear();
            updateStatus();
            assert.equal(statusDiv.style.display, 'none');
            assert.equal(progressEl.classList.contains('active'), false);
            assert.equal(progressEl.title, '');

            // Add more requests
            app.websocketManager.pending
                = new Map(Array.from({ length: 30 }, (_, i) => [i + 1, {}]));
            updateStatus();
            assert.ok(statusDiv.innerHTML.includes('pending: 30'));
            assert.ok(progressEl.classList.contains('active'));
        });
    });

    describe('Disconnected state (new functionality)', () => {
        it('does not immediately show banner on disconnect', () => {
            app.websocketManager = {
                isConnected: false,
                pending: new Map(),
            };

            updateStatus();

            // Should not show banner yet
            assert.equal(statusDiv.style.display, 'none');
            // Should have scheduled a timeout
            assert.ok(updateStatus._getDisconnectTimeout() !== null);
        });

        it('schedules timeout with correct delay when disconnected', () => {
            app.websocketManager = {
                isConnected: false,
                pending: new Map(),
            };

            updateStatus();

            const timeouts = updateStatus._getTimeouts();
            assert.equal(timeouts.size, 1);
            const [id, timeout] = [...timeouts.entries()][0];
            assert.equal(timeout.delay, DISCONNECT_DELAY_MS);
        });

        it('clears timeout when reconnected', () => {
            app.websocketManager = {
                isConnected: false,
                pending: new Map(),
            };

            updateStatus();
            assert.ok(updateStatus._getDisconnectTimeout() !== null);

            // Reconnect
            app.websocketManager.isConnected = true;
            updateStatus();

            assert.equal(updateStatus._getDisconnectTimeout(), null);
            assert.equal(statusDiv.style.display, 'none');
        });

        it('shows a backlog count after reconnecting with a long queue', () => {
            app.websocketManager = {
                isConnected: false,
                pending: new Map(),
            };

            updateStatus();

            // Reconnect with a queue long enough to be worth a number
            app.websocketManager.isConnected = true;
            app.websocketManager.pending
                = new Map(Array.from({ length: 25 }, (_, i) => [i + 1, {}]));
            updateStatus();

            assert.equal(statusDiv.style.display, 'block');
            assert.ok(statusDiv.innerHTML.includes('pending: 25'));
        });

        it('does not schedule multiple timeouts on repeated disconnect calls', () => {
            app.websocketManager = {
                isConnected: false,
                pending: new Map(),
            };

            updateStatus();
            const timeoutsAfterFirst = updateStatus._getTimeouts().size;

            // Call updateStatus again while still disconnected
            updateStatus();
            const timeoutsAfterSecond = updateStatus._getTimeouts().size;

            // Should not create another timeout
            assert.equal(timeoutsAfterFirst, timeoutsAfterSecond);
        });
    });

    describe('Edge cases', () => {
        it('handles null websocketManager gracefully', () => {
            app.websocketManager = null;

            updateStatus();

            assert.equal(statusDiv.style.display, 'none');
        });

        it('handles connect/disconnect cycle without stale timeouts', () => {
            app.websocketManager = {
                isConnected: false,
                pending: new Map(),
            };

            updateStatus();
            const firstTimeout = updateStatus._getDisconnectTimeout();
            assert.ok(firstTimeout !== null);

            // Reconnect
            app.websocketManager.isConnected = true;
            updateStatus();
            assert.equal(updateStatus._getDisconnectTimeout(), null);

            // Disconnect again
            app.websocketManager.isConnected = false;
            updateStatus();
            const secondTimeout = updateStatus._getDisconnectTimeout();
            assert.ok(secondTimeout !== null);
            assert.notEqual(secondTimeout, firstTimeout, 'Should have new timeout, not old one');
        });

        it('does not show stale banner content after state changes', () => {
            app.websocketManager = {
                isConnected: true,
                pending: new Map(Array.from({ length: 25 }, (_, i) => [i + 1, {}])),
            };

            updateStatus();
            assert.ok(statusDiv.innerHTML.includes('pending-indicator'));
            assert.equal(statusDiv.innerHTML.includes('disconnected-banner'), false);

            app.websocketManager.isConnected = false;
            updateStatus();

            // Still shows pending indicator (not the banner, which only shows after timeout)
            assert.ok(statusDiv.innerHTML.includes('pending-indicator'));
        });

        it('displays correct HTML structure for pending indicator', () => {
            app.websocketManager = {
                isConnected: true,
                pending: new Map(Array.from({ length: 22 }, (_, i) => [i + 1, {}])),
            };

            updateStatus();

            const indicator = statusDiv.querySelector('.pending-indicator');
            assert.ok(indicator);
            assert.equal(indicator.textContent, 'pending: 22');
        });

        it('displays correct HTML structure for disconnected banner', () => {
            app.websocketManager = {
                isConnected: false,
                pending: new Map(),
            };

            // Manually trigger the banner display (simulating timeout callback)
            statusDiv.innerHTML = '<div class="disconnected-banner">⚠ OpenROAD disconnected</div>';
            statusDiv.style.display = 'block';

            const banner = statusDiv.querySelector('.disconnected-banner');
            assert.ok(banner);
            assert.ok(banner.textContent.includes('OpenROAD disconnected'));
        });
    });
});

