// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';

const {
    project, unproject, viewWorldRect, fitView, zoomAround, tileZoomFor,
    tileRange, growRange, inRange, rangeTiles, tileWorldBounds, snapTarget,
    rectsOverlap,
    flyPath,
} = await import('../../src/view-math.js');

const vs = (over = {}) => ({ target: [120, 120], zoom: 0, width: 800,
                             height: 600, ...over });
const close = (a, b, eps = 1e-9) => Math.abs(a - b) <= eps;

describe('project / unproject', () => {
    it('puts the target at the centre of the view', () => {
        assert.deepEqual(project([120, 120], vs()), [400, 300]);
    });

    // World y is up, screen y is down.
    it('maps world up to screen up', () => {
        const [, py] = project([120, 130], vs());
        assert.equal(py, 290);
    });

    it('scales by 2^zoom', () => {
        assert.deepEqual(project([121, 120], vs({ zoom: 3 })), [408, 300]);
    });

    it('round-trips', () => {
        const v = vs({ zoom: 2.5, target: [10.25, -3.5] });
        const [wx, wy] = unproject(project([7.125, 19.5], v), v);
        assert.ok(close(wx, 7.125) && close(wy, 19.5));
    });

    it('reports the world rect on show', () => {
        assert.deepEqual(viewWorldRect(vs()), [-280, -180, 520, 420]);
    });
});

describe('fitView', () => {
    // The view rests on whole levels, so the fitted zoom is floored.
    it('floors to a whole zoom level and centres the rect', () => {
        const fit = fitView([0, 0, 100, 100], vs());
        assert.equal(fit.zoom, 2);     // 600/100 = 6 -> log2 = 2.58 -> 2
        assert.deepEqual(fit.target, [50, 50]);
    });

    it('keeps the padding clear on both sides', () => {
        // 600 - 2*100 = 400 -> 4x -> zoom 2 exactly.
        assert.equal(fitView([0, 0, 100, 100], vs(),
                             { padding: [100, 100] }).zoom, 2);
        assert.equal(fitView([0, 0, 100, 100], vs(),
                             { padding: [101, 101] }).zoom, 1);
    });

    // The fit rounds to 1/100 of a level before flooring, so a fit
    // a hair short of a whole level does not lose it.
    it('does not lose a level to rounding noise', () => {
        const rect = [0, 0, 600 / (4 * 1.00001), 1];
        assert.equal(fitView(rect, vs({ height: 1e6 })).zoom, 2);
    });

    it('honours the zoom limits', () => {
        assert.equal(fitView([0, 0, 1e-3, 1e-3], vs(), { maxZoom: 12 }).zoom, 12);
        assert.equal(fitView([0, 0, 1e6, 1e6], vs(), { minZoom: -2 }).zoom, -2);
    });
});

describe('zoomAround', () => {
    it('keeps the world point under the cursor', () => {
        const v = vs({ zoom: 1 });
        const p = [650, 120];
        const before = unproject(p, v);
        const target = zoomAround(v, p, 2);
        const after = unproject(p, { ...v, target, zoom: 2 });
        assert.ok(close(before[0], after[0]) && close(before[1], after[1]));
    });
});

describe('tileZoomFor (upscale-only, #10463)', () => {
    it('floors the view zoom', () => {
        for (const [zoom, want] of [[3, 3], [3.2, 3], [3.999, 3], [-0.5, -1]]) {
            assert.equal(tileZoomFor(zoom), want);
        }
    });

    it('does not lose a level to float noise at rest', () => {
        assert.equal(tileZoomFor(2.9999999999), 3);
    });

    // A level the swap keeps hidden is never shown downscaled, so it may load
    // for the target from the start of the animation.
    it('uses the target only while new levels stay hidden', () => {
        assert.equal(tileZoomFor(3.4, { targetZoom: 4 }), 3);
        assert.equal(tileZoomFor(3.4, { targetZoom: 4,
                                        hidingNewLevels: true }), 4);
    });
});

describe('tileRange', () => {
    // The server's grid: tile (z, x, y) has y counting down from the top of the
    // [0, T] square, so tile (1, 0, 0) is the top-left quarter.
    it('uses the server top-down rows', () => {
        assert.deepEqual(tileRange([0, 130, 100, 230], 1, 240),
                         { z: 1, x0: 0, x1: 0, y0: 0, y1: 0 });
        assert.deepEqual(tileRange([0, 10, 100, 100], 1, 240),
                         { z: 1, x0: 0, x1: 0, y0: 1, y1: 1 });
    });

    it('clamps to the grid and drops a rect that misses it', () => {
        assert.deepEqual(tileRange([-500, -500, 500, 500], 1, 240),
                         { z: 1, x0: 0, x1: 1, y0: 0, y1: 1 });
        assert.equal(tileRange([300, 300, 400, 400], 2, 240), null);
    });

    // A rect edge exactly on a tile edge does not pull in the next tile
    // (ceil - 1).
    it('stops at an exact tile edge', () => {
        assert.deepEqual(tileRange([0, 0, 120, 240], 1, 240),
                         { z: 1, x0: 0, x1: 0, y0: 0, y1: 1 });
    });

    it('agrees with tileWorldBounds', () => {
        for (let x = 0; x < 4; x++) {
            for (let y = 0; y < 4; y++) {
                const b = tileWorldBounds(2, x, y, 240);
                const inner = [b[0] + 1, b[1] + 1, b[2] - 1, b[3] - 1];
                assert.deepEqual(tileRange(inner, 2, 240),
                                 { z: 2, x0: x, x1: x, y0: y, y1: y });
            }
        }
    });
});

describe('growRange / inRange / rangeTiles', () => {
    it('grows by one tile, clamped to the grid', () => {
        assert.deepEqual(growRange({ z: 2, x0: 0, x1: 1, y0: 2, y1: 3 }),
                         { z: 2, x0: 0, x1: 2, y0: 1, y1: 3 });
        assert.equal(growRange(null), null);
    });

    it('tests membership', () => {
        const r = { z: 2, x0: 1, x1: 2, y0: 1, y1: 2 };
        assert.ok(inRange(r, 1, 2));
        assert.ok(!inRange(r, 0, 2));
        assert.ok(!inRange(null, 1, 1));
    });

    it('lists the range nearest the centre first', () => {
        const tiles = rangeTiles({ z: 3, x0: 0, x1: 2, y0: 0, y1: 2 });
        assert.equal(tiles.length, 9);
        assert.deepEqual(tiles[0], { z: 3, x: 1, y: 1 });
    });
});

describe('tileWorldBounds', () => {
    it('is [left, bottom, right, top] with row 0 at the top', () => {
        assert.deepEqual(tileWorldBounds(0, 0, 0, 240), [0, 0, 240, 240]);
        assert.deepEqual(tileWorldBounds(1, 1, 0, 240), [120, 120, 240, 240]);
        assert.deepEqual(tileWorldBounds(1, 0, 1, 240), [0, 0, 120, 120]);
    });
});

describe('rectsOverlap', () => {
    it('is strict on touching edges', () => {
        assert.ok(rectsOverlap([0, 0, 2, 2], [1, 1, 3, 3]));
        assert.ok(!rectsOverlap([0, 0, 1, 1], [1, 0, 2, 1]));
    });
});

describe('snapTarget', () => {
    // At rest every tile edge must sit on a whole device pixel, which holds
    // once world (0, 0) does: tile sizes are whole device pixels.
    for (const dpr of [1, 1.25, 1.5, 2]) {
        it(`puts world (0,0) on a device pixel at dpr ${dpr}`, () => {
            const v = vs({ target: [100.37, 77.123], zoom: 3, width: 801,
                           height: 603 });
            const snapped = { ...v, target: snapTarget(v, dpr) };
            const [px, py] = project([0, 0], snapped);
            assert.ok(close(px * dpr, Math.round(px * dpr), 1e-6));
            assert.ok(close(py * dpr, Math.round(py * dpr), 1e-6));
        });
    }

    it('moves the view by less than a device pixel', () => {
        const v = vs({ target: [100.37, 77.123], zoom: 3 });
        const t = snapTarget(v, 2);
        const shift = Math.hypot(t[0] - v.target[0], t[1] - v.target[1]) * 8 * 2;
        assert.ok(shift <= Math.SQRT2 * 0.5 + 1e-9);
    });
});

describe('flyPath', () => {
    const a = { target: [10, 10], zoom: 6, width: 800, height: 600 };
    const b = { target: [230, 200], zoom: 6, width: 800, height: 600 };
    const near = (x, y, tol = 1e-6) => Math.abs(x - y) < tol;

    it('starts and ends on the two views', () => {
        const p = flyPath(a, b);
        const s = p.at(0);
        const e = p.at(1);
        assert.ok(near(s.zoom, 6) && near(s.target[0], 10) && near(s.target[1], 10));
        assert.ok(near(e.zoom, 6) && near(e.target[0], 230) && near(e.target[1], 200));
    });

    it('pulls out to travel far, then closes in', () => {
        const p = flyPath(a, b);
        const mid = p.at(0.5);
        assert.ok(mid.zoom < 3, `apex at zoom ${mid.zoom}`);
        assert.ok(p.S > 1);
        // Monotone travel along the line between the two targets.
        let last = -Infinity;
        for (let i = 0; i <= 10; i++) {
            const x = p.at(i / 10).target[0];
            assert.ok(x >= last - 1e-9);
            last = x;
        }
    });

    it('zooms in place when the targets coincide', () => {
        const p = flyPath(a, { ...a, zoom: 2 });
        assert.ok(near(p.at(0.5).zoom, 4, 1e-9));
        assert.deepEqual(p.at(0.5).target, [10, 10]);
        assert.ok(near(p.S, 4 * Math.LN2 / Math.SQRT2, 1e-9));
    });
});
