// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Loads the web_save_report HTML fixture via jsdom, lets the inline
// <script>window.__STATIC_CACHE__ = {...};</script> block evaluate, and
// asserts that the cache was populated with real tiles, timing paths, and
// path overlays. This guards against issue #10201 — the report file was
// written but opened blank in a browser because either (a) the inline script
// was malformed and threw a SyntaxError on evaluation, or (b) the cache
// contained only empty arrays.

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { fileURLToPath } from 'node:url';
import { JSDOM } from 'jsdom';

// Fixture lives one directory up from this test file (in the same Bazel
// package) — resolve relative to the test script's own URL so it works
// regardless of cwd.
const fixturePath = fileURLToPath(
    new URL('../save_report_fixture.html', import.meta.url));

function loadCache() {
    const html = fs.readFileSync(fixturePath, 'utf8');
    const dom = new JSDOM(html, { runScripts: 'dangerously' });
    return { html, cache: dom.window.__STATIC_CACHE__ };
}

describe('web_save_report static cache', () => {
    it('inline cache script evaluates without throwing', () => {
        const { cache } = loadCache();
        assert.ok(cache,
            '__STATIC_CACHE__ is defined (inline <script> ran without SyntaxError)');
    });

    it('zoom level is fixed at 1', () => {
        const { cache } = loadCache();
        assert.equal(cache.zoom, 1);
    });

    it('tech, bounds, timing, histogram, filter responses are cached', () => {
        const { cache } = loadCache();
        assert.ok(cache.json, 'json bag present');
        assert.ok(cache.json.tech, 'tech response');
        assert.ok(cache.json.bounds, 'bounds response');
        assert.ok(cache.json['timing_report:setup'], 'setup timing');
        assert.ok(cache.json['timing_report:hold'], 'hold timing');
        assert.ok(cache.json['slack_histogram:setup'], 'setup histogram');
        assert.ok(cache.json['slack_histogram:hold'], 'hold histogram');
        assert.ok(cache.json.chart_filters, 'chart filters');
    });

    it('at least one tile rendered at zoom 1', () => {
        const { cache } = loadCache();
        const keys = Object.keys(cache.tiles || {});
        assert.ok(keys.length > 0, 'tiles object non-empty');
        assert.ok(keys.every(k => /^.+\/1\/\d+\/\d+$/.test(k)),
            'every key is layer/1/x/y');
        assert.ok(Object.values(cache.tiles).every(
            v => typeof v === 'string' && v.startsWith('iVBO')),
            'every tile value is a base64 PNG');
    });

    it('overlay arrays exist for setup and hold', () => {
        const { cache } = loadCache();
        assert.ok(Array.isArray(cache.overlays?.setup), 'setup overlay array');
        assert.ok(Array.isArray(cache.overlays?.hold), 'hold overlay array');
    });

    it('no raw </script> leaked inside the cache block', () => {
        const { html } = loadCache();
        const start = html.indexOf('__STATIC_CACHE__');
        assert.ok(start >= 0);
        const end = html.indexOf('</script>', start);
        assert.ok(end > start);
        const block = html.slice(start, end);
        assert.ok(!/<\/script\s*>/i.test(block),
            'raw </script> inside inline JSON would truncate the script');
    });
});
