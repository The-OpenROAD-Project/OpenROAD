// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Every module must import without the renderer global (`deck`), which only
// exists once its CDN script has run.  A module that reads it at its top
// level breaks every test that imports it and the saved report too.

import './setup-dom.js';
import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { readdirSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const srcDir = join(dirname(fileURLToPath(import.meta.url)), '../../src');

describe('modules import without the renderer global', () => {
    for (const file of readdirSync(srcDir).filter((f) => f.endsWith('.js'))) {
        it(file, async () => {
            const saved = { deck: globalThis.deck };
            delete globalThis.deck;
            try {
                await import(join(srcDir, file));
            } catch (e) {
                // Remote ES imports (esm.sh) cannot resolve offline; only a
                // missing global is this test's business.
                assert.ok(!/\bdeck is not defined/.test(String(e)),
                          `${file}: ${e}`);
            } finally {
                Object.assign(globalThis, saved);
            }
        });
    }
});
