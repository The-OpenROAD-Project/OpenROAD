// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { applyArrowStep, boundsEqual, computeBoundsTransforms, computeScaleBar,
         cssColorToHex, fittedTileSizeCss, installWheelPanning, isValidHexColor,
         kArrowStepDefault, kZoomMargin, maxUsefulZoom, MAX_TILE_ZOOM,
         niceRoundParts }
    from '../../src/ui-utils.js';
import { isDeviceExactTileSize, TILE_SIZE_CSS, TILE_SIZE_QUANTUM }
    from '../../src/tile-request.js';

// The 5% Qt margin applied to a rect's dimension, as computeBoundsTransforms
// does before converting it.
const bloated = (extent) => extent * (1 + 2 * kZoomMargin);

// deepEqual on latlngs the margin arithmetic has been through: the same value
// reached two ways differs in the last bits.
function assertClose(actual, expected) {
    assert.equal(actual.length, expected.length);
    for (let i = 0; i < expected.length; i++) {
        for (let j = 0; j < expected[i].length; j++) {
            assert.ok(Math.abs(actual[i][j] - expected[i][j]) < 1e-9,
                      `[${i}][${j}]: ${actual[i][j]} != ${expected[i][j]}`);
        }
    }
}

describe('computeBoundsTransforms', () => {
    it('derives the tile-grid transforms from a bounds response', () => {
        // ibex-like: block bbox inflated by the pin-label margin.
        const t = computeBoundsTransforms([[-63538, -63538],
                                           [544908, 544908]]);
        assert.equal(t.originX, -63538);
        assert.equal(t.originY, -63538);
        assert.equal(t.maxDXDY, 608446);
        assert.equal(t.scale, 256 / 608446);
        // With no fit rect the georeference rect is fitted, plus the margin.
        const pad = 608446 * kZoomMargin * t.scale;
        assertClose(t.fitBounds, [[-256 - pad, -pad], [pad, 256 + pad]]);
    });

    it('uses the larger dimension for non-square designs', () => {
        const t = computeBoundsTransforms([[0, 0], [100, 400]]);
        assert.equal(t.maxDXDY, 400);
        // fitBounds top edge reflects the smaller height.
        const scale = 256 / 400;
        const padX = 400 * kZoomMargin * scale;
        const padY = 100 * kZoomMargin * scale;
        assertClose(
            t.fitBounds,
            [[-256 - padY, -padX], [(100 - 400) * scale + padY, 256 + padX]]);
    });

    it('frames the fit rect, not the georeference rect', () => {
        // The georeference rect is the design grown by a pin-label margin;
        // framing on it would leave that margin as dead space (issue #11338).
        const geo = [[-100, -100], [1100, 1100]];
        const fit = [[0, 0], [1000, 1000]];
        const t = computeBoundsTransforms(geo, 256, fit);
        // The transforms still come from the georeference rect.
        assert.equal(t.maxDXDY, 1200);
        assert.equal(t.originX, -100);

        const withFit = t.fitBounds[1][1] - t.fitBounds[0][1];
        const withoutFit = computeBoundsTransforms(geo, 256);
        const geoWidth = withoutFit.fitBounds[1][1] - withoutFit.fitBounds[0][1];
        assert.ok(withFit < geoWidth,
                  'the framed box is smaller than the georeference rect');
        assert.equal(withFit, bloated(1000) * t.scale);
    });

    it('leaves 5% of each dimension per side, as Qt does', () => {
        // Qt's zoomTo divides the fitted pixels-per-DBU by (1 + 2*0.05), so
        // the design ends up filling 1/1.1 of the framed box.
        const t = computeBoundsTransforms([[0, 0], [400, 1000]], 256,
                                          [[0, 0], [400, 1000]]);
        const framedW = t.fitBounds[1][1] - t.fitBounds[0][1];
        const framedH = t.fitBounds[1][0] - t.fitBounds[0][0];
        assert.ok(Math.abs(1000 * t.scale / framedW - 1 / 1.1) < 1e-12);
        assert.ok(Math.abs(400 * t.scale / framedH - 1 / 1.1) < 1e-12);
    });

    it('ignores an absent or degenerate fit rect', () => {
        const geo = [[0, 0], [400, 400]];
        const expected = computeBoundsTransforms(geo, 256).fitBounds;
        for (const bad of [null, undefined, [[0, 0], [0, 400]],
                           [[0, 0], [400, 0]]]) {
            assert.deepEqual(computeBoundsTransforms(geo, 256, bad).fitBounds,
                             expected);
        }
    });

    it('returns null for an empty or degenerate design', () => {
        assert.equal(computeBoundsTransforms(null), null);
        assert.equal(computeBoundsTransforms([[0, 0], [0, 0]]), null);
        assert.equal(computeBoundsTransforms([[10, 10], [10, 400]]), null);
    });
});

describe('fittedTileSizeCss', () => {
    // The zoom Leaflet's fitBounds would compute for this tile size, before it
    // floors: the design (plus the 5% margin) filling the constrained axis.
    function fitZoom({ designBounds, fitRect, viewW, viewH }, tileSize) {
        const geoMaxDXDY = Math.max(designBounds[1][1] - designBounds[0][1],
                                    designBounds[1][0] - designBounds[0][0]);
        const fit = fitRect || designBounds;
        const scale = tileSize / geoMaxDXDY;
        return Math.log2(Math.min(viewW / (bloated(fit[1][1] - fit[0][1])
                                           * scale),
                                  viewH / (bloated(fit[1][0] - fit[0][0])
                                           * scale)));
    }

    // A wide design in a wide viewport, the shape of the issue #11338 report.
    const wide = {
        designBounds: [[-3000, -3000], [43000, 103000]],
        fitRect: [[0, 0], [40000, 100000]],
        viewW: 770,
        viewH: 610,
    };

    // What the design covers of the viewport once fitBounds has floored: one
    // whole framed box is 1, and halving the zoom level halves it.
    const fill = (input, size) =>
        2 ** (Math.floor(fitZoom(input, size)) - fitZoom(input, size));

    it('lands the fit zoom just above a whole level', () => {
        // Just above, not exactly on: at exactly the integer the last bit of
        // the float decides whether the floor keeps the level or drops one.
        const size = fittedTileSizeCss(wide);
        const z = fitZoom(wide, size);
        assert.ok(z >= Math.floor(z), `fit zoom ${z} sits below its level`);
        assert.ok(fill(wide, size) > 2 / 3,
                  `design fills only ${fill(wide, size)} at tile size ${size}`);
    });

    it('recovers the level the floor was costing', () => {
        // Only meaningful if the base size did lose most of a level to it.
        assert.ok(fill(wide, TILE_SIZE_CSS) < 0.7);
        const size = fittedTileSizeCss(wide);
        assert.equal(Math.floor(fitZoom(wide, size)),
                     Math.floor(fitZoom(wide, TILE_SIZE_CSS)));
        // Same zoom level, larger tiles: the design covers more of the
        // viewport by exactly the ratio of the two sizes.
        assert.ok(size > TILE_SIZE_CSS,
                  `tile size did not grow: ${size}`);
    });

    it('fills the viewport at every window size', () => {
        // Rounding down to the quantum can only leave the design short of the
        // framed box, and never below the 2/3 the two candidate sizes allow --
        // against the 1/2 floor the bare zoom rounding permits.
        for (const viewW of [300, 480, 770, 1000, 1600, 2560]) {
            for (const viewH of [400, 610, 900, 1440]) {
                const input = { ...wide, viewW, viewH };
                const size = fittedTileSizeCss(input);
                assert.ok(fill(input, size) > 2 / 3,
                          `w=${viewW} h=${viewH}: fills ${fill(input, size)}`);
            }
        }
    });

    it('stays within one doubling of the base size', () => {
        for (const viewW of [300, 481, 777, 1013, 1600, 2561]) {
            const size = fittedTileSizeCss({ ...wide, viewW });
            assert.ok(size >= TILE_SIZE_CSS && size < 2 * TILE_SIZE_CSS,
                      `${size} is outside [240, 480)`);
        }
    });

    it('picks a size whole in device pixels at EVERY ratio, not just now', () => {
        // The panes are built once, so a size chosen for the boot ratio would
        // come apart on the next browser zoom and bring the tile seams back.
        // The ratios test-dpr.js pins the tile pitch against.
        const ratios = [1, 1.1, 1.2, 1.25, 1.3333333333333333, 1.375, 1.5,
                        1.6666666269302368, 1.75, 1.8333333333333333, 2, 2.5,
                        3];
        for (const viewW of [300, 480, 777, 1013, 1600, 2560]) {
            for (const viewH of [400, 610, 900, 1440]) {
                const size = fittedTileSizeCss({ ...wide, viewW, viewH });
                assert.equal(size % TILE_SIZE_QUANTUM, 0,
                             `${size} is not a multiple of the quantum`);
                for (const dpr of ratios) {
                    assert.ok(isDeviceExactTileSize(size, dpr),
                              `${size} css @${dpr} is not a whole device px`);
                }
            }
        }
    });

    it('falls back to the base size on unusable input', () => {
        const bad = [
            { ...wide, designBounds: null },
            { ...wide, designBounds: [[0, 0], [0, 0]] },
            { ...wide, viewW: 0 },
            { ...wide, viewH: NaN },
            // A viewport too small to reach zoom 0: the map is pinned at its
            // minZoom and a bigger tile would only overflow it.
            { ...wide, viewW: 10, viewH: 10 },
        ];
        for (const input of bad) {
            assert.equal(fittedTileSizeCss(input), TILE_SIZE_CSS);
        }
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

// ─── installWheelPanning (Options > mouse-wheel-zoom, 2.15) ─────────────────

// A stand-in for the Leaflet map: DOM-free, so this file stays free of jsdom.
function makeWheelMap({ zoom = 5, clientHeight = 600 } = {}) {
    const calls = { pan: [], zoom: [], disabled: 0, prevented: 0 };
    let listener = null;
    const container = {
        clientHeight,
        addEventListener(type, fn) {
            if (type === 'wheel') listener = fn;
        },
    };
    const map = {
        scrollWheelZoom: { disable() { calls.disabled++; } },
        getContainer: () => container,
        getZoom: () => zoom,
        panBy: (offset) => calls.pan.push(offset),
        setZoomAround: (latlng, z) => calls.zoom.push(z),
        mouseEventToLatLng: () => 'cursor',
    };
    const fire = (e) => listener({
        deltaX: 0, deltaY: 0, deltaMode: 0, ctrlKey: false,
        preventDefault: () => { calls.prevented++; },
        ...e,
    });
    return { map, calls, fire };
}

describe('installWheelPanning', () => {
    // Leaflet's own handler would zoom on top of whatever this one did.
    it('turns off Leaflet\'s wheel zoom and swallows the page scroll', () => {
        const { map, calls, fire } = makeWheelMap();
        installWheelPanning(map, () => true);
        assert.equal(calls.disabled, 1);
        fire({ deltaY: -1 });
        assert.equal(calls.prevented, 1);
    });

    // Qt's LayoutScroll::wheelEvent: pan when the preference and the Ctrl
    // modifier agree, zoom otherwise.
    it('zooms on a bare wheel when the preference is on', () => {
        const { map, calls, fire } = makeWheelMap({ zoom: 5 });
        installWheelPanning(map, () => true);
        fire({ deltaY: -1 });
        fire({ deltaY: 1 });
        assert.deepEqual(calls.zoom, [6, 4]);
        assert.deepEqual(calls.pan, []);
    });

    it('pans on Ctrl+wheel when the preference is on', () => {
        const { map, calls, fire } = makeWheelMap();
        installWheelPanning(map, () => true);
        fire({ deltaY: 120, ctrlKey: true });
        assert.deepEqual(calls.pan, [[0, 120]]);
        assert.deepEqual(calls.zoom, []);
    });

    it('pans on a bare wheel when the preference is off (Qt default)', () => {
        const { map, calls, fire } = makeWheelMap();
        installWheelPanning(map, () => false);
        fire({ deltaY: 120 });
        assert.deepEqual(calls.pan, [[0, 120]]);
        assert.deepEqual(calls.zoom, []);
    });

    it('zooms on Ctrl+wheel when the preference is off', () => {
        const { map, calls, fire } = makeWheelMap({ zoom: 3 });
        installWheelPanning(map, () => false);
        fire({ deltaY: -1, ctrlKey: true });
        assert.deepEqual(calls.zoom, [4]);
    });

    it('reads the preference on every event, not just at install', () => {
        let pref = true;
        const { map, calls, fire } = makeWheelMap();
        installWheelPanning(map, () => pref);
        fire({ deltaY: -1 });
        pref = false;
        fire({ deltaY: -1 });
        assert.equal(calls.zoom.length, 1);
        assert.equal(calls.pan.length, 1);
    });

    it('pans horizontally from deltaX', () => {
        const { map, calls, fire } = makeWheelMap();
        installWheelPanning(map, () => false);
        fire({ deltaX: -40, deltaY: 10 });
        assert.deepEqual(calls.pan, [[-40, 10]]);
    });

    // Firefox reports lines and some remote-desktop stacks report pages;
    // treating either as pixels would make one notch pan a few pixels.
    it('scales line- and page-mode deltas to pixels', () => {
        const { map, calls, fire } = makeWheelMap({ clientHeight: 600 });
        installWheelPanning(map, () => false);
        fire({ deltaY: 3, deltaMode: 1 });
        fire({ deltaY: 1, deltaMode: 2 });
        assert.deepEqual(calls.pan, [[0, 48], [0, 600]]);
    });
});

// ─── applyArrowStep (Options > arrow keys scroll step, 2.15) ────────────────

// A stand-in for Leaflet's Keyboard handler, transcribed from Leaflet 1.9.4
// (Map.Keyboard.js): options.keyboardPanDelta is read once in initialize, and
// _setPanDelta rebuilds _panKeys from its *argument* -- it never re-reads the
// option.  _onKeyDown then pans by whatever _panKeys holds.  Modelling it this
// way means the assertions below are about the distance an arrow press moves,
// not merely about which function got called.
const kLeft = 37, kRight = 39;

function makeKeyboardMap(initialPanDelta) {
    const keyboard = {
        _panKeys: {},
        _setPanDelta(panDelta) {
            this._panKeys = {
                [kLeft]: [-1 * panDelta, 0],
                [kRight]: [panDelta, 0],
            };
        },
    };
    // What Keyboard.initialize does with options.keyboardPanDelta.
    keyboard._setPanDelta(initialPanDelta);
    const map = { keyboard, options: { keyboardPanDelta: initialPanDelta } };
    // What _onKeyDown pans by for a key press.
    const panFor = (key) => keyboard._panKeys[key];
    return { map, panFor };
}

describe('applyArrowStep', () => {
    // The regression this guards: writing only the cookie (or only
    // map.options.keyboardPanDelta) leaves an open viewer panning by the old
    // distance until a reload, because Leaflet reads that option once.
    it('changes the live pan distance, not just the next map', () => {
        const { map, panFor } = makeKeyboardMap(kArrowStepDefault);
        assert.deepEqual(panFor(kRight), [kArrowStepDefault, 0]);

        applyArrowStep(map, 250);
        assert.deepEqual(panFor(kRight), [250, 0], 'right arrow pans by 250');
        assert.deepEqual(panFor(kLeft), [-250, 0], 'left arrow mirrors it');
    });

    // Assigning the option instead would be a dead store; this pins that the
    // helper does not settle for that and leave _panKeys stale.
    it('does not rely on map.options.keyboardPanDelta', () => {
        const { map, panFor } = makeKeyboardMap(kArrowStepDefault);
        map.options.keyboardPanDelta = 999;
        assert.deepEqual(panFor(kRight), [kArrowStepDefault, 0],
                         'the option alone moves nothing');
        applyArrowStep(map, 120);
        assert.deepEqual(panFor(kRight), [120, 0]);
    });

    // A static report builds no keyboard handler, and the Options menu can be
    // driven before the map exists; neither may throw.
    it('is a no-op when there is no map or no keyboard handler', () => {
        assert.doesNotThrow(() => applyArrowStep(null, 100));
        assert.doesNotThrow(() => applyArrowStep(undefined, 100));
        assert.doesNotThrow(() => applyArrowStep({}, 100));
        assert.doesNotThrow(() => applyArrowStep({ keyboard: {} }, 100));
    });
});
