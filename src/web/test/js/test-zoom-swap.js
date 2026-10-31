// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';

const { ZoomSwap, ZOOM_FADE_MS } = await import('../../src/zoom-swap.js');

// A swap with a hand-driven clock and timer.
function makeSwap(over = {}) {
    const clock = { t: 0, timers: [] };
    const swap = new ZoomSwap({
        now: () => clock.t,
        setTimer: (fn, ms) => {
            const timer = { fn, at: clock.t + ms };
            clock.timers.push(timer);
            return timer;
        },
        clearTimer: (timer) => {
            clock.timers = clock.timers.filter((x) => x !== timer);
        },
        reducedMotion: () => false,
        ...over,
    });
    const advance = (ms) => {
        clock.t += ms;
        for (const timer of clock.timers.filter((x) => x.at <= clock.t)) {
            clock.timers = clock.timers.filter((x) => x !== timer);
            timer.fn();
        }
    };
    return { swap, clock, advance };
}

const levels = (plan) => plan.draw.map((d) => d.z);

describe('ZoomSwap with double buffering', () => {
    it('shows the first level as it loads', () => {
        const { swap } = makeSwap();
        swap.setZoom(3);
        assert.equal(swap.shownZ, 3);
        assert.deepEqual(levels(swap.plan()), [3]);
    });

    it('keeps the new level hidden, and the old one, until all is loaded', () => {
        const { swap } = makeSwap();
        swap.setZoom(3);
        swap.setZoom(4, { hasContent: true });
        assert.equal(swap.wantedZ(), 4);
        assert.deepEqual(levels(swap.plan()), [3]);
        assert.equal(swap.pendingZ, 4);
        assert.equal(swap.check({ settled: false }), false);
        assert.deepEqual(levels(swap.plan()), [3]);
        assert.equal(swap.check({ settled: true }), true);
        assert.deepEqual(levels(swap.plan()), [4]);
    });

    // The new level is only ever shown at scale 1 (#10463), so the reveal
    // waits for the zoom animation to end.
    it('holds the reveal while the zoom animates', () => {
        const { swap } = makeSwap();
        swap.setZoom(3);
        swap.setZoom(4);
        assert.equal(swap.check({ settled: true, animating: true }), false);
        assert.equal(swap.check({ settled: true, animating: false }), true);
    });

    // With nothing on screen there is nothing to keep showing.
    it('shows the new level as it loads when the old one is empty', () => {
        const { swap } = makeSwap();
        swap.setZoom(3);
        swap.setZoom(4, { hasContent: false });
        assert.equal(swap.pendingZ, null);
        assert.deepEqual(levels(swap.plan()), [4]);
    });

    it('reveals after the timeout when a tile never arrives', () => {
        const { swap, advance } = makeSwap({ timeoutMs: 50 });
        swap.setZoom(3);
        swap.setZoom(4);
        advance(49);
        assert.equal(swap.pendingZ, 4);
        advance(1);
        assert.equal(swap.shownZ, 4);
    });

    it('cancels the swap when the zoom goes back before it', () => {
        const { swap, clock } = makeSwap();
        swap.setZoom(3);
        swap.setZoom(4);
        swap.setZoom(3);
        assert.equal(swap.pendingZ, null);
        assert.equal(clock.timers.length, 0);
        assert.deepEqual(levels(swap.plan()), [3]);
    });

    it('reveals at once when double buffering is turned off mid-swap', () => {
        let enabled = true;
        const { swap } = makeSwap({ isEnabled: () => enabled });
        swap.setZoom(3);
        swap.setZoom(4);
        enabled = false;
        assert.equal(swap.check({ settled: false }), true);
        assert.equal(swap.shownZ, 4);
    });

    // Shown before it is complete, the new level would leave holes where its
    // tiles are still loading; the old one stays under until it settles.
    it('keeps the old level under a level shown before it settles', () => {
        for (const reveal of ['timeout', 'double buffering off']) {
            let enabled = true;
            const { swap, advance } = makeSwap({ timeoutMs: 50,
                                                 isEnabled: () => enabled });
            swap.setZoom(3);
            swap.setZoom(4);
            if (reveal === 'timeout') {
                advance(50);
            } else {
                enabled = false;
                swap.check({ settled: false });
            }
            assert.deepEqual(levels(swap.plan()), [3, 4], reveal);
            assert.equal(swap.check({ settled: false }), false, reveal);
            assert.equal(swap.check({ settled: true }), true, reveal);
            assert.deepEqual(levels(swap.plan()), [4], reveal);
        }
    });

    // The early reveal left level 4 with holes; the next zoom keeps level 3
    // filling them until level 5 is shown.
    it('keeps what lay under an early reveal through the next zoom', () => {
        const { swap, advance } = makeSwap({ timeoutMs: 50 });
        swap.setZoom(3);
        swap.setZoom(4);
        advance(50);
        assert.deepEqual(levels(swap.plan()), [3, 4]);
        swap.setZoom(5);
        assert.equal(swap.pendingZ, 5);
        assert.deepEqual(levels(swap.plan()), [3, 4]);
        assert.equal(swap.check({ settled: true }), true);
        assert.deepEqual(levels(swap.plan()), [5]);
        assert.deepEqual(swap.under, []);
    });

    it('drops the old level when the new one arrives complete', () => {
        const { swap } = makeSwap();
        swap.setZoom(3);
        swap.setZoom(4);
        assert.equal(swap.check({ settled: true }), true);
        assert.deepEqual(levels(swap.plan()), [4]);
    });
});

describe('ZoomSwap fade', () => {
    it('fades the new level in over the old one when smoothing is on', () => {
        const { swap, advance } = makeSwap({ isSmooth: () => true });
        swap.setZoom(3);
        swap.setZoom(4);
        swap.check({ settled: true });
        let plan = swap.plan();
        assert.ok(plan.fading);
        assert.deepEqual(levels(plan), [3, 4]);
        assert.equal(plan.draw[1].opacity, 0);
        advance(ZOOM_FADE_MS / 2);
        plan = swap.plan();
        assert.ok(plan.draw[1].opacity > 0.5 && plan.draw[1].opacity < 1);
        assert.deepEqual(levels(plan), [3, 4], 'old level kept under the fade');
        advance(ZOOM_FADE_MS);
        plan = swap.plan();
        assert.ok(!plan.fading);
        assert.deepEqual(levels(plan), [4]);
    });

    it('swaps at once with smoothing off', () => {
        const { swap } = makeSwap({ isSmooth: () => false });
        swap.setZoom(3);
        swap.setZoom(4);
        swap.check({ settled: true });
        assert.ok(!swap.plan().fading);
    });

    it('swaps at once when the user asks for reduced motion', () => {
        const { swap } = makeSwap({ isSmooth: () => true,
                                    reducedMotion: () => true });
        swap.setZoom(3);
        swap.setZoom(4);
        swap.check({ settled: true });
        assert.ok(!swap.plan().fading);
    });

    // The next zoom must not inherit a half-finished fade.
    it('ends a fade when the next zoom starts', () => {
        const { swap } = makeSwap({ isSmooth: () => true });
        swap.setZoom(3);
        swap.setZoom(4);
        swap.check({ settled: true });
        swap.setZoom(5);
        assert.equal(swap.fade, null);
        assert.deepEqual(levels(swap.plan()), [4]);
        assert.equal(swap.pendingZ, 5);
    });
});

describe('ZoomSwap without double buffering', () => {
    // The old level stays under the new one, which paints tile by tile,
    // until the new level has settled.
    it('draws the new level over the old one until it settles', () => {
        const { swap } = makeSwap({ isEnabled: () => false });
        swap.setZoom(3);
        swap.setZoom(4);
        assert.equal(swap.wantedZ(), 4);
        assert.deepEqual(levels(swap.plan()), [3, 4]);
        assert.equal(swap.check({ settled: false }), false);
        assert.equal(swap.check({ settled: true, animating: true }), false);
        assert.equal(swap.check({ settled: true }), true);
        assert.deepEqual(levels(swap.plan()), [4]);
    });
});

describe('ZoomSwap without double buffering, zooming again', () => {
    // A second zoom before the first level drew anything must not drop the
    // level still showing under it.
    it('keeps the old level under a new one that has nothing yet', () => {
        const { swap } = makeSwap({ isEnabled: () => false });
        swap.setZoom(3);
        swap.setZoom(4);
        swap.setZoom(5, { hasContent: false });
        assert.deepEqual(levels(swap.plan()), [3, 5]);
    });

    // The level being replaced may itself be partly loaded, so the one under
    // it stays as well.
    it('keeps every level under a partly loaded one', () => {
        const { swap } = makeSwap({ isEnabled: () => false });
        swap.setZoom(3);
        swap.setZoom(4);
        swap.setZoom(5);
        assert.deepEqual(levels(swap.plan()), [3, 4, 5]);
        swap.setZoom(6);
        swap.setZoom(7);
        assert.deepEqual(levels(swap.plan()), [4, 5, 6, 7], 'at most three under');
        assert.equal(swap.check({ settled: true }), true);
        assert.deepEqual(levels(swap.plan()), [7]);
    });

    it('does not draw a level twice when zooming back onto the one under', () => {
        const { swap } = makeSwap({ isEnabled: () => false });
        swap.setZoom(3);
        swap.setZoom(4);
        swap.setZoom(3, { hasContent: false });
        assert.deepEqual(levels(swap.plan()), [3]);
    });
});

describe('ZoomSwap.isIdle', () => {
    it('is false while a level is pending, kept under, or fading', () => {
        const { swap, advance } = makeSwap({ isSmooth: () => true });
        swap.setZoom(3);
        assert.ok(swap.isIdle());
        swap.setZoom(4);
        assert.ok(!swap.isIdle());
        swap.check({ settled: true });
        assert.ok(!swap.isIdle());
        advance(ZOOM_FADE_MS + 1);
        assert.ok(swap.isIdle());
    });
});
