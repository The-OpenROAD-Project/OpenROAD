// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// golden-layout publishes no browser build, so the build bundles its dist/esm
// tree.  A version bump can leave that bundling silently producing a module the
// viewer cannot use; main.js only asks for these two names, so this is what
// catches it.

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';

const bundle = await import(
    '../../third-party/golden-layout/golden-layout.esm.js');

describe('the golden-layout bundle', () => {
    it('exports what main.js imports', () => {
        assert.equal(typeof bundle.GoldenLayout, 'function');
        // A merged TypeScript namespace, not a class: main.js reaches it for
        // LayoutConfig.fromResolved().
        assert.equal(typeof bundle.LayoutConfig.fromResolved, 'function');
    });

    it('resolves its own internal imports', () => {
        // A bundle that lost a module still imports; it fails when a class up
        // the chain is undefined at `extends` time, which is here.
        assert.equal(typeof bundle.GoldenLayout.prototype.loadLayout, 'function');
    });
});
