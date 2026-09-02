// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { boundsEqual, computeBoundsTransforms, computeScaleBar, cssColorToHex,
         isValidHexColor, maxUsefulZoom, MAX_TILE_ZOOM, niceRoundParts }
    from '../../src/ui-utils.js';

describe('computeBoundsTransforms', () => {
    it('derives the tile-grid transforms from a bounds response', () => {
        // ibex-like: block bbox inflated by the pin-label margin.
        const t = computeBoundsTransforms([[-63538, -63538],
                                           [544908, 544908]]);
        assert.equal(t.originX, -63538);
        assert.equal(t.originY, -63538);
        assert.equal(t.maxDXDY, 608446);
        assert.equal(t.scale, 256 / 608446);
        assert.deepEqual(t.fitBounds, [[-256, 0], [0, 256]]);
    });

    it('uses the larger dimension for non-square designs', () => {
        const t = computeBoundsTransforms([[0, 0], [100, 400]]);
        assert.equal(t.maxDXDY, 400);
        // fitBounds top edge reflects the smaller height.
        assert.deepEqual(t.fitBounds,
                         [[-256, 0], [(100 - 400) * (256 / 400), 256]]);
    });

    it('returns null for an empty or degenerate design', () => {
        assert.equal(computeBoundsTransforms(null), null);
        assert.equal(computeBoundsTransforms([[0, 0], [0, 0]]), null);
        assert.equal(computeBoundsTransforms([[10, 10], [10, 400]]), null);
    });
});

describe('boundsEqual', () => {
    it('compares all four corners', () => {
        const a = [[0, 0], [100, 100]];
        assert.ok(boundsEqual(a, [[0, 0], [100, 100]]));
        assert.ok(!boundsEqual(a, [[0, 0], [100, 101]]));
        assert.ok(!boundsEqual(a, [[-1, 0], [100, 100]]));
        assert.ok(!boundsEqual(a, null));
        assert.ok(!boundsEqual(null, a));
    });
});

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

describe('maxUsefulZoom', () => {
    // designScale is pixels per DBU at zoom 0 (tileSize / maxDXDY), so these
    // are real die widths: gcd is 71510 DBU across, swerv
    // 962800, microwatt 3610000.
    const scaleFor = (maxDXDY) => 256 / maxDXDY;

    it('caps where one DBU covers the pixel budget', () => {
        // At the cap, designScale * 2^z must be at or just past maxPxPerDbu
        // (8 by default) and one level lower must still be under it.
        for (const maxDXDY of [71510, 962800, 3610000]) {
            const scale = scaleFor(maxDXDY);
            const z = maxUsefulZoom(scale);
            assert.ok(scale * Math.pow(2, z) >= 8,
                      `z=${z} should reach the budget for ${maxDXDY} DBU`);
            assert.ok(scale * Math.pow(2, z - 1) < 8,
                      `z=${z} should be the first level to reach it`);
        }
    });

    it('returns an integer within the server tile-grid ceiling', () => {
        for (const maxDXDY of [888, 71510, 3610000, 1e9]) {
            const z = maxUsefulZoom(scaleFor(maxDXDY));
            assert.equal(z, Math.trunc(z), 'zoom levels are integers');
            assert.ok(z >= 1 && z <= MAX_TILE_ZOOM,
                      `z=${z} outside [1, ${MAX_TILE_ZOOM}]`);
        }
    });

    it('bounds the zoom even before a design is loaded', () => {
        // The whole point is that no path leaves maxZoom at Infinity.
        for (const bad of [undefined, null, 0, -1, NaN, Infinity]) {
            const z = maxUsefulZoom(bad);
            assert.ok(Number.isFinite(z) && z > 0,
                      `maxUsefulZoom(${bad}) must be finite and positive`);
        }
    });

    it('honours a caller-supplied pixel budget', () => {
        const scale = scaleFor(71510);
        assert.ok(maxUsefulZoom(scale, 64) > maxUsefulZoom(scale, 8),
                  'a larger budget allows deeper zoom');
    });
});
