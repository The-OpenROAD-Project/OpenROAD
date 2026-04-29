// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { updateDocumentTitle } from '../../src/title.js';

describe('updateDocumentTitle', () => {
    it('uses bare "OpenROAD" when no block name given', () => {
        const doc = { title: '' };
        const out = updateDocumentTitle('', doc);
        assert.equal(out, 'OpenROAD');
        assert.equal(doc.title, 'OpenROAD');
    });

    it('uses bare "OpenROAD" when block name is null', () => {
        const doc = { title: '' };
        const out = updateDocumentTitle(null, doc);
        assert.equal(out, 'OpenROAD');
        assert.equal(doc.title, 'OpenROAD');
    });

    it('uses bare "OpenROAD" when block name is undefined', () => {
        const doc = { title: '' };
        const out = updateDocumentTitle(undefined, doc);
        assert.equal(out, 'OpenROAD');
        assert.equal(doc.title, 'OpenROAD');
    });

    it('mirrors GUI format with " - " separator when block name present', () => {
        // GUI uses "{window_title} - {block_name}" (mainWindow.cpp:540).
        const doc = { title: '' };
        const out = updateDocumentTitle('gcd', doc);
        assert.equal(out, 'OpenROAD - gcd');
        assert.equal(doc.title, 'OpenROAD - gcd');
    });

    it('handles block names with special characters verbatim', () => {
        const doc = { title: '' };
        const out = updateDocumentTitle('top_module/foo', doc);
        assert.equal(out, 'OpenROAD - top_module/foo');
    });

    it('overwrites a previously set title', () => {
        const doc = { title: 'old' };
        updateDocumentTitle('design1', doc);
        assert.equal(doc.title, 'OpenROAD - design1');
        updateDocumentTitle('', doc);
        assert.equal(doc.title, 'OpenROAD');
    });

    it('returns the formatted title without a doc', () => {
        // Useful for callers that only need the string.
        const out = updateDocumentTitle('myblock', null);
        assert.equal(out, 'OpenROAD - myblock');
    });
});
