// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Every module must import without `deck`, the global deck-global.js sets:
// one read at a module's top level breaks every test that imports it.

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
                // Only a missing global is this test's business.
                assert.ok(!/\bdeck is not defined/.test(String(e)),
                          `${file}: ${e}`);
            } finally {
                Object.assign(globalThis, saved);
            }
        });
    }
});
