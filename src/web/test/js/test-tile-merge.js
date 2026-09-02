// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';

import {
    BYTES_PER_PIXEL, DEFAULT_BUDGET_BYTES, UNMERGED_PANE_COUNT, tileBytes,
    estimateTilesPerPane, measureViewport, reserveForUnmergedPanes,
    setItemVisible, computeGroupCount,
    partitionIntoGroups, mergeOntoContext, closeAll, describePlan,
    MERGED_PANE_OPACITY, mergedPaneOptions, blendOver, compositeStack,
    renderMergedTile,
} from '../../src/tile-merge.js';

// The measured figures this module is sized against: 97 panes, 24 tiles/pane,
// dpr 1, 1240x999 window => 582 MB of decoded tile images, against a ~458 MB
// ceiling above which the browser starts discarding decodes.
const PANES = 97;
const TILES_PER_PANE = 24;
const TILE_BYTES = 256 * 256 * BYTES_PER_PIXEL;

describe('tile-merge is pure', () => {
    it('exports plain functions with no DOM or Leaflet dependency', () => {
        // Everything here is arithmetic and compositing rules, so it stays
        // testable without a browser. The Leaflet and canvas parts live in
        // merged-tile-layer.js.
        for (const fn of [tileBytes, estimateTilesPerPane, measureViewport,
                          computeGroupCount, partitionIntoGroups,
                          mergeOntoContext, closeAll, describePlan]) {
            assert.equal(typeof fn, 'function');
        }
        assert.equal(typeof DEFAULT_BUDGET_BYTES, 'number');
    });
});

describe('tileBytes', () => {
    it('is the decoded RGBA size, not the PNG size', () => {
        // The browser holds 32 bits per pixel whatever the PNG's bit depth, so
        // compressing harder buys nothing — this is the number that matters.
        assert.equal(tileBytes(256, 1), 256 * 256 * 4);
        assert.equal(tileBytes(256, 1), 262144);
    });

    it('scales with the square of dpr', () => {
        assert.equal(tileBytes(256, 2), 4 * tileBytes(256, 1));
        assert.equal(tileBytes(256, 3), 9 * tileBytes(256, 1));
    });

    it('rounds the device-pixel side length', () => {
        assert.equal(tileBytes(256, 1.5), 384 * 384 * 4);
    });
});

describe('computeGroupCount', () => {
    it('turns the measured budget into a group count', () => {
        // 350 MB / (24 tiles * 256 KB) = 350 / 6 = 58 groups.
        const n = computeGroupCount({
            budgetBytes: 350 * 1024 * 1024,
            tilesPerPane: TILES_PER_PANE,
            bytesPerTile: TILE_BYTES,
            paneCount: PANES,
        });
        assert.equal(n, 58);
        // And that must actually fit under the budget.
        assert.ok(n * TILES_PER_PANE * TILE_BYTES <= 350 * 1024 * 1024);
    });

    it('shrinks N as the window grows, which is the whole point', () => {
        // 2560x1440 needs ~77 tiles/pane instead of 24. A fixed N chosen at one
        // window size would put a maximised 4K window back over the ceiling
        // (97 panes there is ~1.9 GB), so N has to fall as tilesPerPane rises.
        const small = computeGroupCount({
            tilesPerPane: 24, bytesPerTile: TILE_BYTES, paneCount: PANES,
        });
        const large = computeGroupCount({
            tilesPerPane: 77, bytesPerTile: TILE_BYTES, paneCount: PANES,
        });
        assert.ok(large < small, `${large} should be < ${small}`);
        assert.ok(large * 77 * TILE_BYTES <= DEFAULT_BUDGET_BYTES);
    });

    it('shrinks N on a HiDPI display', () => {
        // dpr 2 quadruples the bytes per tile, so N has to fall by about 4x.
        // Asserted as the contract (falls, and still fits) rather than as
        // at1 === 4*at2: flooring does not distribute over division, so the
        // exact identity is false (58 vs 4*14=56) even though the code is right.
        const at1 = computeGroupCount({
            tilesPerPane: 24, bytesPerTile: tileBytes(256, 1), paneCount: 400,
        });
        const at2 = computeGroupCount({
            tilesPerPane: 24, bytesPerTile: tileBytes(256, 2), paneCount: 400,
        });
        assert.ok(at2 < at1, `${at2} should be < ${at1}`);
        assert.ok(at2 <= Math.ceil(at1 / 4), `${at2} vs ~${at1 / 4}`);
        // The contract that actually matters: whatever N is, it fits.
        for (const [n, dpr] of [[at1, 1], [at2, 2]]) {
            assert.ok(n * 24 * tileBytes(256, dpr) <= DEFAULT_BUDGET_BYTES,
                      `N=${n} at dpr ${dpr} exceeds the budget`);
        }
    });

    it('never exceeds the pane count', () => {
        // More groups than panes cannot help and would just rebuild the
        // one-pane-per-layer situation this exists to avoid.
        const n = computeGroupCount({
            budgetBytes: 100 * 1024 * 1024 * 1024,
            tilesPerPane: 1, bytesPerTile: 4, paneCount: 12,
        });
        assert.equal(n, 12);
    });

    it('never returns less than 1', () => {
        // A budget too small for even one group still has to render something;
        // returning 0 would blank the viewer.
        assert.equal(computeGroupCount({
            budgetBytes: 1, tilesPerPane: TILES_PER_PANE,
            bytesPerTile: TILE_BYTES, paneCount: PANES,
        }), 1);
    });

    it('degrades to 1 on missing or nonsense inputs', () => {
        assert.equal(computeGroupCount(), 1);
        assert.equal(computeGroupCount({ tilesPerPane: 0, bytesPerTile: 4,
                                         paneCount: 4 }), 1);
        assert.equal(computeGroupCount({ tilesPerPane: 4, bytesPerTile: 0,
                                         paneCount: 4 }), 1);
        assert.equal(computeGroupCount({ tilesPerPane: 4, bytesPerTile: 4,
                                         paneCount: 0 }), 1);
    });

    it('defaults to a budget with margin under the measured ceiling', () => {
        // 458 MB was where it broke, 462 MB already flickered. A default at or
        // above that would ship the bug.
        assert.ok(DEFAULT_BUDGET_BYTES < 458 * 1024 * 1024);
    });
});

describe('partitionIntoGroups', () => {
    const items = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

    it('produces contiguous runs of the z-order', () => {
        // THE correctness constraint. Each group is rasterized into one image,
        // and two images can only be stacked to reproduce the original if
        // neither interleaves with the other: for z-order [1,2,3,4], the
        // grouping {1,3},{2,4} has NO stacking that yields the original.
        const groups = partitionIntoGroups(items, 3);
        const flat = groups.flat();
        assert.deepEqual(flat, items, 'flattening must restore the z-order');
        for (const g of groups) {
            for (let i = 1; i < g.length; i++) {
                assert.equal(g[i], g[i - 1] + 1,
                             `group ${JSON.stringify(g)} is not contiguous`);
            }
        }
    });

    it('covers every item exactly once', () => {
        for (const n of [1, 2, 3, 4, 7, 10]) {
            const flat = partitionIntoGroups(items, n).flat();
            assert.deepEqual(flat, items, `n=${n}`);
        }
    });

    it('makes exactly n groups when n <= length', () => {
        for (const n of [1, 2, 3, 4, 7, 10]) {
            assert.equal(partitionIntoGroups(items, n).length, n);
        }
    });

    it('balances group sizes to within one', () => {
        const sizes = partitionIntoGroups(items, 3).map(g => g.length);
        assert.equal(Math.max(...sizes) - Math.min(...sizes) <= 1, true,
                     JSON.stringify(sizes));
        assert.equal(sizes.reduce((a, b) => a + b, 0), items.length);
    });

    it('caps at one item per group when n exceeds the item count', () => {
        const groups = partitionIntoGroups(items, 40);
        assert.equal(groups.length, items.length);
        assert.ok(groups.every(g => g.length === 1));
    });

    it('handles the degenerate inputs', () => {
        assert.deepEqual(partitionIntoGroups([], 4), []);
        assert.deepEqual(partitionIntoGroups(null, 4), []);
        // n of 0 or negative must still produce one usable group.
        assert.equal(partitionIntoGroups(items, 0).length, 1);
        assert.equal(partitionIntoGroups(items, -3).length, 1);
    });

    it('preserves order for the real shape', () => {
        const panes = Array.from({ length: PANES }, (_, i) => i);
        const groups = partitionIntoGroups(panes, 58);
        assert.equal(groups.length, 58);
        assert.deepEqual(groups.flat(), panes);
    });
});

describe('mergeOntoContext', () => {
    function fakeCtx() {
        const calls = [];
        return {
            calls,
            globalAlpha: 1,
            clearRect: (...a) => calls.push(['clearRect', ...a]),
            drawImage: (img, x, y, w, h) => calls.push(
                ['drawImage', img.id, x, y, w, h, calls.ctxAlpha]),
        };
    }

    // drawImage must observe the alpha set immediately before it, so record it.
    function recordingCtx() {
        const drawn = [];
        const ctx = {
            _alpha: 1,
            get globalAlpha() { return this._alpha; },
            set globalAlpha(v) { this._alpha = v; },
            cleared: 0,
            clearRect() { this.cleared++; },
            drawImage(img) { drawn.push({ id: img.id, alpha: this._alpha }); },
        };
        return { ctx, drawn };
    }

    it('draws bottom-most first, at each layer opacity', () => {
        const { ctx, drawn } = recordingCtx();
        const n = mergeOntoContext(ctx, [
            { image: { id: 'a' }, opacity: 0.7 },
            { image: { id: 'b' }, opacity: 0.5 },
            { image: { id: 'c' }, opacity: 1 },
        ], 256);
        assert.equal(n, 3);
        assert.deepEqual(drawn, [
            { id: 'a', alpha: 0.7 },
            { id: 'b', alpha: 0.5 },
            { id: 'c', alpha: 1 },
        ]);
        // Cleared once so a re-merge doesn't accumulate onto the old content.
        assert.equal(ctx.cleared, 1);
    });

    it('restores globalAlpha so the canvas is reusable', () => {
        const { ctx } = recordingCtx();
        mergeOntoContext(ctx, [{ image: { id: 'a' }, opacity: 0.3 }], 256);
        assert.equal(ctx.globalAlpha, 1);
    });

    it('defaults a missing opacity to fully opaque', () => {
        const { ctx, drawn } = recordingCtx();
        mergeOntoContext(ctx, [{ image: { id: 'a' } }], 256);
        assert.deepEqual(drawn, [{ id: 'a', alpha: 1 }]);
    });

    it('clamps an opacity above 1', () => {
        const { ctx, drawn } = recordingCtx();
        mergeOntoContext(ctx, [{ image: { id: 'a' }, opacity: 4 }], 256);
        assert.equal(drawn[0].alpha, 1);
    });

    it('skips a layer that failed to decode rather than losing the group', () => {
        // One missing layer must not blank the whole merged tile — that would
        // turn a single failed request into exactly the symptom being fixed.
        const { ctx, drawn } = recordingCtx();
        const n = mergeOntoContext(ctx, [
            { image: { id: 'a' }, opacity: 1 },
            { image: null, opacity: 1 },
            null,
            { image: { id: 'd' }, opacity: 1 },
        ], 256);
        assert.equal(n, 2);
        assert.deepEqual(drawn.map(d => d.id), ['a', 'd']);
    });

    it('skips a fully transparent layer', () => {
        const { ctx, drawn } = recordingCtx();
        mergeOntoContext(ctx, [{ image: { id: 'a' }, opacity: 0 }], 256);
        assert.equal(drawn.length, 0);
    });

    it('still clears for an empty source list', () => {
        // An empty group must produce a blank tile, not stale pixels.
        const { ctx } = recordingCtx();
        assert.equal(mergeOntoContext(ctx, [], 256), 0);
        assert.equal(ctx.cleared, 1);
    });

    it('is a no-op without a context or a size', () => {
        const { ctx } = recordingCtx();
        assert.equal(mergeOntoContext(null, [{ image: { id: 'a' } }], 256), 0);
        assert.equal(mergeOntoContext(ctx, [{ image: { id: 'a' } }], 0), 0);
        assert.equal(ctx.cleared, 0);
        assert.equal(mergeOntoContext(ctx, null, 256), 0);
    });
});

describe('closeAll', () => {
    it('releases every bitmap', () => {
        // The explicit release is the entire reason for ImageBitmap over <img>:
        // an <img> gives no control over when the decode is freed, which is the
        // bug. Leaking here defeats the purpose of the whole change.
        const closed = [];
        const bmp = (id) => ({ id, close() { closed.push(id); } });
        closeAll([bmp('a'), bmp('b'), bmp('c')]);
        assert.deepEqual(closed, ['a', 'b', 'c']);
    });

    it('tolerates nulls and plain images with no close()', () => {
        assert.doesNotThrow(() => closeAll([null, {}, undefined,
                                            { close: 'not a function' }]));
        assert.doesNotThrow(() => closeAll(null));
    });
});

describe('describePlan', () => {
    it('reports the real before and after for the measured shape', () => {
        const plan = describePlan({
            paneCount: PANES, groupCount: 58,
            tilesPerPane: TILES_PER_PANE, bytesPerTile: TILE_BYTES,
        });
        assert.equal(plan.beforeMB, 582);
        assert.equal(plan.afterMB, 348);
        assert.ok(plan.afterMB < 458, 'must land under the measured ceiling');
    });

    it('reports the transient peak, which grows as N shrinks', () => {
        // Small N means large groups, and a group's sources are decoded
        // together before being closed. Merging all 97 into 1 has a far bigger
        // transient than merging into 58, and that is worth seeing.
        const wide = describePlan({
            paneCount: PANES, groupCount: 58,
            tilesPerPane: TILES_PER_PANE, bytesPerTile: TILE_BYTES,
        });
        const narrow = describePlan({
            paneCount: PANES, groupCount: 1,
            tilesPerPane: TILES_PER_PANE, bytesPerTile: TILE_BYTES,
        });
        assert.ok(narrow.transientMB > wide.transientMB);
    });
});

describe('opacity semantics (Leaflet applies it to the container div)', () => {
    // Every layer is created opaque today (transparency lives in the tile
    // pixels), so a partial opacity is exercised here as a stand-in: the
    // associativity these tests pin down is what keeps the merge honest if a
    // pane ever does carry one.
    const PARTIAL = 0.7;

    const A = [200, 40, 40, 255];    // opaque red-ish layer
    const B = [40, 200, 40, 180];    // partly transparent green-ish layer
    const C = [40, 40, 200, 255];    // opaque blue-ish layer
    const BG = [0, 0, 0, 0];

    function near(actual, expected, tol = 1e-9, label = '') {
        for (let i = 0; i < 4; i++) {
            assert.ok(Math.abs(actual[i] - expected[i]) < tol,
                      `${label} channel ${i}: ${actual[i]} vs ${expected[i]}`);
        }
    }

    it('merging a run then compositing at 1 equals compositing member-by-member',
       () => {
        // This is the invariant the whole approach rests on: src-over is
        // associative, so rasterizing a contiguous run into one image and
        // drawing that image is identical to drawing the run's members in order.
        const stack = [
            { rgba: A, opacity: PARTIAL },
            { rgba: B, opacity: PARTIAL },
            { rgba: C, opacity: PARTIAL },
        ];
        const direct = compositeStack(stack, BG);

        // Merge into a transparent intermediate (what the canvas holds), then
        // composite that intermediate at MERGED_PANE_OPACITY.
        const merged = compositeStack(stack, [0, 0, 0, 0]);
        const viaGroup = blendOver(BG, merged, MERGED_PANE_OPACITY);

        near(viaGroup, direct, 1e-9, 'grouped vs direct');
    });

    it('holds when the group mixes opacities', () => {
        // An opaque pane under two partly transparent ones — grouping those
        // together must not change the result.
        const stack = [
            { rgba: C, opacity: 1 },        // _instances
            { rgba: A, opacity: PARTIAL },  // metal1
            { rgba: B, opacity: PARTIAL },  // metal2
        ];
        const direct = compositeStack(stack, BG);
        const merged = compositeStack(stack, [0, 0, 0, 0]);
        near(blendOver(BG, merged, MERGED_PANE_OPACITY), direct, 1e-9);
    });

    it('holds for any split point, which is what makes N groups safe', () => {
        // Splitting the same stack into two groups at every possible boundary
        // must give the same answer — otherwise the choice of N would change
        // what the user sees.
        const stack = [
            { rgba: A, opacity: PARTIAL },
            { rgba: B, opacity: PARTIAL },
            { rgba: C, opacity: 1 },
            { rgba: A, opacity: 0.5 },
        ];
        const direct = compositeStack(stack, BG);
        for (let split = 0; split <= stack.length; split++) {
            const lower = compositeStack(stack.slice(0, split), [0, 0, 0, 0]);
            const upper = compositeStack(stack.slice(split), [0, 0, 0, 0]);
            let out = blendOver(BG, lower, MERGED_PANE_OPACITY);
            out = blendOver(out, upper, MERGED_PANE_OPACITY);
            near(out, direct, 1e-9, `split at ${split}`);
        }
    });

    it('DOUBLE-APPLIES if the merged pane keeps the layer opacity', () => {
        // The trap: leaving the merged pane at a source's opacity multiplies
        // every source by it twice (0.7 -> 0.49) and washes the view out. Pinned
        // as a real numeric
        // difference so the guard cannot be quietly dropped.
        const stack = [{ rgba: A, opacity: PARTIAL },
                       { rgba: B, opacity: PARTIAL }];
        const direct = compositeStack(stack, BG);
        const merged = compositeStack(stack, [0, 0, 0, 0]);

        const correct = blendOver(BG, merged, MERGED_PANE_OPACITY);
        const doubled = blendOver(BG, merged, PARTIAL);
        near(correct, direct, 1e-9);
        assert.ok(Math.abs(doubled[3] - direct[3]) > 1,
                  'a pane opacity of 0.7 must visibly differ — it is the bug');
        // And it is specifically an under-blend: less opaque than intended.
        assert.ok(doubled[3] < direct[3]);
    });

    it('mergedPaneOptions never hands back a translucent pane', () => {
        assert.equal(MERGED_PANE_OPACITY, 1);
        assert.equal(mergedPaneOptions().opacity, 1);
        assert.equal(mergedPaneOptions({ zIndex: 7 }).opacity, 1);
        assert.equal(mergedPaneOptions({ zIndex: 7 }).zIndex, 7);
        // tileSize is only forwarded when asked for, so Leaflet's default holds.
        assert.equal('tileSize' in mergedPaneOptions(), false);
        assert.equal(mergedPaneOptions({ tileSize: 256 }).tileSize, 256);
    });
});

describe('blendOver (reference src-over)', () => {
    it('leaves the destination alone for a fully transparent source', () => {
        const dst = [10, 20, 30, 255];
        assert.deepEqual(blendOver(dst, [200, 0, 0, 0], 1), dst);
    });

    it('replaces the destination for an opaque source at full opacity', () => {
        const out = blendOver([10, 20, 30, 255], [200, 100, 50, 255], 1);
        assert.deepEqual(out, [200, 100, 50, 255]);
    });

    it('keeps the source colour when landing on transparency', () => {
        // The classic premultiplication mistake would darken this towards zero.
        const out = blendOver([0, 0, 0, 0], [200, 100, 50, 255], 0.5);
        assert.equal(Math.round(out[0]), 200);
        assert.equal(Math.round(out[1]), 100);
        assert.equal(Math.round(out[2]), 50);
        assert.equal(Math.round(out[3]), 128);
    });

    it('treats opacity as a multiplier on the source alpha', () => {
        const full = blendOver([0, 0, 0, 0], [10, 20, 30, 200], 1);
        const half = blendOver([0, 0, 0, 0], [10, 20, 30, 200], 0.5);
        assert.ok(Math.abs(half[3] - full[3] / 2) < 1e-9);
    });

    it('is order dependent, as compositing is', () => {
        const ab = blendOver([200, 0, 0, 255], [0, 0, 200, 128], 1);
        const ba = blendOver([0, 0, 200, 128], [200, 0, 0, 255], 1);
        assert.notDeepEqual(ab, ba);
    });

    it('clamps an out-of-range opacity', () => {
        const at1 = blendOver([0, 0, 0, 0], [10, 20, 30, 255], 1);
        assert.deepEqual(blendOver([0, 0, 0, 0], [10, 20, 30, 255], 4), at1);
        const out = blendOver([1, 2, 3, 255], [10, 20, 30, 255], -1);
        assert.deepEqual(out, [1, 2, 3, 255]);
    });

    it('returns full transparency when nothing is present', () => {
        assert.deepEqual(blendOver([0, 0, 0, 0], [10, 20, 30, 0], 1),
                         [0, 0, 0, 0]);
    });
});

describe('compositeStack', () => {
    it('applies sources bottom-most first', () => {
        const red = { rgba: [255, 0, 0, 255], opacity: 1 };
        const blue = { rgba: [0, 0, 255, 255], opacity: 1 };
        assert.deepEqual(compositeStack([red, blue]), [0, 0, 255, 255]);
        assert.deepEqual(compositeStack([blue, red]), [255, 0, 0, 255]);
    });

    it('skips null entries rather than throwing', () => {
        const red = { rgba: [255, 0, 0, 255], opacity: 1 };
        assert.deepEqual(compositeStack([null, red, {}]), [255, 0, 0, 255]);
        assert.deepEqual(compositeStack(null), [0, 0, 0, 0]);
    });
});

describe('renderMergedTile', () => {
    // Harness: every browser dependency is injected, so the orchestration is
    // testable without a DOM or a real ImageBitmap.
    function harness({ items, failRequests = [], failDecodes = [],
                       stale = () => false, delays = {} } = {}) {
        const released = [];
        const drawnCalls = [];
        const requested = [];
        return {
            released, drawnCalls, requested,
            opts: {
                items,
                request: async (item) => {
                    requested.push(item.layer);
                    const ms = delays[item.layer] || 0;
                    if (ms) {
                        await new Promise(r => setTimeout(r, ms));
                    }
                    if (failRequests.includes(item.layer)) {
                        throw new Error('cancelled: ' + item.layer);
                    }
                    return { payloadFor: item.layer };
                },
                decode: async (payload) => {
                    if (failDecodes.includes(payload.payloadFor)) {
                        throw new Error('bad png');
                    }
                    return { id: payload.payloadFor,
                             close() { released.push(this.id); } };
                },
                draw: (sources) => {
                    drawnCalls.push(sources.map(
                        s => ({ id: s.image ? s.image.id : null,
                                opacity: s.opacity })));
                    return sources.filter(s => s.image).length;
                },
                isStale: stale,
            },
        };
    }

    const ITEMS = [
        { layer: 'metal1', opacity: 0.7 },
        { layer: 'metal2', opacity: 0.7 },
        { layer: '_instances', opacity: 1 },
    ];

    it('composites in item order regardless of arrival order', () => {
        // A group is a contiguous z-run, so the canvas is recomposited from
        // scratch in list order on every arrival — the stacking must not depend
        // on which response happens to land first. metal2 arrives first here.
        const h = harness({
            items: ITEMS,
            delays: { metal1: 20, metal2: 0, _instances: 10 },
        });
        return renderMergedTile(h.opts).then((stats) => {
            assert.equal(stats.drawn, 3);
            const last = h.drawnCalls[h.drawnCalls.length - 1];
            assert.deepEqual(last.map(d => d.id),
                             ['metal1', 'metal2', '_instances']);
            // Every paint walks the full list, so a slot not yet in is drawn as
            // nothing rather than the later layers sliding down into its place.
            for (const call of h.drawnCalls) {
                assert.deepEqual(call.map(d => d.opacity), [0.7, 0.7, 1]);
            }
        });
    });

    it('carries each item its own opacity', async () => {
        const h = harness({ items: ITEMS });
        await renderMergedTile(h.opts);
        const last = h.drawnCalls[h.drawnCalls.length - 1];
        assert.deepEqual(last.map(d => d.opacity), [0.7, 0.7, 1]);
    });

    it('issues the requests concurrently, not one after another', async () => {
        // Serialising K requests would multiply tile latency by K.
        const h = harness({
            items: ITEMS,
            delays: { metal1: 40, metal2: 40, _instances: 40 },
        });
        const t0 = Date.now();
        await renderMergedTile(h.opts);
        assert.ok(Date.now() - t0 < 100,
                  'requests look serialised');
        assert.equal(h.requested.length, 3);
    });

    it('releases every decoded image', async () => {
        const h = harness({ items: ITEMS });
        await renderMergedTile(h.opts);
        assert.deepEqual(h.released.sort(),
                         ['_instances', 'metal1', 'metal2']);
    });

    it('releases even when the draw throws', async () => {
        // A leak here reintroduces exactly the unbounded decoded-image growth
        // this whole change exists to bound.
        const h = harness({ items: ITEMS });
        h.opts.draw = () => { throw new Error('canvas gone'); };
        await assert.rejects(renderMergedTile(h.opts), /canvas gone/);
        assert.equal(h.released.length, 3);
    });

    it('skips a failed request and still paints the rest', async () => {
        // One dropped request must not blank the tile — that would turn a
        // cancelled fetch into the symptom being fixed.
        const h = harness({ items: ITEMS, failRequests: ['metal2'] });
        const stats = await renderMergedTile(h.opts);
        assert.equal(stats.requested, 3);
        assert.equal(stats.arrived, 2);
        assert.equal(stats.drawn, 2);
        const last = h.drawnCalls[h.drawnCalls.length - 1];
        assert.deepEqual(last.map(d => d.id), ['metal1', null, '_instances']);
    });

    it('skips an undecodable payload and still paints the rest', async () => {
        const h = harness({ items: ITEMS, failDecodes: ['metal1'] });
        const stats = await renderMergedTile(h.opts);
        assert.equal(stats.arrived, 3);
        assert.equal(stats.decoded, 2);
        assert.equal(stats.drawn, 2);
        const last = h.drawnCalls[h.drawnCalls.length - 1];
        assert.deepEqual(last.map(d => d.id), [null, 'metal2', '_instances']);
        // The failed decode leaves nothing to release.
        assert.deepEqual(h.released.sort(), ['_instances', 'metal2']);
    });

    it('paints an empty group rather than leaving stale pixels', async () => {
        // The canvas is reused across refreshes, so an empty group must clear.
        const h = harness({ items: [] });
        const stats = await renderMergedTile(h.opts);
        assert.equal(stats.requested, 0);
        assert.equal(h.drawnCalls.length, 1);
        assert.deepEqual(h.drawnCalls[0], []);
    });

    it('abandons a stale tile before drawing', async () => {
        const h = harness({ items: ITEMS, stale: () => true });
        const stats = await renderMergedTile(h.opts);
        assert.equal(stats.stale, true);
        assert.equal(h.drawnCalls.length, 0, 'must not paint superseded content');
    });

    it('stops painting the moment it goes stale, and still frees the decodes', async () => {
        // A refresh or pan mid-flight must not keep painting the canvas with
        // content the map has already moved past — but the bitmaps decoded
        // before that point still have to be released.
        let stale = false;
        const h = harness({ items: ITEMS, stale: () => stale });
        const opts = h.opts;
        const realDraw = opts.draw;
        opts.draw = (sources) => {
            stale = true;   // superseded right after the first paint
            return realDraw(sources);
        };
        const stats = await renderMergedTile(opts);
        assert.equal(stats.stale, true);
        assert.equal(h.drawnCalls.length, 1, 'no painting after going stale');
        assert.equal(h.released.length, 3, 'decoded images must still be freed');
    });

    it('tolerates being called with nothing', async () => {
        const stats = await renderMergedTile();
        assert.equal(stats.requested, 0);
    });
});

describe('renderMergedTile: a missing draw is a loud failure, not a blank tile', () => {
    it('throws when a non-empty group has no draw', async () => {
        // The no-argument tolerance above must not turn a forgotten dependency
        // into a silently blank tile.
        await assert.rejects(renderMergedTile({
            items: [{ layer: 'metal1', opacity: 0.7 }],
            request: async () => ({}),
            decode: async () => ({ close() {} }),
        }), TypeError);
    });
});

describe('dpr must match the server and honour the display', () => {
    // The client sizes its merged canvas from this and the server renders from
    // its own copy. If they disagree, every tile is drawn at the wrong size:
    // it resamples (the moiré beat the pipeline avoids) and the memory budget
    // reserves one bitmap size while holding another.
    it('honours the display ratio instead of snapping to a ladder', async () => {
        const { quantizeDpr } = await import('../../src/tile-request.js');
        // 1.75 and 2.5 are ordinary Windows scale factors. A 5-step ladder
        // served them 1.5 and 2.0, i.e. tiles 17% and 25% too small, upscaled
        // by the browser — exactly what the tile sizing exists to prevent.
        assert.equal(quantizeDpr(1.75), 1.75);
        assert.equal(quantizeDpr(2.5), 2.5);
        for (const d of [1, 1.25, 1.5, 2, 3]) {
            assert.equal(quantizeDpr(d), d);
        }
    });

    it('rounds to two decimals to bound the cache key space', async () => {
        // dpr is part of the server's tile-cache key, so an unrounded
        // 1.3333333 would fork its own set of cached tiles.
        const { quantizeDpr } = await import('../../src/tile-request.js');
        assert.equal(quantizeDpr(4 / 3), 1.33);
        assert.equal(quantizeDpr(1.666666), 1.67);
        assert.equal(quantizeDpr(1.005), 1.0);
    });

    it('clamps and degrades exactly as the server does', async () => {
        const { quantizeDpr } = await import('../../src/tile-request.js');
        assert.equal(quantizeDpr(8), 3);
        assert.equal(quantizeDpr(3.0001), 3);
        assert.equal(quantizeDpr(1), 1);
        assert.equal(quantizeDpr(0.5), 1);
        assert.equal(quantizeDpr(0), 1);
        assert.equal(quantizeDpr(-2), 1);
        assert.equal(quantizeDpr(NaN), 1);
        assert.equal(quantizeDpr(undefined), 1);
    });

    it('makes the byte estimate match the pixels actually rendered', async () => {
        const { quantizeDpr } = await import('../../src/tile-request.js');
        // The budget must reserve what the tile really costs.
        assert.equal(tileBytes(256, quantizeDpr(1.75)), 448 * 448 * 4);
        assert.equal(tileBytes(256, quantizeDpr(2.5)), 640 * 640 * 4);
    });
});

describe('measureViewport: a zero-sized container silently disables merging', () => {
    // The bug this exists for. A container that is hidden or not yet laid out
    // reports clientWidth 0. estimateTilesPerPane(0,0) returns 1, so
    // computeGroupCount divides the whole budget by ONE tile and comes back
    // with groupCount == paneCount — one pane per layer, no merging at all —
    // while the startup log reports "merged 94 layers into 94 panes". The
    // failure then arrives the moment the panel is shown.
    const WIN = { innerWidth: 1600, innerHeight: 1000 };

    it('uses the container when it has been laid out', () => {
        assert.deepEqual(
            measureViewport({ clientWidth: 1240, clientHeight: 999 }, WIN),
            { width: 1240, height: 999 });
    });

    it('falls back to the window for a zero-sized container', () => {
        assert.deepEqual(measureViewport({ clientWidth: 0, clientHeight: 0 },
                                         WIN),
                         { width: 1600, height: 1000 });
    });

    it('falls back per axis, not all or nothing', () => {
        assert.deepEqual(measureViewport({ clientWidth: 1240, clientHeight: 0 },
                                         WIN),
                         { width: 1240, height: 1000 });
    });

    it('falls back to a sane default with no container and no window', () => {
        assert.deepEqual(measureViewport(null, null),
                         { width: 1024, height: 768 });
        assert.deepEqual(measureViewport(undefined,
                                         { innerWidth: 0, innerHeight: 0 }),
                         { width: 1024, height: 768 });
    });

    it('keeps merging enabled when the container is not laid out yet', () => {
        // End to end: the whole point is that the group count stays bounded.
        const perTile = tileBytes(256, 1);
        const bare = computeGroupCount({
            tilesPerPane: estimateTilesPerPane(0, 0),
            bytesPerTile: perTile,
            paneCount: PANES,
        });
        assert.equal(bare, PANES, 'unguarded, this is one pane per layer');

        const { width, height } = measureViewport({ clientWidth: 0,
                                                    clientHeight: 0 }, WIN);
        const guarded = computeGroupCount({
            tilesPerPane: estimateTilesPerPane(width, height),
            bytesPerTile: perTile,
            paneCount: PANES,
        });
        assert.ok(guarded < PANES,
                  `still one pane per layer: ${guarded} of ${PANES}`);
        assert.ok(guarded * estimateTilesPerPane(width, height) * perTile
                  <= DEFAULT_BUDGET_BYTES);
    });

    it('errs toward fewer panes, which is the safe direction', () => {
        // The window is larger than the map panel inside it, so falling back
        // over-estimates tiles per pane and under-estimates how many panes fit.
        // Under-estimating panes is conservative; over-estimating is the
        // failure mode.
        const perTile = tileBytes(256, 1);
        const fromWindow = computeGroupCount({
            tilesPerPane: estimateTilesPerPane(1600, 1000),
            bytesPerTile: perTile, paneCount: PANES,
        });
        const fromPanel = computeGroupCount({
            tilesPerPane: estimateTilesPerPane(800, 600),
            bytesPerTile: perTile, paneCount: PANES,
        });
        assert.ok(fromWindow < fromPanel,
                  `${fromWindow} should be < ${fromPanel}`);
    });
});

describe('reserveForUnmergedPanes: the panes outside the grouping are not free', () => {
    // _instances, _pins and the always-on highlight overlay each hold a full
    // tile grid and are never merged. At dpr 1 they are ~6 MB each and hardly
    // matter; at dpr 3 a pane is ~54 MB, so ignoring them let the budget report
    // a comfortable fit while the real total sat above the ceiling.
    const TILES = 24;

    it('charges the unmerged panes before the merged ones', () => {
        const perTile = tileBytes(256, 1);
        const perPane = TILES * perTile;
        const budget = 350 * 1024 * 1024;
        assert.equal(reserveForUnmergedPanes(budget, TILES, perTile),
                     budget - UNMERGED_PANE_COUNT * perPane);
    });

    it('keeps the real total under budget on a HiDPI display', () => {
        // The case that motivated it: dpr 3, where each unmerged pane is ~54 MB.
        const perTile = tileBytes(256, 3);
        const budget = 350 * 1024 * 1024;
        const naive = computeGroupCount({
            budgetBytes: budget, tilesPerPane: TILES,
            bytesPerTile: perTile, paneCount: PANES,
        });
        const guarded = computeGroupCount({
            budgetBytes: reserveForUnmergedPanes(budget, TILES, perTile),
            tilesPerPane: TILES, bytesPerTile: perTile, paneCount: PANES,
        });
        assert.ok(guarded < naive, `${guarded} should be < ${naive}`);

        const perPane = TILES * perTile;
        assert.ok((naive + UNMERGED_PANE_COUNT) * perPane > budget,
                  'the unguarded total should indeed bust the budget');
        assert.ok((guarded + UNMERGED_PANE_COUNT) * perPane <= budget,
                  'the guarded total must fit including the unmerged panes');
    });

    it('never starves the merged panes entirely', () => {
        // A budget too small for the unmerged panes alone must still leave room
        // for one group: a blank map is worse than a slow one.
        const perTile = tileBytes(256, 3);
        const left = reserveForUnmergedPanes(1, TILES, perTile);
        assert.equal(left, TILES * perTile);
        assert.equal(computeGroupCount({
            budgetBytes: left, tilesPerPane: TILES,
            bytesPerTile: perTile, paneCount: PANES,
        }), 1);
    });

    it('degrades to the raw budget on nonsense input', () => {
        assert.equal(reserveForUnmergedPanes(1000, 0, 4), 1000);
        assert.equal(reserveForUnmergedPanes(1000, 4, 0), 1000);
    });

    it('honours a caller-supplied pane count', () => {
        const perTile = tileBytes(256, 1);
        const perPane = TILES * perTile;
        assert.equal(reserveForUnmergedPanes(100 * perPane, TILES, perTile, 5),
                     95 * perPane);
    });
});

describe('setItemVisible: only a real change may dirty a pane', () => {
    // The layer tree's onChange walks every node and asserts the desired state
    // on each one. Reporting a change every time made a single checkbox click
    // re-request every tile in every pane — ~2300 requests on the 97-layer
    // design. The per-layer panes never had this problem because Leaflet's
    // addTo() is a no-op for a layer already on the map.
    it('reports a change only when the state moves', () => {
        const item = { layer: 'metal1', opacity: 0.7, visible: false };
        assert.equal(setItemVisible(item, true), true);
        assert.equal(item.visible, true);
        assert.equal(setItemVisible(item, true), false, 'already visible');
        assert.equal(setItemVisible(item, false), true);
        assert.equal(setItemVisible(item, false), false, 'already hidden');
    });

    it('treats a missing visible flag as hidden', () => {
        const item = { layer: 'metal1', opacity: 0.7 };
        assert.equal(setItemVisible(item, true), true);
        assert.equal(setItemVisible(item, true), false);
    });

    it('normalizes truthiness so it cannot report a spurious change', () => {
        const item = { layer: 'metal1', visible: true };
        assert.equal(setItemVisible(item, 1), false);
        assert.equal(setItemVisible(item, 'yes'), false);
        assert.equal(setItemVisible(item, 0), true);
    });

    it('is quiet for a whole tree walk that changes nothing', () => {
        // The actual regression: re-asserting the current state across every
        // layer must dirty nothing at all.
        const items = Array.from({ length: 94 }, (_, i) => ({
            layer: 'm' + i, opacity: 0.7, visible: i % 3 !== 0,
        }));
        let dirtied = 0;
        for (const item of items) {
            if (setItemVisible(item, item.visible)) {
                dirtied++;
            }
        }
        assert.equal(dirtied, 0);
    });

    it('tolerates a missing item', () => {
        assert.equal(setItemVisible(null, true), false);
        assert.equal(setItemVisible(undefined, false), false);
    });
});

describe('renderMergedTile: incremental painting', () => {
    function incHarness(items, { hang = [], delays = {} } = {}) {
        const drawnCalls = [];
        const released = [];
        let firstDraws = 0;
        return {
            drawnCalls, released,
            firstDraws: () => firstDraws,
            opts: {
                items,
                request: async (item) => {
                    if (hang.includes(item.layer)) {
                        return new Promise(() => {});  // never settles
                    }
                    const ms = delays[item.layer] || 0;
                    if (ms) {
                        await new Promise(r => setTimeout(r, ms));
                    }
                    return { payloadFor: item.layer };
                },
                decode: async (p) => ({ id: p.payloadFor,
                                        close() { released.push(this.id); } }),
                draw: (sources) => {
                    drawnCalls.push(sources.map(
                        s => (s.image ? s.image.id : null)));
                    return sources.filter(s => s.image).length;
                },
                onFirstDraw: () => { firstDraws++; },
            },
        };
    }

    const ITEMS3 = [
        { layer: 'a', opacity: 1 },
        { layer: 'b', opacity: 1 },
        { layer: 'c', opacity: 1 },
    ];

    it('paints as each layer arrives rather than once at the end', async () => {
        const h = incHarness(ITEMS3, { delays: { a: 0, b: 10, c: 20 } });
        const stats = await renderMergedTile(h.opts);
        assert.equal(stats.paints, 3, 'one paint per arrival');
        // Each paint is a full composite of what has arrived so far, in order.
        assert.deepEqual(h.drawnCalls, [
            ['a', null, null],
            ['a', 'b', null],
            ['a', 'b', 'c'],
        ]);
    });

    it('draws a not-yet-arrived layer as nothing, in its own slot', async () => {
        // Transparent is the identity for src-over, so the partial composite is
        // exact: the later layers must not slide down into the empty slot.
        const h = incHarness(ITEMS3, { delays: { a: 20, b: 0, c: 10 } });
        await renderMergedTile(h.opts);
        assert.deepEqual(h.drawnCalls[0], [null, 'b', null]);
        assert.deepEqual(h.drawnCalls[1], [null, 'b', 'c']);
        assert.deepEqual(h.drawnCalls[2], ['a', 'b', 'c']);
    });

    it('reports the first paint once, so the tile becomes visible early', async () => {
        // Leaflet hides a tile until done() marks it loaded, so onFirstDraw is
        // what makes incremental painting visible at all — and it must fire
        // exactly once, not on every repaint.
        const h = incHarness(ITEMS3, { delays: { a: 0, b: 5, c: 10 } });
        await renderMergedTile(h.opts);
        assert.equal(h.firstDraws(), 1);
    });

    it('does not let one stuck request blank the whole group', async () => {
        // The motivating case. With a batch draw, a request that never settles
        // left the tile blank indefinitely; now it costs one layer.
        const h = incHarness(ITEMS3, { hang: ['b'] });
        renderMergedTile(h.opts);           // deliberately not awaited
        await new Promise(r => setTimeout(r, 20));
        assert.ok(h.drawnCalls.length >= 1, 'must paint without the straggler');
        const last = h.drawnCalls[h.drawnCalls.length - 1];
        assert.deepEqual(last, ['a', null, 'c']);
        assert.equal(h.firstDraws(), 1, 'the tile is visible');
    });

    it('paints an all-blank composite when nothing arrives', async () => {
        // Otherwise a tile whose every layer failed would stay hidden forever,
        // because Leaflet only reveals it once done() has been called.
        const h = incHarness(ITEMS3);
        h.opts.request = async () => { throw new Error('cancelled'); };
        const stats = await renderMergedTile(h.opts);
        assert.equal(stats.arrived, 0);
        assert.equal(stats.paints, 1);
        assert.deepEqual(h.drawnCalls[0], [null, null, null]);
        assert.equal(h.firstDraws(), 1);
    });

    it('holds the decodes until the tile is complete, then frees them all', async () => {
        // A late arrival belongs UNDERNEATH layers already drawn, so the canvas
        // is recomposited from scratch and every source must still be around.
        // Releasing per paint would free a bitmap still needed by the next one.
        const h = incHarness(ITEMS3, { delays: { a: 0, b: 5, c: 10 } });
        const opts = h.opts;
        const realDraw = opts.draw;
        opts.draw = (sources) => {
            assert.equal(h.released.length, 0,
                         'nothing may be released while paints remain');
            return realDraw(sources);
        };
        await renderMergedTile(opts);
        assert.deepEqual(h.released.sort(), ['a', 'b', 'c']);
    });
});
