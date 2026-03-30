// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it, beforeEach } from 'node:test';
import assert from 'node:assert/strict';
import { dom } from './setup-dom.js';

const { TclCompleter } = await import('../../src/tcl-completer.js');

// Mock WebSocketManager that returns canned responses.
class MockWsManager {
    constructor() {
        this._handler = null;
    }
    onRequest(handler) {
        this._handler = handler;
    }
    request(msg) {
        if (this._handler) {
            return Promise.resolve(this._handler(msg));
        }
        return Promise.resolve({
            completions: [],
            mode: 'commands',
            prefix: '',
            replace_start: 0,
            replace_end: 0,
        });
    }
}

function createInput() {
    const row = document.createElement('div');
    row.className = 'tcl-input-row';
    const input = document.createElement('input');
    input.type = 'text';
    row.appendChild(input);
    document.body.appendChild(row);
    return input;
}

function makeKeyEvent(key, opts = {}) {
    return new dom.window.KeyboardEvent('keydown', {
        key,
        bubbles: true,
        cancelable: true,
        ...opts,
    });
}

describe('TclCompleter', () => {
    let input, ws, completer;

    beforeEach(() => {
        document.body.innerHTML = '';
        input = createInput();
        ws = new MockWsManager();
        completer = new TclCompleter(input, ws);
    });

    describe('prefix extraction', () => {
        it('extracts word at cursor', () => {
            // _extractPrefix is private, test via handleKeyDown + Tab
            input.value = 'place_de';
            input.setSelectionRange(8, 8);
            // The prefix should be 'place_de'
            const prefix = completer._extractPrefix('place_de', 8);
            assert.equal(prefix, 'place_de');
        });

        it('handles word after bracket', () => {
            const prefix = completer._extractPrefix('[get_cells foo', 14);
            assert.equal(prefix, 'foo');
        });

        it('handles word at start', () => {
            const prefix = completer._extractPrefix('route', 5);
            assert.equal(prefix, 'route');
        });

        it('handles variable prefix', () => {
            const prefix = completer._extractPrefix('set $my_v', 9);
            assert.equal(prefix, '$my_v');
        });

        it('handles argument prefix', () => {
            const prefix = completer._extractPrefix('place_design -den', 17);
            assert.equal(prefix, '-den');
        });

        it('handles empty line', () => {
            const prefix = completer._extractPrefix('', 0);
            assert.equal(prefix, '');
        });

        it('handles cursor in middle', () => {
            const prefix = completer._extractPrefix('place_design -density 0.5', 14);
            assert.equal(prefix, '-');
        });

        it('handles braces as boundary', () => {
            const prefix = completer._extractPrefix('{route}', 6);
            assert.equal(prefix, 'route');
        });
    });

    describe('command history', () => {
        it('navigates history with up/down', () => {
            completer.addToHistory('cmd1');
            completer.addToHistory('cmd2');
            completer.addToHistory('cmd3');

            input.value = 'current';

            // Up arrow goes to most recent
            const handled1 = completer.handleKeyDown(makeKeyEvent('ArrowUp'));
            assert.ok(handled1);
            assert.equal(input.value, 'cmd3');

            const handled2 = completer.handleKeyDown(makeKeyEvent('ArrowUp'));
            assert.ok(handled2);
            assert.equal(input.value, 'cmd2');

            // Down arrow goes back
            const handled3 = completer.handleKeyDown(makeKeyEvent('ArrowDown'));
            assert.ok(handled3);
            assert.equal(input.value, 'cmd3');

            // Down again restores original
            const handled4 = completer.handleKeyDown(makeKeyEvent('ArrowDown'));
            assert.ok(handled4);
            assert.equal(input.value, 'current');
        });

        it('does not duplicate consecutive entries', () => {
            completer.addToHistory('cmd1');
            completer.addToHistory('cmd1');
            assert.equal(completer._history.length, 1);
        });

        it('resets history index on Enter', () => {
            completer.addToHistory('cmd1');
            completer.handleKeyDown(makeKeyEvent('ArrowUp'));
            assert.equal(input.value, 'cmd1');

            // Enter when popup not visible returns false (not consumed)
            const handled = completer.handleKeyDown(makeKeyEvent('Enter'));
            assert.equal(handled, false);
            assert.equal(completer._historyIndex, -1);
        });
    });

    describe('keyboard handling', () => {
        it('Tab triggers completion request', async () => {
            let requested = false;
            ws.onRequest((msg) => {
                if (msg.type === 'tcl_complete') {
                    requested = true;
                    return {
                        completions: ['place_cell', 'place_design'],
                        mode: 'commands',
                        prefix: 'place',
                        replace_start: 0,
                        replace_end: 5,
                    };
                }
            });

            input.value = 'place';
            input.setSelectionRange(5, 5);
            const handled = completer.handleKeyDown(makeKeyEvent('Tab'));
            assert.ok(handled);

            // Wait for async request
            await new Promise(r => setTimeout(r, 10));
            assert.ok(requested);
        });

        it('Escape hides popup', () => {
            // Show popup manually
            completer._showPopup(['a', 'b'], 0, 1);
            assert.notEqual(completer._popup.style.display, 'none');

            const handled = completer.handleKeyDown(makeKeyEvent('Escape'));
            assert.ok(handled);
            assert.equal(completer._popup.style.display, 'none');
        });

        it('Escape returns false when popup hidden', () => {
            const handled = completer.handleKeyDown(makeKeyEvent('Escape'));
            assert.equal(handled, false);
        });
    });

    describe('popup display', () => {
        it('shows completions', () => {
            input.value = 'pla';
            completer._showPopup(
                ['place_cell', 'place_design', 'place_pin'],
                0, 3
            );
            assert.notEqual(completer._popup.style.display, 'none');
            const items = completer._popup.querySelectorAll('.tcl-complete-item');
            assert.equal(items.length, 3);
            assert.equal(items[0].textContent, 'place_cell');
        });

        it('hides when empty', () => {
            completer._showPopup([], 0, 0);
            assert.equal(completer._popup.style.display, 'none');
        });

        it('hides when single exact match', () => {
            input.value = 'place_cell';
            completer._showPopup(['place_cell'], 0, 10);
            assert.equal(completer._popup.style.display, 'none');
        });

        it('selects first item by default', () => {
            input.value = 'pla';
            completer._showPopup(['place_cell', 'place_design'], 0, 3);
            assert.equal(completer._selectedIndex, 0);
            const items = completer._popup.querySelectorAll('.tcl-complete-item');
            assert.ok(items[0].classList.contains('selected'));
        });
    });

    describe('completion acceptance', () => {
        it('replaces prefix with selected completion', () => {
            input.value = 'pla';
            completer._showPopup(['place_cell', 'place_design'], 0, 3);
            completer._selectedIndex = 1;
            completer._acceptCompletion();
            assert.equal(input.value, 'place_design');
        });

        it('preserves text after cursor', () => {
            input.value = 'pla -verbose';
            completer._showPopup(['place_design'], 0, 3);
            completer._acceptCompletion();
            assert.equal(input.value, 'place_design -verbose');
        });

        it('Enter accepts when popup visible', () => {
            input.value = 'pla';
            completer._showPopup(['place_cell', 'place_design'], 0, 3);
            completer._selectedIndex = 0;
            const handled = completer.handleKeyDown(makeKeyEvent('Enter'));
            assert.ok(handled);
            assert.equal(input.value, 'place_cell');
        });
    });

    describe('arrow navigation', () => {
        it('ArrowDown moves selection forward', () => {
            input.value = 'p';
            completer._showPopup(['pa', 'pb', 'pc'], 0, 1);
            assert.equal(completer._selectedIndex, 0);

            completer.handleKeyDown(makeKeyEvent('ArrowDown'));
            assert.equal(completer._selectedIndex, 1);

            completer.handleKeyDown(makeKeyEvent('ArrowDown'));
            assert.equal(completer._selectedIndex, 2);
        });

        it('ArrowDown clamps at end', () => {
            input.value = 'p';
            completer._showPopup(['pa', 'pb'], 0, 1);
            completer._selectedIndex = 1;

            completer.handleKeyDown(makeKeyEvent('ArrowDown'));
            assert.equal(completer._selectedIndex, 1);
        });

        it('ArrowUp moves selection backward', () => {
            input.value = 'p';
            completer._showPopup(['pa', 'pb', 'pc'], 0, 1);
            completer._selectedIndex = 2;

            completer.handleKeyDown(makeKeyEvent('ArrowUp'));
            assert.equal(completer._selectedIndex, 1);
        });

        it('ArrowUp clamps at start', () => {
            input.value = 'p';
            completer._showPopup(['pa', 'pb'], 0, 1);
            completer._selectedIndex = 0;

            completer.handleKeyDown(makeKeyEvent('ArrowUp'));
            assert.equal(completer._selectedIndex, 0);
        });
    });
});
