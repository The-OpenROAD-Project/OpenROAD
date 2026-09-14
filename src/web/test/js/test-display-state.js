// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { JSDOM } from 'jsdom';

// Fresh DOM + fresh module instance per test.  display-state.js imports
// theme.js, which runs initialization code at import time and would otherwise
// be served from the ES module cache, so the import is cache-busted the same
// way test-theme.js does it.
async function loadDisplayState({ cookie, session, local } = {}) {
    const dom = new JSDOM('<!DOCTYPE html><html><body></body></html>', {
        url: 'http://localhost/',
    });
    globalThis.document = dom.window.document;
    globalThis.window = dom.window;
    globalThis.localStorage = dom.window.localStorage;
    globalThis.matchMedia = () => ({
        matches: false,
        addListener() {}, removeListener() {},
        addEventListener() {}, removeEventListener() {},
        dispatchEvent() { return false; },
    });

    for (const [k, v] of Object.entries(cookie ?? {})) {
        dom.window.document.cookie = `${k}=${v}`;
    }
    for (const [k, v] of Object.entries(session ?? {})) {
        dom.window.sessionStorage.setItem(k, v);
    }
    for (const [k, v] of Object.entries(local ?? {})) {
        dom.window.localStorage.setItem(k, v);
    }

    // Same suffix for both so they share one theme.js instance, and read
    // cookies through the product's own getCookie rather than a look-alike.
    const suffix = '?t=' + Date.now() + Math.random();
    const mod = await import('../../src/display-state.js' + suffix);
    const { getCookie } = await import('../../src/theme.js' + suffix);
    return { dom, mod, getCookie };
}

describe('serializeDisplayState', () => {
    it('reads each key from its own store', async () => {
        const { mod } = await loadDisplayState({
            cookie: { or_visibility: '%7B%22rows%22%3Atrue%7D',
                      or_show_dbu: '1' },
            session: { or_hidden_layers: '["layers_parent/metal1"]',
                       or_layer_patterns: '{"metal1":2}' },
        });
        const { entries, version } = mod.serializeDisplayState();
        assert.equal(version, 1);
        assert.equal(entries.or_visibility, '%7B%22rows%22%3Atrue%7D');
        assert.equal(entries.or_show_dbu, '1');
        assert.equal(entries.or_hidden_layers, '["layers_parent/metal1"]');
        assert.equal(entries.or_layer_patterns, '{"metal1":2}');
    });

    it('omits unset keys so restore can tell them from empty', async () => {
        const { mod } = await loadDisplayState({ cookie: { or_theme: 'dark' } });
        const { entries } = mod.serializeDisplayState();
        assert.deepEqual(Object.keys(entries), ['or_theme']);
    });

    // A cookie-only read would silently return nothing for the three
    // sessionStorage-backed keys, which is what shipped before this was
    // dispatched per store.
    it('does not lose the sessionStorage-backed keys', async () => {
        const { mod } = await loadDisplayState({
            session: { or_hidden_layers: '["a"]',
                       or_nonselectable_layers: '["b"]',
                       or_layer_patterns: '{"c":2}' },
        });
        const { entries } = mod.serializeDisplayState();
        assert.deepEqual(Object.keys(entries).sort(), [
            'or_hidden_layers', 'or_layer_patterns', 'or_nonselectable_layers',
        ]);
    });
});

describe('applyDisplayStateEntries', () => {
    it('round-trips every key through its own store', async () => {
        const all = {
            or_visibility: '%7B%22rows%22%3Atrue%7D',
            or_selectability: '%7B%7D',
            or_hidden_layers: '["layers_parent/metal1"]',
            or_nonselectable_layers: '["metal2"]',
            or_layer_patterns: '{"metal1":2}',
            or_hidden_chiplets: '%5B%5D',
            or_bg_color: '#202020',
            or_show_dbu: '1',
            or_theme: 'dark',
            or_use_true_z: '1',
            // Options-menu preferences (2.15).  The font family is stored
            // URI-encoded, since a CSS stack carries commas and quotes.
            or_ruler_style: 'manhattan',
            or_wheel_zoom: '0',
            or_arrow_step: '120',
            or_font_family: 'monospace',
            or_font_scale: '125',
        };
        const { dom, mod } = await loadDisplayState();
        mod.applyDisplayStateEntries(all);
        assert.deepEqual(mod.serializeDisplayState().entries, all);
        // And clearing everything really clears both stores.
        mod.applyDisplayStateEntries({});
        assert.deepEqual(mod.serializeDisplayState().entries, {});
    });

    // The theme is mirrored into localStorage for standalone file:// reports,
    // and theme.js reads `cookie || localStorage || system`.  Clearing only the
    // cookie left the stale mirror winning, so restoring a state without
    // or_theme silently kept the old theme.
    it('clears the localStorage theme mirror, not just the cookie',
       async () => {
        const { dom, mod, getCookie } = await loadDisplayState({
            cookie: { or_theme: 'dark' }, local: { or_theme: 'dark' },
        });
        mod.applyDisplayStateEntries({});
        assert.equal(getCookie('or_theme'), null);
        assert.equal(dom.window.localStorage.getItem('or_theme'), null,
                     'stale mirror would override the restored default');
    });

    it('writes the theme to both cookie and mirror', async () => {
        const { dom, mod, getCookie } = await loadDisplayState();
        mod.applyDisplayStateEntries({ or_theme: 'light' });
        assert.equal(getCookie('or_theme'), 'light');
        assert.equal(dom.window.localStorage.getItem('or_theme'), 'light');
    });

    // These values land verbatim in document.cookie, so a ';' would inject
    // cookie attributes and a number/boolean would change a key's meaning for
    // readers comparing with ===.  The server rejects such a file (WEB-0052);
    // this is the second line of defence.
    it('drops non-string values instead of coercing them', async () => {
        const { mod, getCookie } = await loadDisplayState();
        mod.applyDisplayStateEntries({
            or_show_dbu: 1,
            or_theme: true,
            or_bg_color: { nope: 1 },
        });
        assert.equal(getCookie('or_show_dbu'), null);
        assert.equal(getCookie('or_theme'), null);
        assert.equal(getCookie('or_bg_color'), null);
    });

    // Same guard, delimiter case.
    it('drops values carrying cookie delimiters', async () => {
        const { dom, mod, getCookie } = await loadDisplayState();
        mod.applyDisplayStateEntries({
            or_bg_color: '#fff; Domain=evil.test',
            or_theme: 'dark\r\nSet-Cookie: x=y',
        });
        assert.equal(getCookie('or_bg_color'), null);
        assert.equal(getCookie('or_theme'), null);
    });
});

describe('storage failures', () => {
    it('survives sessionStorage throwing', async () => {
        const { dom, mod } = await loadDisplayState();
        Object.defineProperty(dom.window, 'sessionStorage', {
            get() { throw new Error('storage disabled'); },
        });
        assert.doesNotThrow(() => mod.applyDisplayStateEntries({
            or_hidden_layers: '["a"]',
        }));
        assert.doesNotThrow(() => mod.serializeDisplayState());
    });
});
