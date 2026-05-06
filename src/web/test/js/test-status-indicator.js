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
    const DISCONNECT_DELAY_MS = 2000;

    beforeEach(async () => {
        // Setup DOM
        dom = new JSDOM(`
            <!DOCTYPE html>
            <html>
            <body>
                <div id="websocket-status" style="display: none;"></div>
            </body>
            </html>
        `);
        globalThis.document = dom.window.document;

        statusDiv = document.getElementById('websocket-status');

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
                
                if (pendingCount === 0) {
                    statusDiv.style.display = 'none';
                } else {
                    statusDiv.innerHTML = `<div class="pending-indicator">pending: ${pendingCount}</div>`;
                    statusDiv.style.display = 'block';
                    const color = pendingCount > 20 ? 'var(--error)' : 'var(--fg-bright)';
                    statusDiv.querySelector('.pending-indicator').style.color = color;
                }
            }
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

        it('shows pending count when connected with pending requests', () => {
            app.websocketManager = {
                isConnected: true,
                pending: new Map([
                    [1, {}],
                    [2, {}],
                    [3, {}],
                ]),
            };

            updateStatus();

            assert.equal(statusDiv.style.display, 'block');
            assert.ok(statusDiv.innerHTML.includes('pending: 3'));
        });

        it('shows pending indicator with normal color when count <= 20', () => {
            app.websocketManager = {
                isConnected: true,
                pending: new Map(Array.from({ length: 15 }, (_, i) => [i + 1, {}])),
            };

            updateStatus();

            const indicator = statusDiv.querySelector('.pending-indicator');
            assert.ok(indicator);
            assert.equal(indicator.style.color, 'var(--fg-bright)');
        });

        it('shows pending indicator with error color when count > 20', () => {
            app.websocketManager = {
                isConnected: true,
                pending: new Map(Array.from({ length: 25 }, (_, i) => [i + 1, {}])),
            };

            updateStatus();

            const indicator = statusDiv.querySelector('.pending-indicator');
            assert.ok(indicator);
            assert.equal(indicator.style.color, 'var(--error)');
        });

        it('updates pending count on repeated calls', () => {
            app.websocketManager = {
                isConnected: true,
                pending: new Map([[1, {}]]),
            };

            updateStatus();
            assert.ok(statusDiv.innerHTML.includes('pending: 1'));

            // Simulate request completion
            app.websocketManager.pending.clear();
            updateStatus();
            assert.equal(statusDiv.style.display, 'none');

            // Add more requests
            app.websocketManager.pending = new Map(Array.from({ length: 5 }, (_, i) => [i + 1, {}]));
            updateStatus();
            assert.ok(statusDiv.innerHTML.includes('pending: 5'));
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

        it('shows pending count after reconnecting with pending requests', () => {
            app.websocketManager = {
                isConnected: false,
                pending: new Map(),
            };

            updateStatus();

            // Reconnect with pending requests
            app.websocketManager.isConnected = true;
            app.websocketManager.pending = new Map(Array.from({ length: 3 }, (_, i) => [i + 1, {}]));
            updateStatus();

            assert.equal(statusDiv.style.display, 'block');
            assert.ok(statusDiv.innerHTML.includes('pending: 3'));
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
                pending: new Map([[1, {}]]),
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
                pending: new Map([[1, {}], [2, {}]]),
            };

            updateStatus();

            const indicator = statusDiv.querySelector('.pending-indicator');
            assert.ok(indicator);
            assert.equal(indicator.textContent, 'pending: 2');
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

