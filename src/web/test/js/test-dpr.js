// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The device-pixel-ratio contract, end to end.
//
// Seams along tile boundaries were three separate dpr failures compounding, and
// each one was invisible until a display with the wrong ratio came along:
//
//   1. the tile's CSS size was not a whole number of device pixels, so the tile
//      PITCH was fractional and no placement could put every boundary on the
//      device grid;
//   2. the pixel count was derived from a two-decimal dpr instead of being
//      stated, so the server rendered an image that was the wrong size for the
//      box and the browser resampled every tile;
//   3. nothing re-requested tiles when the ratio changed, so a window moved
//      between monitors kept displaying images baked for the old ratio.
//
// The ratios below are the ones that actually occur — display scaling, browser
// zoom, and the two multiplied — so a change that only works at dpr 1 and 2
// cannot pass. Each test states one invariant of the pipeline and checks it
// across the whole matrix rather than at a single convenient ratio.

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';

import {
    TILE_SIZE_CSS, STATIC_TILE_SIZE_CSS, nativeDpr, quantizeDpr, tileDevicePx,
    tileSizeCss, useStaticTileSize, useDeviceExactTileSize,
    tileRequestBuilder, tileSizeFields, watchDevicePixelRatio,
} from '../../src/tile-request.js';
import { buildHeatmapRequest, buildOverlayRequest, createCompositeSource }
    from '../../src/tile-sources.js';
import { project, snapTarget } from '../../src/view-math.js';

// What browsers report. 1.6666666269302368 is verbatim from the display this
// bug was found on: a float32 5/3, which is why nothing here is bit-exact.
const RATIOS = [
    1,                     // no scaling
    1.1,                   // 110% browser zoom
    1.2,                   // 120%
    1.25,                  // 125% display scaling
    1.3333333333333333,    // 133%, or 166% display at 80% zoom
    1.375,                 // 125% display at 110% zoom
    1.5,                   // 150% display scaling
    1.6666666269302368,    // 166% display scaling — the reported case
    1.75,                  // 175% display scaling
    1.8333333333333333,    // 166% display at 110% zoom
    2,                     // 200%
    2.5,                   // 250%
    3,                     // 300%
];

// The browser lays out on a 1/64 device-pixel grid, so "whole" means whole to
// within a layout unit; a float32 ratio cannot do better than that anyway.
const LAYOUT_UNIT = 1 / 64;

function assertWhole(devicePx, message) {
    assert.ok(Math.abs(devicePx - Math.round(devicePx)) < LAYOUT_UNIT,
              `${message}: ${devicePx} is not a whole device pixel`);
}

describe('dpr contract: the tile pitch is whole at every ratio', () => {
    // Failure 1. A fractional pitch cannot be fixed downstream: boundaries land
    // at pitch*k, so if the pitch has a fraction only every 1/frac-th boundary
    // can be on the grid however the view is positioned.
    it('holds for the tile size the viewer uses', () => {
        for (const dpr of RATIOS) {
            assertWhole(TILE_SIZE_CSS * dpr, `${TILE_SIZE_CSS} css @${dpr}`);
        }
    });

});

describe('dpr contract: the image is exactly the box it goes in', () => {
    // Failure 2. The request states the pixel count; nothing downstream may
    // re-derive it from the rounded dpr.
    it('asks for a count that fills the box to within a layout unit', () => {
        for (const dpr of RATIOS) {
            const boxDevicePx = TILE_SIZE_CSS * dpr;
            const asked = tileDevicePx(TILE_SIZE_CSS, dpr);
            assert.ok(Math.abs(asked - boxDevicePx) < LAYOUT_UNIT,
                      `dpr ${dpr}: asked ${asked} for a ${boxDevicePx} box`);
        }
    });
});

describe('dpr contract: all three tile kinds are sized alike', () => {
    // Layer, overlay and heat-map tiles are stacked on each other in the
    // browser. If they disagree about the pixel count, the upper ones are
    // rescaled by the browser -- blurry, and half a pixel off the shapes they
    // annotate. tileSizeFields is the single place that decides, so the test is
    // that every kind goes through it and gets the same answer.
    it('gives every kind the same count at every ratio', () => {
        for (const dpr of RATIOS) {
            const fields = tileSizeFields(dpr, TILE_SIZE_CSS);
            const c = { x: 1, y: 2, z: 3 };
            const kinds = {
                layer: tileRequestBuilder(c, { visibility: {} }, dpr,
                                          TILE_SIZE_CSS)('metal1'),
                overlay: buildOverlayRequest(c, dpr, TILE_SIZE_CSS, {}, null),
                'heat map': buildHeatmapRequest(c, dpr, TILE_SIZE_CSS,
                                                { activeHeatMap: 'Placement' }),
            };
            assert.equal(fields.tile_px, Math.round(TILE_SIZE_CSS * dpr),
                         `dpr ${dpr}`);
            for (const [kind, req] of Object.entries(kinds)) {
                assert.equal(req.tile_px, fields.tile_px, `${kind} @${dpr}`);
                assert.equal(req.dpr, fields.dpr, `${kind} dpr @${dpr}`);
            }
        }
    });

    it('carries both the rounded ratio and the exact count', () => {
        // The server needs both: the count sizes the image, the ratio scales
        // what is authored in CSS px (label heights, overlay pen widths).
        const fields = tileSizeFields(1.6666666269302368, TILE_SIZE_CSS);
        assert.equal(fields.tile_px, 400);
        assert.equal(fields.dpr, 1.67);
    });

    it('follows the tile size it is given, so a static report stays 256', () => {
        assert.equal(tileSizeFields(1, 256).tile_px, 256);
        assert.equal(tileSizeFields(2, 256).tile_px, 512);
        assert.equal(tileSizeFields(1.6666666269302368, 240).tile_px, 400);
    });
});

describe('dpr contract: quantizeDpr matches the server', () => {
    // MUST mirror quantizeDpr() in request_handler.cpp — it is half of the
    // cache key, and a mismatch means the two sides disagree about what was
    // rendered.
    it('clamps to [1,3] and rounds to two decimals', () => {
        assert.equal(quantizeDpr(1.6666666269302368), 1.67);
        assert.equal(quantizeDpr(1), 1);
        assert.equal(quantizeDpr(0.5), 1);
        assert.equal(quantizeDpr(4), 3);
        assert.equal(quantizeDpr(2.555), 2.56);
        assert.equal(quantizeDpr(4 / 3), 1.33);
        assert.equal(quantizeDpr(1.005), 1.0);
        assert.equal(quantizeDpr(3.0001), 3);
        assert.equal(quantizeDpr(0), 1);
        assert.equal(quantizeDpr(-2), 1);
        assert.equal(quantizeDpr(NaN), 1);
        assert.equal(quantizeDpr(undefined), 1);
    });

    // 1.75 and 2.5 are ordinary display scale factors.
    it('keeps the display ratio instead of snapping to a ladder', () => {
        for (const d of [1, 1.25, 1.5, 1.75, 2, 2.5, 3]) {
            assert.equal(quantizeDpr(d), d);
        }
    });

    it('rounds the field but never the pixel count', () => {
        // The whole point of tile_px. Rounding the ratio first and multiplying
        // after cannot land on the box: 1.6666666 -> 1.67 asks for 401 px to
        // fill 400, and the browser resamples every tile to make up the
        // difference. The payload carries the rounded ratio AND the exact count.
        for (const dpr of RATIOS) {
            const req = tileRequestBuilder(
                { x: 0, y: 0, z: 0 }, { visibility: {} }, dpr,
                TILE_SIZE_CSS)('metal1');
            assert.equal(req.dpr, quantizeDpr(dpr), `dpr field @${dpr}`);
            assert.equal(req.tile_px, Math.round(TILE_SIZE_CSS * dpr),
                         `dpr ${dpr}: pixel count must follow the real ratio`);
            assertWhole(req.tile_px - TILE_SIZE_CSS * dpr,
                        `dpr ${dpr}: count vs box`);
        }
    });
});

describe('dpr contract: the residual phase is removable', () => {
    // With a whole pitch, every boundary shares one fractional offset, so
    // snapping the view's target once puts all of them on the grid.
    it('is one shared residual, not one per tile', () => {
        for (const dpr of RATIOS) {
            for (const zoom of [0, 2, 5]) {
                const vs = { width: Math.floor(1003 * dpr) / dpr,
                             height: Math.floor(797 * dpr) / dpr,
                             zoom, target: [37.3, 91.7] };
                vs.target = snapTarget(vs, dpr);
                const pitch = TILE_SIZE_CSS / 2 ** zoom;   // world units
                for (const k of [0, 1, 5, 40]) {
                    const [x, y] = project([k * pitch, k * pitch], vs);
                    assertWhole(x * dpr, `dpr ${dpr}, z ${zoom}, tile ${k} x`);
                    assertWhole(y * dpr, `dpr ${dpr}, z ${zoom}, tile ${k} y`);
                }
            }
        }
    });
});

describe('dpr contract: static reports keep their baked size', () => {
    // A report's tiles are 256 px images embedded in the file, so the viewer
    // cannot pick a different box for them without resampling every one.
    it('switches every consumer of the size at once', () => {
        assert.equal(tileSizeCss(), TILE_SIZE_CSS);
        useStaticTileSize();
        try {
            assert.equal(tileSizeCss(), STATIC_TILE_SIZE_CSS);
            // The source takes it from here, so the baked image is asked for
            // at its own size rather than the live one.
            const sent = [];
            const wm = {
                request(msg) {
                    sent.push(msg);
                    return new Promise(() => {});
                },
            };
            createCompositeSource({
                wm, ctx: { visibility: {} }, dpr: () => 1,
                getItems: () => [{ layer: 'metal1', opacity: 1 }],
            }).load({ z: 0, x: 0, y: 0 });
            assert.equal(sent[0].tile_px, STATIC_TILE_SIZE_CSS);
        } finally {
            // Module state: restore it so test order cannot matter.
            useDeviceExactTileSize();
        }
    });
});

describe('watchDevicePixelRatio', () => {
    function fakeWindow(dpr) {
        const queries = [];
        return {
            devicePixelRatio: dpr,
            queries,
            matchMedia(query) {
                const mql = { query, handlers: [], removed: [] };
                mql.addEventListener = (name, fn) => {
                    assert.equal(name, 'change');
                    mql.handlers.push(fn);
                };
                mql.removeEventListener = (name, fn) => {
                    assert.equal(name, 'change');
                    mql.removed.push(fn);
                };
                queries.push(mql);
                return mql;
            },
        };
    }

    function withWindow(win, fn) {
        const saved = globalThis.window;
        globalThis.window = win;
        try {
            return fn();
        } finally {
            globalThis.window = saved;
        }
    }

    it('watches the ratio in force', () => {
        const win = fakeWindow(1.25);
        withWindow(win, () => watchDevicePixelRatio(() => {}));
        assert.equal(win.queries.length, 1);
        assert.ok(win.queries[0].query.includes('1.25dppx'));
    });

    it('reports the new ratio and watches for the next one', () => {
        const win = fakeWindow(1.25);
        const seen = [];
        withWindow(win, () => {
            watchDevicePixelRatio((dpr) => seen.push(dpr));
            win.devicePixelRatio = 1.6666666269302368;
            win.queries[0].handlers[0]();
        });
        assert.deepEqual(seen, [1.6666666269302368]);
        // A new query: the old one only ever matches the old ratio.
        assert.equal(win.queries.length, 2);
        assert.ok(win.queries[1].query.includes('1.6666666269302368dppx'));
    });

    it('stops reporting and detaches its listener once unsubscribed', () => {
        const win = fakeWindow(1.25);
        const seen = [];
        withWindow(win, () => {
            const stop = watchDevicePixelRatio((dpr) => seen.push(dpr));
            stop();
            win.queries[0].handlers[0]();
        });
        assert.deepEqual(seen, []);
        assert.deepEqual(win.queries[0].removed, [win.queries[0].handlers[0]]);
    });

    it('is a no-op where matchMedia is unavailable', () => {
        withWindow({ devicePixelRatio: 2 }, () => {
            assert.equal(typeof watchDevicePixelRatio(() => {}), 'function');
        });
    });
});

describe('nativeDpr', () => {
    function withDpr(value) {
        const saved = globalThis.window;
        globalThis.window = value === undefined ? {} : { devicePixelRatio: value };
        try {
            return nativeDpr();
        } finally {
            globalThis.window = saved;
        }
    }

    // Sizes are computed from it, so it is not rounded.
    it('keeps the ratio exact', () => {
        assert.equal(withDpr(2), 2);
        assert.equal(withDpr(1.75), 1.75);
        assert.equal(withDpr(1.6666666269302368), 1.6666666269302368);
    });

    // A tile is rendered at about (tileSize*dpr)^2 * 4 bytes.
    it('clamps to [1, 3], and is 1 without a ratio', () => {
        assert.equal(withDpr(8), 3);
        assert.equal(withDpr(0.5), 1);
        assert.equal(withDpr(undefined), 1);
    });
});
