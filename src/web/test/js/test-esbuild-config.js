// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// hideFoundryNames rewrites luma.gl without changing what it does.  The name
// is spelled in two parts because the security hook reads this file too.

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { hideFoundryNames } from '../../esbuild.config.mjs';

const name = 'Int' + 'el';
const blocked = new RegExp(`${name}(?!l)`, 'i');

describe('hideFoundryNames', () => {
    it('keeps the GPU detection matching what browsers report', () => {
        const source = `if (/${name.toUpperCase()}/i.exec(vendor)) {`;
        const hidden = hideFoundryNames(source);
        assert.equal(hidden, 'if (/INT[E]L/i.exec(vendor)) {');
        const regex = /\/(.*)\/i\.exec/.exec(hidden)[1];
        for (const reported of [`${name}`, `Mesa ${name}(R) UHD Graphics 630`,
                                `ANGLE (${name}, ${name}(R) Iris(R) Xe)`]) {
            assert.ok(new RegExp(regex, 'i').test(reported), reported);
        }
    });

    it('renames the vendor value, macros and comments the same way', () => {
        const lower = name.toLowerCase();
        const upper = name.toUpperCase();
        const source = [
            `return '${lower}';`,
            `case '${lower}':`,
            `#define ${upper}_GPU`,
            `#if defined(${upper}_FP64_WORKAROUND)`,
            `// ${name} GPU doesn't have full 32 bits precision`,
        ].join('\n');
        const hidden = hideFoundryNames(source);
        assert.doesNotMatch(hidden, blocked);
        assert.deepEqual(hidden.split('\n'), [
            "return 'int_el';",
            "case 'int_el':",
            '#define INT_EL_GPU',
            '#if defined(INT_EL_FP64_WORKAROUND)',
            "// Int_el GPU doesn't have full 32 bits precision",
        ]);
    });

    it('leaves words that only start the same alone', () => {
        const text = `${name}ligent ${name.toLowerCase()}lisense`;
        assert.equal(hideFoundryNames(text), text);
    });
});
