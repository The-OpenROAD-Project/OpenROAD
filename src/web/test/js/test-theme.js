// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it, beforeEach } from 'node:test';
import assert from 'node:assert/strict';
import { JSDOM } from 'jsdom';

// Helper to create a fresh DOM environment with optional cookie/localStorage
// pre-seeded, then dynamically import theme.js (which runs on import).
async function loadTheme({ cookie, localStorageTheme, prefersDark } = {}) {
    const dom = new JSDOM('<!DOCTYPE html><html><body></body></html>', {
        url: 'http://localhost/',
    });
    globalThis.document = dom.window.document;
    globalThis.localStorage = dom.window.localStorage;

    if (cookie) {
        // JSDOM supports document.cookie assignment.
        dom.window.document.cookie = cookie;
    }
    if (localStorageTheme) {
        dom.window.localStorage.setItem('or_theme', localStorageTheme);
    }
    globalThis.matchMedia = () => ({
        matches: prefersDark === false, // prefers-color-scheme: light
        addListener() {},
        removeListener() {},
        addEventListener() {},
        removeEventListener() {},
        dispatchEvent() { return false; },
    });

    // theme.js caches state on import, so we need a fresh import each time.
    // Use a cache-busting query parameter.
    const suffix = '?t=' + Date.now() + Math.random();
    const mod = await import('../../src/theme.js' + suffix);
    return { dom, mod };
}

describe('theme initialization', () => {
    it('uses cookie when present', async () => {
        const { dom } = await loadTheme({ cookie: 'or_theme=light' });
        assert.equal(dom.window.document.documentElement.dataset.theme, 'light');
    });

    it('falls back to localStorage when no cookie', async () => {
        const { dom } = await loadTheme({ localStorageTheme: 'dark' });
        assert.equal(dom.window.document.documentElement.dataset.theme, 'dark');
    });

    it('prefers cookie over localStorage', async () => {
        const { dom } = await loadTheme({
            cookie: 'or_theme=dark',
            localStorageTheme: 'light',
        });
        assert.equal(dom.window.document.documentElement.dataset.theme, 'dark');
    });

    it('falls back to OS preference when no stored theme', async () => {
        // prefers-color-scheme: light → matches = true in our mock
        const { dom } = await loadTheme({ prefersDark: false });
        assert.equal(dom.window.document.documentElement.dataset.theme, 'light');
    });

    it('defaults to dark when OS prefers dark', async () => {
        const { dom } = await loadTheme({ prefersDark: true });
        assert.equal(dom.window.document.documentElement.dataset.theme, 'dark');
    });
});

describe('setCookie', () => {
    it('writes a cookie readable by getCookie', async () => {
        const { dom, mod } = await loadTheme();
        mod.setCookie('or_theme', 'light');
        assert.ok(dom.window.document.cookie.includes('or_theme=light'));
    });

    it('overwrites previous value', async () => {
        const { dom, mod } = await loadTheme({ cookie: 'or_theme=dark' });
        mod.setCookie('or_theme', 'light');
        // The new value should be present.
        assert.ok(dom.window.document.cookie.includes('or_theme=light'));
    });
});
