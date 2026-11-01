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
    TILE_SIZE_CSS, STATIC_TILE_SIZE_CSS, quantizeDpr, tileDevicePx,
    tileSizeCss, useStaticTileSize, useDeviceExactTileSize,
    withDeviceExactTileSize, buildTileRequestFor, tileSizeFields,
} from '../../src/tile-request.js';
import { deviceResidualCss } from '../../src/device-pixels.js';

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
    // can be on the grid however the panes are positioned.
    it('holds for the tile size the viewer uses', () => {
        for (const dpr of RATIOS) {
            assertWhole(TILE_SIZE_CSS * dpr, `${TILE_SIZE_CSS} css @${dpr}`);
        }
    });

    it('holds for every boundary across a screenful of tiles', () => {
        for (const dpr of RATIOS) {
            for (const k of [1, 2, 3, 7, 12]) {
                assertWhole(k * TILE_SIZE_CSS * dpr, `tile ${k} @${dpr}`);
            }
        }
    });

    it('did NOT hold at 256, which is the reported bug', () => {
        // Kept as a test so the tile size cannot drift back to a value that
        // looks tidier and reintroduces the seams.
        const broken = RATIOS.filter(
            (dpr) => Math.abs(256 * dpr - Math.round(256 * dpr)) >= LAYOUT_UNIT);
        assert.deepEqual(broken, [1.1, 1.2, 1.3333333333333333,
                                  1.6666666269302368, 1.8333333333333333]);
    });
});

describe('dpr contract: the image is exactly the box it goes in', () => {
    // Failure 2. The request states the pixel count; nothing downstream may
    // re-derive it from the rounded dpr.
    it('asks for the pixel count the tile will occupy', () => {
        for (const dpr of RATIOS) {
            const req = buildTileRequestFor({ x: 1, y: 2, z: 3 }, 'metal1',
                                            { visibility: {} }, dpr,
                                            TILE_SIZE_CSS);
            assert.equal(req.tile_px, Math.round(TILE_SIZE_CSS * dpr),
                         `dpr ${dpr}`);
            assert.ok(Number.isInteger(req.tile_px));
        }
    });

    it('asks for a count that fills the box to within a layout unit', () => {
        for (const dpr of RATIOS) {
            const boxDevicePx = TILE_SIZE_CSS * dpr;
            const asked = tileDevicePx(TILE_SIZE_CSS, dpr);
            assert.ok(Math.abs(asked - boxDevicePx) < LAYOUT_UNIT,
                      `dpr ${dpr}: asked ${asked} for a ${boxDevicePx} box`);
        }
    });

    it('would have asked for the wrong size via the old 256*quantized-dpr', () => {
        // 1.6666666 -> 1.67 -> 428 px into a 426.67 px box: the 0.3% resample
        // that softened every tile edge.
        const dpr = 1.6666666269302368;
        const old = Math.round(256 * quantizeDpr(dpr));
        assert.equal(old, 428);
        assert.ok(Math.abs(old - 256 * dpr) > 1);
    });

    it('keeps the merged canvas backing store equal to what it requested', () => {
        // The merged path sizes its canvas with tileDevicePx and requests with
        // tileDevicePx; if those ever diverge, drawImage resamples every tile
        // into the wrong backing store.
        for (const dpr of RATIOS) {
            const requested = buildTileRequestFor(
                { x: 0, y: 0, z: 0 }, 'metal1', { visibility: {} }, dpr,
                TILE_SIZE_CSS).tile_px;
            assert.equal(tileDevicePx(TILE_SIZE_CSS, dpr), requested,
                         `dpr ${dpr}`);
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
            const layer = buildTileRequestFor({ x: 1, y: 2, z: 3 }, 'metal1',
                                              { visibility: {} }, dpr,
                                              TILE_SIZE_CSS);
            assert.equal(fields.tile_px, Math.round(TILE_SIZE_CSS * dpr),
                         `dpr ${dpr}`);
            assert.equal(layer.tile_px, fields.tile_px, `layer @${dpr}`);
            assert.equal(layer.dpr, fields.dpr, `layer dpr @${dpr}`);
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
        assert.equal(quantizeDpr(NaN), 1);
        assert.equal(quantizeDpr(undefined), 1);
    });

    it('rounds the field but never the pixel count', () => {
        // The whole point of tile_px. Rounding the ratio first and multiplying
        // after cannot land on the box: 1.6666666 -> 1.67 asks for 401 px to
        // fill 400, and the browser resamples every tile to make up the
        // difference. The payload carries the rounded ratio AND the exact count.
        for (const dpr of RATIOS) {
            const req = buildTileRequestFor({ x: 0, y: 0, z: 0 }, 'metal1',
                                            { visibility: {} }, dpr,
                                            TILE_SIZE_CSS);
            assert.equal(req.dpr, quantizeDpr(dpr), `dpr field @${dpr}`);
            assert.equal(req.tile_px, Math.round(TILE_SIZE_CSS * dpr),
                         `dpr ${dpr}: pixel count must follow the real ratio`);
            assertWhole(req.tile_px - TILE_SIZE_CSS * dpr,
                        `dpr ${dpr}: count vs box`);
        }
    });

    it('is what a quantized ratio would have got wrong', () => {
        // Regression guard for the flaw the matrix caught: tile_px derived from
        // the rounded ratio is one pixel off at 5/3 and at 4/3.
        for (const dpr of [1.6666666269302368, 1.3333333333333333]) {
            const fromRounded = Math.round(TILE_SIZE_CSS * quantizeDpr(dpr));
            const fromReal = Math.round(TILE_SIZE_CSS * dpr);
            assert.notEqual(fromRounded, fromReal, `dpr ${dpr}`);
            const req = buildTileRequestFor({ x: 0, y: 0, z: 0 }, 'metal1',
                                            { visibility: {} }, dpr,
                                            TILE_SIZE_CSS);
            assert.equal(req.tile_px, fromReal);
        }
    });
});

describe('dpr contract: the residual phase is removable', () => {
    // With a whole pitch, every boundary shares one fractional offset, so a
    // single correction per layer puts all of them on the grid. That is what
    // makes device-pixels.js sufficient — at a fractional pitch it could not be.
    it('is one shared residual, not one per tile', () => {
        for (const dpr of RATIOS) {
            const paneOffset = 37;  // Leaflet rounds offsets to whole CSS px
            const correction = deviceResidualCss(paneOffset, dpr);
            for (const k of [0, 1, 5, 40]) {
                const boundary = (paneOffset - correction + k * TILE_SIZE_CSS)
                                 * dpr;
                assertWhole(boundary, `dpr ${dpr}, tile ${k}`);
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
            // The layers take it from here...
            assert.equal(withDeviceExactTileSize({}).tileSize,
                         STATIC_TILE_SIZE_CSS);
            // ...and so does the request, so the baked image is asked for at
            // its own size rather than the live one.
            const req = buildTileRequestFor({ x: 0, y: 0, z: 0 }, 'metal1',
                                            { visibility: {} }, 1);
            assert.equal(req.tile_px, STATIC_TILE_SIZE_CSS);
        } finally {
            // Module state: restore it so test order cannot matter.
            useDeviceExactTileSize();
        }
    });
});

describe('dpr contract: an explicit size overrides the default', () => {
    it('lets a layer state its own tile size', () => {
        assert.equal(withDeviceExactTileSize({ tileSize: 512 }).tileSize, 512);
        assert.equal(withDeviceExactTileSize({}).tileSize, tileSizeCss());
        assert.equal(withDeviceExactTileSize().tileSize, tileSizeCss());
    });

    it('leaves the other Leaflet options alone', () => {
        const opts = withDeviceExactTileSize({ opacity: 0.7, zIndex: 3 });
        assert.equal(opts.opacity, 0.7);
        assert.equal(opts.zIndex, 3);
    });
});
