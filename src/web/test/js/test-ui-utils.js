// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { computeScaleBar, cssColorToHex, niceRoundParts, isValidHexColor }
    from '../../src/ui-utils.js';

describe('isValidHexColor', () => {
    it('accepts #rrggbb', () => {
        assert.equal(isValidHexColor('#0a0a0a'), true);
        assert.equal(isValidHexColor('#FFFFFF'), true);
    });
    it('rejects malformed / short / non-strings', () => {
        assert.equal(isValidHexColor('#fff'), false);       // 3-digit
        assert.equal(isValidHexColor('111111'), false);     // no #
        assert.equal(isValidHexColor('#gggggg'), false);    // non-hex
        assert.equal(isValidHexColor(''), false);
        assert.equal(isValidHexColor(null), false);
        assert.equal(isValidHexColor(undefined), false);
    });
});

describe('cssColorToHex', () => {
    it('normalizes the forms a CSS custom property can hold', () => {
        assert.equal(cssColorToHex('#111'), '#111111');       // 3-digit hex
        assert.equal(cssColorToHex('#a1B2c3'), '#a1b2c3');    // 6-digit hex
        assert.equal(cssColorToHex(' #111 '), '#111111');     // padded
        assert.equal(cssColorToHex('rgb(17, 17, 17)'), '#111111');
        assert.equal(cssColorToHex('rgb(255,0,10)'), '#ff000a');
    });
    it('returns null for unrecognized values', () => {
        assert.equal(cssColorToHex(''), null);
        assert.equal(cssColorToHex('red'), null);
        assert.equal(cssColorToHex('rgba(1, 2, 3, 0.5)'), null);
        assert.equal(cssColorToHex('rgb(999, 0, 0)'), null);  // out of range
        assert.equal(cssColorToHex(null), null);
        assert.equal(cssColorToHex(undefined), null);
    });
});

describe('niceRoundParts', () => {
    it('rounds to 1/2/5/10 x 10^n with the leading digit', () => {
        assert.deepEqual(niceRoundParts(1), { value: 1, digit: 1 });
        assert.deepEqual(niceRoundParts(2), { value: 2, digit: 2 });
        assert.deepEqual(niceRoundParts(6), { value: 5, digit: 5 });
        assert.deepEqual(niceRoundParts(9), { value: 10, digit: 10 });
        assert.deepEqual(niceRoundParts(60), { value: 50, digit: 5 });
        assert.deepEqual(niceRoundParts(120), { value: 100, digit: 1 });
    });
});

describe('computeScaleBar', () => {
    it('returns null for non-drawable inputs', () => {
        assert.equal(
            computeScaleBar({ targetPx: 60, pxPerDbu: 0, dbuPerMicron: 1000 }),
            null);
        assert.equal(
            computeScaleBar({ targetPx: 0, pxPerDbu: 1, dbuPerMicron: 1000 }),
            null);
        assert.equal(
            computeScaleBar({ targetPx: 60, pxPerDbu: NaN, dbuPerMicron: 1000 }),
            null);
    });

    it('DBU mode: nice length, integer label, no unit', () => {
        const sb = computeScaleBar(
            { targetPx: 60, pxPerDbu: 1, dbuPerMicron: 1000, showDbu: true });
        assert.equal(sb.barPx, 50);
        assert.equal(sb.label, '50');
        assert.equal(sb.segments, 5);  // leading digit 5
    });

    it('metric mode: micron label', () => {
        // pxPerUm = pxPerDbu * dbuPerMicron = 0.001 * 1000 = 1.
        const sb = computeScaleBar(
            { targetPx: 60, pxPerDbu: 0.001, dbuPerMicron: 1000 });
        assert.equal(sb.label, '50 µm');
        assert.equal(sb.barPx, 50);
        assert.equal(sb.segments, 5);
    });

    it('unit switches across magnitudes (mm / nm / pm)', () => {
        // pxPerUm = 1 in every case; vary targetPx.
        const mm = computeScaleBar(
            { targetPx: 1000, pxPerDbu: 0.001, dbuPerMicron: 1000 });
        assert.equal(mm.label, '1 mm');

        const nm = computeScaleBar(
            { targetPx: 0.6, pxPerDbu: 0.001, dbuPerMicron: 1000 });
        assert.equal(nm.label, '500 nm');

        const pm = computeScaleBar(
            { targetPx: 0.0006, pxPerDbu: 0.001, dbuPerMicron: 1000 });
        assert.equal(pm.label, '500 pm');
    });

    it('segment count follows the leading digit (2 -> 2)', () => {
        const sb = computeScaleBar(
            { targetPx: 20, pxPerDbu: 1, dbuPerMicron: 1000, showDbu: true });
        assert.equal(sb.label, '20');
        assert.equal(sb.segments, 2);
    });

    it('falls back to 1000 dbu/micron when unset', () => {
        const sb = computeScaleBar({ targetPx: 60, pxPerDbu: 0.001 });
        assert.equal(sb.label, '50 µm');
    });

    it('treats a corrupt dbu/micron like the missing-value fallback', () => {
        // A negative value would otherwise reach niceRoundParts and yield
        // NaN geometry (Math.log10 of a negative is NaN).
        const expected = computeScaleBar({ targetPx: 60, pxPerDbu: 0.001 });
        assert.deepEqual(
            computeScaleBar(
                { targetPx: 60, pxPerDbu: 0.001, dbuPerMicron: -2000 }),
            expected);
        assert.deepEqual(
            computeScaleBar(
                { targetPx: 60, pxPerDbu: 0.001, dbuPerMicron: NaN }),
            expected);
    });
});
