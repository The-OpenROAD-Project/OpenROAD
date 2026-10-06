// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';

import {
    setItemVisible, mergeOntoContext, closeAll, blendOver, compositeStack,
    renderMergedTile,
} from '../../src/tile-merge.js';

// The merged tile carries the layer opacities inside it, so it is drawn at 1.
const MERGED_OPACITY = 1;

// Let every promise continuation that is ready to run, run.
//
// setImmediate fires in the check phase, after the microtask queue has drained
// to empty, so one await here settles a chain of `await`s of any depth -- which
// is what one arriving request unblocks: the request, then the decode, then the
// paint.
//
// The renderMergedTile tests below are about the ORDER requests arrive in and
// about all of them being in flight at once, never about how long any of it
// takes.  Staging that with setTimeout delays made them depend on wall-clock
// time even so, and under a loaded machine -- `bazel test //src/web/...` runs
// 46 test binaries at once -- timers do not keep their spacing: three 40 ms
// requests measured 105 ms against a 100 ms bound, and two arrivals 5 ms apart
// can land in either order.  Gating each request on a promise the test opens by
// hand states the same orderings exactly, and no load can perturb them.
function flush() {
    return new Promise(resolve => setImmediate(resolve));
}

// One openable gate per named layer, plus `arrive`, which opens one and then
// lets everything it unblocked run before returning.
function makeGates(layers) {
    const gates = new Map();
    for (const layer of layers) {
        let open;
        const promise = new Promise(resolve => { open = resolve; });
        gates.set(layer, { promise, open });
    }
    return {
        wait: (layer) => gates.has(layer) ? gates.get(layer).promise : null,
        arrive: async (layer) => {
            gates.get(layer).open();
            await flush();
        },
    };
}

describe('tile-merge is pure', () => {
    it('exports plain functions with no DOM or renderer dependency', () => {
        // Everything here is compositing rules with the canvas injected, so it
        // stays testable without a browser.  The view parts live in
        // tile-sources.js.
        for (const fn of [setItemVisible, mergeOntoContext, closeAll,
                          blendOver, compositeStack, renderMergedTile]) {
            assert.equal(typeof fn, 'function');
        }
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

describe('opacity semantics (the merged tile is drawn at 1)', () => {
    // Every layer is created opaque today (transparency lives in the tile
    // pixels), so a partial opacity is exercised here as a stand-in: the
    // associativity these tests pin down is what keeps the merge honest if a
    // layer ever does carry one.
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
        // composite that intermediate at MERGED_OPACITY.
        const merged = compositeStack(stack, [0, 0, 0, 0]);
        const viaGroup = blendOver(BG, merged, MERGED_OPACITY);

        near(viaGroup, direct, 1e-9, 'grouped vs direct');
    });

    it('holds when the group mixes opacities', () => {
        // An opaque layer under two partly transparent ones — grouping those
        // together must not change the result.
        const stack = [
            { rgba: C, opacity: 1 },        // _instances
            { rgba: A, opacity: PARTIAL },  // metal1
            { rgba: B, opacity: PARTIAL },  // metal2
        ];
        const direct = compositeStack(stack, BG);
        const merged = compositeStack(stack, [0, 0, 0, 0]);
        near(blendOver(BG, merged, MERGED_OPACITY), direct, 1e-9);
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
            let out = blendOver(BG, lower, MERGED_OPACITY);
            out = blendOver(out, upper, MERGED_OPACITY);
            near(out, direct, 1e-9, `split at ${split}`);
        }
    });

    it('DOUBLE-APPLIES if the merged tile keeps the layer opacity', () => {
        // The trap: drawing the merged tile at a source's opacity multiplies
        // every source by it twice (0.7 -> 0.49) and washes the view out. Pinned
        // as a real numeric
        // difference so the guard cannot be quietly dropped.
        const stack = [{ rgba: A, opacity: PARTIAL },
                       { rgba: B, opacity: PARTIAL }];
        const direct = compositeStack(stack, BG);
        const merged = compositeStack(stack, [0, 0, 0, 0]);

        const correct = blendOver(BG, merged, MERGED_OPACITY);
        const doubled = blendOver(BG, merged, PARTIAL);
        near(correct, direct, 1e-9);
        assert.ok(Math.abs(doubled[3] - direct[3]) > 1,
                  'a tile opacity of 0.7 must visibly differ — it is the bug');
        // And it is specifically an under-blend: less opaque than intended.
        assert.ok(doubled[3] < direct[3]);
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
                       stale = () => false, gated = [] } = {}) {
        const released = [];
        const drawnCalls = [];
        const requested = [];
        const gates = makeGates(gated);
        return {
            released, drawnCalls, requested, arrive: gates.arrive,
            opts: {
                items,
                request: async (item) => {
                    requested.push(item.layer);
                    const gate = gates.wait(item.layer);
                    if (gate) {
                        await gate;
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

    it('composites in item order regardless of arrival order', async () => {
        // A group is a contiguous z-run, so the canvas is recomposited from
        // scratch in list order on every arrival — the stacking must not depend
        // on which response happens to land first. metal2 arrives first here.
        const h = harness({
            items: ITEMS,
            gated: ['metal1', 'metal2', '_instances'],
        });
        const done = renderMergedTile(h.opts);
        await h.arrive('metal2');
        await h.arrive('_instances');
        await h.arrive('metal1');

        const stats = await done;
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

    it('carries each item its own opacity', async () => {
        const h = harness({ items: ITEMS });
        await renderMergedTile(h.opts);
        const last = h.drawnCalls[h.drawnCalls.length - 1];
        assert.deepEqual(last.map(d => d.opacity), [0.7, 0.7, 1]);
    });

    it('issues the requests concurrently, not one after another', async () => {
        // Serialising K requests would multiply tile latency by K.
        //
        // Asserted as "all K are in flight before any of them answers", which
        // is the property itself: with every request held open, a serialised
        // implementation could not have issued the second.  Timing the whole
        // render instead only inferred it, and inferred it from a wall clock
        // that a busy machine moves (see flush()).
        const h = harness({
            items: ITEMS,
            gated: ['metal1', 'metal2', '_instances'],
        });
        const done = renderMergedTile(h.opts);
        await flush();
        assert.deepEqual(h.requested, ['metal1', 'metal2', '_instances']);

        for (const layer of ['metal1', 'metal2', '_instances']) {
            await h.arrive(layer);
        }
        await done;
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
        assert.equal(stats.failed, 1);
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
});

describe('setItemVisible: only a real change may dirty a slot', () => {
    // The layer tree's onChange walks every node and asserts the desired state
    // on each one. Reporting a change every time made a single checkbox click
    // re-request every tile.
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
    function incHarness(items, { hang = [], gated = [] } = {}) {
        const drawnCalls = [];
        const released = [];
        const gates = makeGates(gated);
        return {
            drawnCalls, released, arrive: gates.arrive,
            opts: {
                items,
                request: async (item) => {
                    if (hang.includes(item.layer)) {
                        return new Promise(() => {});  // never settles
                    }
                    const gate = gates.wait(item.layer);
                    if (gate) {
                        await gate;
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
            },
        };
    }

    const ITEMS3 = [
        { layer: 'a', opacity: 1 },
        { layer: 'b', opacity: 1 },
        { layer: 'c', opacity: 1 },
    ];

    it('paints as each layer arrives rather than once at the end', async () => {
        const h = incHarness(ITEMS3, { gated: ['a', 'b', 'c'] });
        const done = renderMergedTile(h.opts);
        await h.arrive('a');
        await h.arrive('b');
        await h.arrive('c');

        const stats = await done;
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
        const h = incHarness(ITEMS3, { gated: ['a', 'b', 'c'] });
        const done = renderMergedTile(h.opts);
        await h.arrive('b');
        await h.arrive('c');
        await h.arrive('a');

        await done;
        assert.deepEqual(h.drawnCalls[0], [null, 'b', null]);
        assert.deepEqual(h.drawnCalls[1], [null, 'b', 'c']);
        assert.deepEqual(h.drawnCalls[2], ['a', 'b', 'c']);
    });

    it('does not let one stuck request blank the whole group', async () => {
        // The motivating case. With a batch draw, a request that never settles
        // left the tile blank indefinitely; now it costs one layer.
        const h = incHarness(ITEMS3, { hang: ['b'] });
        renderMergedTile(h.opts);           // deliberately not awaited
        await flush();  // a and c are ungated, so this is all they need
        assert.ok(h.drawnCalls.length >= 1, 'must paint without the straggler');
        const last = h.drawnCalls[h.drawnCalls.length - 1];
        assert.deepEqual(last, ['a', null, 'c']);
    });

    it('paints an all-blank composite when nothing arrives', async () => {
        // Otherwise a tile whose every layer failed would never settle, and
        // the view would wait on it forever.
        const h = incHarness(ITEMS3);
        h.opts.request = async () => { throw new Error('cancelled'); };
        const stats = await renderMergedTile(h.opts);
        assert.equal(stats.arrived, 0);
        assert.equal(stats.failed, 3);
        assert.equal(stats.paints, 1);
        assert.deepEqual(h.drawnCalls[0], [null, null, null]);
    });

    it('holds the decodes until the tile is complete, then frees them all', async () => {
        // A late arrival belongs UNDERNEATH layers already drawn, so the canvas
        // is recomposited from scratch and every source must still be around.
        // Releasing per paint would free a bitmap still needed by the next one.
        const h = incHarness(ITEMS3, { gated: ['a', 'b', 'c'] });
        const opts = h.opts;
        const realDraw = opts.draw;
        opts.draw = (sources) => {
            assert.equal(h.released.length, 0,
                         'nothing may be released while paints remain');
            return realDraw(sources);
        };
        const done = renderMergedTile(opts);
        await h.arrive('a');
        await h.arrive('b');
        await h.arrive('c');

        await done;
        assert.deepEqual(h.released.sort(), ['a', 'b', 'c']);
    });

    // ── Options > "Double buffering" (#11340) ──
    //
    // Every paint clears the canvas first, so repainting a tile that is already
    // on screen shows the group being rebuilt layer by layer.  One composite,
    // after everything has settled, makes the swap atomic — the canvas is the
    // buffer, because a synchronous clear-and-composite is never presented half
    // done.

    it('paints once, at the end, when buffering', async () => {
        const h = incHarness(ITEMS3, { gated: ['a', 'b', 'c'] });
        const done = renderMergedTile({ ...h.opts, incremental: false });
        await h.arrive('a');
        await h.arrive('b');
        assert.deepEqual(h.drawnCalls, [], 'nothing painted mid-flight');

        await h.arrive('c');
        const stats = await done;
        assert.equal(stats.paints, 1);
        assert.deepEqual(h.drawnCalls, [['a', 'b', 'c']]);
    });

    // The mode decides WHEN the composite is shown, never what it contains.
    it('composites the same result either way', async () => {
        const buffered = incHarness(ITEMS3);
        const incremental = incHarness(ITEMS3);
        await renderMergedTile({ ...buffered.opts, incremental: false });
        await renderMergedTile(incremental.opts);
        const last = (h) => h.drawnCalls[h.drawnCalls.length - 1];
        assert.deepEqual(last(buffered), last(incremental));
    });

    // The cost of buffering, stated in a test rather than in a comment: the
    // tile now appears at the pace of its slowest layer, so a request that
    // never settles holds it hidden instead of costing one missing layer.  That
    // rests on cancel() rejecting rather than leaving a request pending — see
    // WebSocketManager.cancel.
    it('waits for a layer the incremental mode would paint around', async () => {
        const buffered = incHarness(ITEMS3, { hang: ['b'] });
        renderMergedTile({ ...buffered.opts, incremental: false });
        await flush();
        assert.deepEqual(buffered.drawnCalls, [], 'nothing shown yet');

        const incremental = incHarness(ITEMS3, { hang: ['b'] });
        renderMergedTile(incremental.opts);
        await flush();
        assert.ok(incremental.drawnCalls.length > 0, 'a and c are already up');
    });

    it('paints an all-blank composite when nothing arrives, buffered too',
       async () => {
           const h = incHarness(ITEMS3);
           h.opts.request = async () => { throw new Error('cancelled'); };
           const stats = await renderMergedTile(
               { ...h.opts, incremental: false });
           assert.equal(stats.paints, 1);
           assert.deepEqual(h.drawnCalls, [[null, null, null]]);
       });

    // The decodes are held to the end in both modes, so buffering costs no
    // extra memory — the reason no off-screen canvas is needed.
    it('frees every decode when buffering', async () => {
        const h = incHarness(ITEMS3, { gated: ['a', 'b', 'c'] });
        const done = renderMergedTile({ ...h.opts, incremental: false });
        await h.arrive('a');
        await h.arrive('b');
        await h.arrive('c');
        await done;
        assert.deepEqual(h.released.sort(), ['a', 'b', 'c']);
    });
});
