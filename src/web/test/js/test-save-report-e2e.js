// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// End-to-end test: load the generated web_save_report HTML fixture under
// jsdom, evaluate the application script, and assert that it initializes
// the map and Golden Layout without raising any unhandled errors.
//
// Why this exists: issue #10201 had two follow-up regressions that the
// narrower static-cache test (test-save-report-static-cache.js) could not
// catch because it only inspects the inline __STATIC_CACHE__ JSON and never
// runs main.js:
//   (a) `Cannot read properties of null (reading 'fitBounds')` — the
//       initial-data `.then` callback assumed app.map was already set by
//       the LayoutViewer factory, which only holds when LayoutViewer is the
//       initial active tab in its stack.
//   (b) `Cannot read properties of undefined (reading 'isComponent')` —
//       focusComponent, invoked by a Windows-menu click, dereferenced
//       app.goldenLayout.rootItem without checking it was defined.
//
// jsdom does not execute <script type="module">, so we extract the main
// application script from the fixture and eval it from a plain <script>
// after DOMContentLoaded — this matches how a deferred module script runs
// in a real browser (document.readyState != 'loading' when it starts) and
// lets GoldenLayout's constructor self-initialize instead of deferring to
// DOMContentLoaded.

import { describe, it, before } from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { fileURLToPath } from 'node:url';
import { JSDOM, VirtualConsole } from 'jsdom';

const fixturePath = fileURLToPath(
    new URL('../save_report_fixture.html', import.meta.url));

// jsdom's HTMLCanvasElement has no 2d context without the native `canvas`
// npm package. ClockTreeWidget and ChartsWidget both call getContext('2d')
// during their constructors (wired up as Golden Layout factories that fire
// during loadLayout). Since neither is relevant to this test, filter their
// "Not implemented: HTMLCanvasElement.prototype.getContext" noise instead
// of pulling in the native canvas dep.
const CANVAS_NOT_IMPLEMENTED = /HTMLCanvasElement\.prototype\.getContext/;

function extractMainScript(html) {
    // The report emits one `<script type="module">` block containing the
    // concatenated application code. Carve it out so jsdom doesn't see it
    // during parse (jsdom 26 silently skips type="module"), and we can
    // eval it from a plain <script> after DOMContentLoaded.
    const open = '<script type="module">';
    const openIdx = html.lastIndexOf(open);
    assert.ok(openIdx > 0, 'main <script type="module"> not found in fixture');
    const closeIdx = html.indexOf('</script>', openIdx);
    assert.ok(closeIdx > openIdx, 'no </script> after main module script');
    let app = html.slice(openIdx + open.length, closeIdx);
    // Expose `app` to the test by hooking the outer block. The application
    // script is wrapped in `{ ... }` (from embed_report_assets.py's
    // `process_js_file`) for const/let isolation, which also makes `app`
    // unreachable from outside. Insert a single probe assignment just
    // before the closing brace.
    app = app.replace(/\}\s*$/, '\nwindow.__appProbe = app;\n}\n');
    return {
        html: html.slice(0, openIdx) + html.slice(closeIdx + '</script>'.length),
        appScript: app,
    };
}

function buildDom(html) {
    const vc = new VirtualConsole();
    const errors = [];
    vc.on('jsdomError', (e) => {
        if (CANVAS_NOT_IMPLEMENTED.test(e.message)) return;
        errors.push({ kind: 'jsdomError', message: e.message });
    });
    // .then(initialDataLoaded) in main.js wraps its body in try/catch that
    // console.errors failures — so a regression in the fitBounds path would
    // never surface as an uncaught exception. Catch it here too.
    vc.on('error', (...args) => {
        const txt = args.map(String).join(' ');
        if (/Failed to load initial data from server/.test(txt)) {
            errors.push({ kind: 'console.error', message: txt });
        }
    });

    const dom = new JSDOM(html, {
        runScripts: 'dangerously',
        pretendToBeVisual: true,
        virtualConsole: vc,
        // file:// is an opaque origin in jsdom — localStorage would throw.
        url: 'http://localhost/save_report_fixture.html',
        beforeParse(win) {
            // Shims for APIs jsdom doesn't implement but main.js/GL use.
            win.ResizeObserver = class {
                observe() {} unobserve() {} disconnect() {}
            };
            win.matchMedia = () => ({
                matches: false,
                addEventListener() {}, removeEventListener() {},
                addListener() {}, removeListener() {},
            });
        },
    });
    dom.window.addEventListener('error', (e) => {
        errors.push({ kind: 'error', message: e.message,
                      stack: (e.error?.stack || '').split('\n').slice(0, 5).join('\n') });
    });
    dom.window.addEventListener('unhandledrejection', (e) => {
        errors.push({ kind: 'unhandledrejection',
                      message: e.reason?.message || String(e.reason),
                      stack: (e.reason?.stack || '').split('\n').slice(0, 5).join('\n') });
    });
    return { dom, errors };
}

async function runFixture({ seedLocalStorage, preScriptHook } = {}) {
    const fullHtml = fs.readFileSync(fixturePath, 'utf8');
    const { html, appScript } = extractMainScript(fullHtml);
    const { dom, errors } = buildDom(html);

    await new Promise((r) => dom.window.addEventListener(
        'DOMContentLoaded', r, { once: true }));

    if (seedLocalStorage) seedLocalStorage(dom.window.localStorage);
    if (preScriptHook) preScriptHook(dom.window);

    const s = dom.window.document.createElement('script');
    s.textContent = appScript;
    dom.window.document.body.appendChild(s);

    // Let the `.then(initialDataLoaded)` microtask chain settle.
    await new Promise((r) => setTimeout(r, 300));

    return { dom, errors, app: dom.window.__appProbe };
}

describe('web_save_report end-to-end (jsdom)', () => {
    let first;
    before(async () => { first = await runFixture(); });

    it('main application script evaluates with no unhandled errors', () => {
        assert.deepEqual(first.errors, [],
            'errors surfaced from jsdom: ' + JSON.stringify(first.errors, null, 2));
    });

    it('GoldenLayout initializes and exposes rootItem', () => {
        assert.equal(first.app.goldenLayout.isInitialised, true);
        assert.ok(first.app.goldenLayout.rootItem, 'rootItem is defined');
    });

    it('Leaflet map is created and fitBounds has been applied', () => {
        assert.ok(first.app.map, 'app.map is non-null');
        assert.ok(Array.isArray(first.app.fitBounds),
            'app.fitBounds is computed');
    });

    it('focusComponent is a no-op (not a throw) before GL is initialized',
        () => {
            // Simulate the edge case behind the reported isComponent crash:
            // a menu click landing before loadLayout finishes. GL's
            // rootItem getter throws "Cannot access rootItem before init"
            // when _groundItem is undefined, and app.goldenLayout.rootItem
            // can also return undefined between init() and loadLayout().
            // focusComponent must tolerate both.
            const { app } = first;
            const saved = {
                initialised: app.goldenLayout._isInitialised,
                ground: app.goldenLayout._groundItem,
            };
            try {
                app.goldenLayout._isInitialised = false;
                app.goldenLayout._groundItem = undefined;
                assert.throws(() => app.goldenLayout.rootItem,
                    /Cannot access rootItem before init/,
                    'getter should throw pre-init (sanity)');
                assert.doesNotThrow(() => app.focusComponent('Inspector'));
            } finally {
                app.goldenLayout._isInitialised = saved.initialised;
                app.goldenLayout._groundItem = saved.ground;
            }
        });

    it('initial-data .then does not crash when LayoutViewer factory never ran',
        async () => {
            // The reported bug: `Cannot read properties of null (reading
            // 'fitBounds')`. This fires when Golden Layout instantiates a
            // layout whose active tab in LayoutViewer's stack is NOT
            // LayoutViewer — the factory (which sets app.map) is deferred
            // until the user activates that tab, but the initial-data
            // promise chain tried to call app.map.fitBounds unconditionally.
            //
            // Simulate that by stubbing GoldenLayout.loadLayout to skip
            // component instantiation entirely. app.map stays null while
            // the .then microtask still fires and attempts the fit-bounds
            // path. The crash — before the fix — was caught by the outer
            // try/catch as "Failed to load initial data from server:", so
            // the test watches console.error for that specific message.
            const run = await runFixture({
                preScriptHook(win) {
                    win.GoldenLayout.prototype.loadLayout = function() {};
                },
            });
            assert.equal(run.app.map, null,
                'sanity: LayoutViewer factory did not fire');
            assert.deepEqual(run.errors, [],
                'fitBounds must not be called through a null app.map: '
                + JSON.stringify(run.errors, null, 2));
        });
});
