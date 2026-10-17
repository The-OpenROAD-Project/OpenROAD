// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The saved report inlines every module into one script
// (embed_report_assets.py).  A module the embedder cannot rewrite, or one that
// runs before a module it imports, breaks the whole report, and nothing short
// of running the bundle shows it.  So run it, against stand-ins for what the
// report page loads from the network.

import './setup-dom.js';
import { describe, it } from 'node:test';
import { makeFakeDeck, makeFrames } from './fake-deck.js';
import assert from 'node:assert/strict';
import { existsSync, readFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const here = dirname(fileURLToPath(import.meta.url));
// Bazel: the genrule output next to the sources.  CMake: the build tree.
const cppPath = [
    process.env.REPORT_ASSETS_CPP,
    join(here, '../../src/report_assets.cpp'),
    join(here, '../../../../build/src/web/report_assets.cpp'),
].find((p) => p && existsSync(p));

function bundleJs() {
    const cpp = readFileSync(cppPath, 'utf8');
    const open = 'R"__JS__(';
    const start = cpp.indexOf(open) + open.length;
    return cpp.slice(start, cpp.indexOf(')__JS__"', start));
}

// Anything the page would get from GoldenLayout or three.js: every property is
// callable and constructible, and nothing is a thenable.
function anything() {
    return new Proxy(function() {}, {
        get: (target, key) => (key === 'then' ? undefined
            : key === Symbol.toPrimitive ? () => '' : anything()),
        apply: () => anything(),
        construct: () => anything(),
        set: () => true,
    });
}

// The report page's body (web.cpp).
const BODY = '<div id="menu-bar"></div><div id="gl-container"></div>'
    + '<div id="websocket-status"></div>'
    + '<div id="loading-overlay"><div class="loading-overlay-content"></div>'
    + '</div>';

// Bazel always provides it, so only a local run may skip.
const skip = !cppPath && !process.env.TEST_SRCDIR
    && 'no report_assets.cpp; build the report assets or set REPORT_ASSETS_CPP';

describe('saved report bundle', { skip }, () => {
    it('has no export left for the browser to reject', () => {
        const stray = bundleJs().split('\n').filter((l) => /^\s*export\s/.test(l));
        assert.deepEqual(stray, []);
    });

    let exported = null;
    async function runBundle() {
        if (exported) {
            return exported;
        }
        document.body.innerHTML = BODY;
        // Never connects: the bundle's own server requests stay pending.
        globalThis.WebSocket = class {
            addEventListener() {}
            send() {}
            close() {}
        };
        const AsyncFunction = (async () => {}).constructor;
        const run = new AsyncFunction(
            'GoldenLayout', 'LayoutConfig', 'THREE',
            '"use strict";\n' + bundleJs() + '\nreturn { captureLayout, '
            + 'renderMergedTile, copyPngToClipboard, createLayoutView };');
        exported = await run(anything(), anything(), anything());
        return exported;
    }

    it('runs every module without a load-time error', async () => {
        for (const [name, value] of Object.entries(await runBundle())) {
            assert.equal(typeof value, 'function', name);
        }
    });

    // layout-view.js imports `zoomAround as zoomAroundTarget`.
    it('keeps aliased imports: a view from the bundle zooms', async () => {
        const { createLayoutView } = await runBundle();
        const frames = makeFrames();
        const container = document.createElement('div');
        document.body.appendChild(container);
        const view = createLayoutView(container, {
            deckApi: makeFakeDeck(),
            prefs: { smooth: () => false, doubleBuffer: () => true,
                     defer: () => false },
            tileSize: () => 240, dpr: () => 1,
            raf: frames.raf, now: frames.now,
        });
        view.setView([120, 120], 2, { animate: false });
        view.zoomAround(null, 1);
        frames.flush();
        assert.equal(view.getZoom(), 3);
    });
});
