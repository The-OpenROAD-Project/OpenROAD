// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { makeFakeDeck, makeFrames } from './fake-deck.js';

const { createLayoutView, DRAW_ORDER, toRgba }
    = await import('../../src/layout-view.js');
const { tileWorldBounds, project } = await import('../../src/view-math.js');

const T = 240;

function makeContainer(width = 800, height = 600) {
    const el = document.createElement('div');
    Object.defineProperty(el, 'clientWidth', { value: width });
    Object.defineProperty(el, 'clientHeight', { value: height });
    document.body.appendChild(el);
    return el;
}

// A source whose tiles the test settles by hand.
function makeSource() {
    const loads = new Map();
    return {
        loads,
        load(c, { background }) {
            let resolve;
            const promise = new Promise((res) => { resolve = res; });
            const key = `${c.z}/${c.x}/${c.y}`;
            loads.set(key, { ...c, background, resolve });
            return { promise, cancel() { loads.delete(key); }, promote() {} };
        },
        release() {},
        // Settle every pending load of level z with a drawable, or `value`.
        settle(z, value) {
            for (const [key, l] of [...loads]) {
                if (l.z === z) {
                    loads.delete(key);
                    l.resolve(value === undefined
                        ? { image: { key }, width: 480, height: 480 } : value);
                }
            }
        },
    };
}

const tick = () => new Promise((resolve) => setTimeout(resolve, 0));

function setup({ smooth = false, doubleBuffer = true, defer = false,
                 deck = makeFakeDeck(), ...more } = {}) {
    const frames = makeFrames();
    const prefs = { smooth, doubleBuffer, defer };
    const view = createLayoutView(makeContainer(), {
        ...more,
        deckApi: deck,
        prefs: { smooth: () => prefs.smooth,
                 doubleBuffer: () => prefs.doubleBuffer,
                 defer: () => prefs.defer },
        tileSize: () => T, dpr: () => 1,
        raf: frames.raf, now: frames.now,
    });
    return { view, deck, frames, prefs };
}

const tileIds = (deck) => deck.layers()
    .filter((l) => l instanceof deck.BitmapLayer)
    .map((l) => l.id);

describe('LayoutView view state', () => {
    it('drives the deck view state and reports the move', () => {
        const { view, deck, frames } = setup();
        const events = [];
        for (const e of ['movestart', 'moveend', 'zoomstart', 'zoom', 'zoomend']) {
            view.on(e, () => events.push(e));
        }
        view.setView([120, 120], 2);
        frames.flush();
        assert.equal(view.getZoom(), 2);
        assert.deepEqual(deck.decks[0].props.viewState.target.slice(0, 2),
                         view.getCenter());
        assert.equal(deck.decks[0].props.viewState.zoom, 2);
        assert.deepEqual(events, ['movestart', 'zoomstart', 'zoom', 'moveend',
                                  'zoomend']);
    });

    it('uses an orthographic view with y up and no wheel zoom of its own', () => {
        const { deck } = setup();
        const viewProps = deck.decks[0].props.views.props;
        assert.equal(viewProps.flipY, false);
        assert.equal(viewProps.controller.scrollZoom, false);
        assert.equal(viewProps.controller.dragPan, true);
        assert.equal(viewProps.controller.inertia, false);
    });

    it('fits a rect on a whole zoom level', () => {
        const { view, frames } = setup();
        view.fitWorldRect([0, 0, 240, 240]);
        frames.flush();
        assert.equal(view.getZoom(), 1);          // 600 / 240 = 2.5
        assert.deepEqual(view.getCenter(), [120, 120]);
    });

    it('keeps the point under the cursor while zooming around it', () => {
        const { view, frames } = setup();
        view.setView([120, 120], 1);
        const pt = [700, 100];
        const before = view.unproject(pt);
        view.zoomAround(pt, 1);
        frames.flush();
        const after = view.unproject(pt);
        assert.equal(view.getZoom(), 2);
        assert.ok(Math.abs(before[0] - after[0]) < 1e-9);
        assert.ok(Math.abs(before[1] - after[1]) < 1e-9);
    });

    it('animates over 250 ms with smoothing and rests on the target', () => {
        const { view, frames } = setup({ smooth: true });
        view.setView([120, 120], 1, { animate: false });
        view.zoomAround(null, 1);
        assert.ok(view.isAnimating());
        assert.equal(view.getTargetZoom(), 2);
        frames.flush(100);
        assert.ok(view.getZoom() > 1 && view.getZoom() < 2);
        frames.flush(200);
        assert.ok(!view.isAnimating());
        assert.equal(view.getZoom(), 2);
    });

    it('holds a fractional zoom mid-gesture and settles on request', () => {
        const { view } = setup();
        view.setView([120, 120], 3, { animate: false });
        const events = [];
        view.on('movestart moveend', (e) => events.push(view.isMoving()));
        view.zoomAround([100, 100], 0.4, { animate: false, settle: false });
        assert.ok(Math.abs(view.getZoom() - 3.4) < 1e-9);
        assert.ok(view.isMoving());
        assert.deepEqual(events, [true]);
        view.zoomAround([100, 100], 4 - view.getZoom());
        assert.equal(view.getZoom(), 4);
        assert.ok(!view.isMoving());
        assert.deepEqual(events, [true, false]);
    });

    it('pans by screen pixels', () => {
        const { view, frames } = setup();
        view.setView([120, 120], 1);
        view.panByPx(20, -40);
        frames.flush();
        assert.deepEqual(view.getCenter(), [130, 140]);
    });

    it('honours the zoom limits', () => {
        const { view, frames } = setup();
        view.setLimits({ minZoom: 0, maxZoom: 3 });
        view.setView([0, 0], 9);
        frames.flush();
        assert.equal(view.getZoom(), 3);
    });

    // Tiles map 1:1 onto the device grid at rest.
    it('snaps world (0, 0) onto a device pixel at rest', () => {
        const frames = makeFrames();
        const view = createLayoutView(makeContainer(801, 601), {
            deckApi: makeFakeDeck(), tileSize: () => T, dpr: () => 1.5,
            raf: frames.raf, now: frames.now,
        });
        view.setView([100.377, 77.123], 3);
        frames.flush();
        const [px, py] = view.project([0, 0]);
        assert.ok(Math.abs(px * 1.5 - Math.round(px * 1.5)) < 1e-6);
        assert.ok(Math.abs(py * 1.5 - Math.round(py * 1.5)) < 1e-6);
    });
});

describe('LayoutView tiles', () => {
    // Mid-animation the view zoom is fractional; tiles stay on the floor
    // level (#10463) unless the new level is kept hidden until complete.
    it('loads the floor level mid-zoom, the target one when hiding it', () => {
        for (const doubleBuffer of [false, true]) {
            const { view, frames } = setup({ smooth: true, doubleBuffer });
            const source = makeSource();
            view.setView([120, 120], 1, { animate: false });
            view.addTileSlot('base', { order: DRAW_ORDER.base, source });
            source.settle(1);
            view.zoomAround(null, 1);
            frames.flush(100);
            assert.ok(view.getZoom() > 1 && view.getZoom() < 2);
            const level2 = [...source.loads.values()].some((l) => l.z === 2);
            assert.equal(level2, doubleBuffer, `doubleBuffer=${doubleBuffer}`);
        }
    });

    it('sends the ring at background priority', () => {
        const { view, frames } = setup();
        const source = makeSource();
        view.setView([120, 120], 4);
        view.addTileSlot('base', { order: DRAW_ORDER.base, source });
        frames.flush();
        const loads = [...source.loads.values()];
        assert.ok(loads.some((l) => !l.background));
        assert.ok(loads.some((l) => l.background));
    });

    it('draws loaded tiles at their world bounds, skipping empty ones', async () => {
        const { view, deck, frames } = setup();
        const source = makeSource();
        view.setView([120, 120], 0);
        view.addTileSlot('base', { order: DRAW_ORDER.base, source });
        source.settle(0);
        await tick();
        frames.flush();
        const layer = deck.layers().find((l) => l.id === 'base:0/0/0');
        assert.ok(layer);
        assert.deepEqual(layer.props.bounds, tileWorldBounds(0, 0, 0, T));
        assert.deepEqual(layer.props.parameters,
                         { depthCompare: 'always', depthWriteEnabled: false });

        const empty = makeSource();
        view.addTileSlot('top', { order: DRAW_ORDER.top, source: empty });
        empty.settle(0, null);
        await tick();
        frames.flush();
        assert.ok(!tileIds(deck).some((id) => id.startsWith('top:')));
    });

    // deck can draw a new view state before the next render, so ring tiles
    // just off screen are already in the layer list.
    it('keeps ring tiles just off screen in the layer list', async () => {
        const { view, deck, frames } = setup();
        const source = makeSource();
        view.setView([120, 120], 3);    // world x 70..170 on screen
        view.addTileSlot('base', { order: DRAW_ORDER.base, source });
        source.settle(3);
        await tick();
        frames.flush();
        assert.ok(tileIds(deck).includes('base:3/1/3'), 'x 30..60 is kept');
    });

    it('stacks slots by draw order', async () => {
        const { view, deck, frames } = setup();
        const top = makeSource();
        const base = makeSource();
        view.setView([120, 120], 0);
        view.addTileSlot('top', { order: DRAW_ORDER.top, source: top });
        view.addTileSlot('base', { order: DRAW_ORDER.base, source: base });
        top.settle(0);
        base.settle(0);
        await tick();
        frames.flush();
        assert.deepEqual(tileIds(deck), ['base:0/0/0', 'top:0/0/0']);
    });

    // Double buffering across the whole view (#11340).
    it('keeps the old level until the new one is complete', async () => {
        const { view, deck, frames } = setup();
        const source = makeSource();
        view.setView([120, 120], 0);
        view.addTileSlot('base', { order: DRAW_ORDER.base, source });
        source.settle(0);
        await tick();
        view.setView([120, 120], 1);
        frames.flush();
        assert.deepEqual(tileIds(deck), ['base:0/0/0']);
        assert.equal(view.debugSnapshot().pendingZ, 1);
        source.settle(1);
        await tick();
        frames.flush();
        assert.ok(tileIds(deck).every((id) => id.startsWith('base:1/')));
        assert.equal(tileIds(deck).length, 4);
    });

    // A zoom out widens the view past the old level's tiles; the new level's
    // tiles fill that edge as they arrive, masked off under the old ones.
    it('fills the edges of a zoom out from the loading level', async () => {
        const { view, deck, frames } = setup();
        const source = makeSource();
        view.setView([120, 120], 2);
        view.addTileSlot('base', { order: DRAW_ORDER.base, source });
        source.settle(2);
        await tick();
        view.setView([120, 120], 1);
        const [key, first] = [...source.loads].find(([, l]) => l.z === 1);
        source.loads.delete(key);
        first.resolve({ image: { key }, width: 480, height: 480 });
        await tick();
        frames.flush();
        const ids = deck.layers().map((l) => l.id);
        const under = ids.indexOf(`base:under:${key}`);
        const mask = ids.indexOf('base:mask');
        const shown = ids.findIndex((id) => id.startsWith('base:2/'));
        assert.ok(under >= 0 && mask > under && shown > mask, ids.join(' '));
        const maskLayer = deck.layers()[mask];
        assert.ok(maskLayer instanceof deck.SolidPolygonLayer);
        assert.deepEqual(maskLayer.props.getFillColor, [0, 0, 0, 255]);

        source.settle(1);
        await tick();
        frames.flush();
        assert.ok(!deck.layers().some((l) => l.id.includes(':under:')
                                      || l.id === 'base:mask'));
    });

    it('fades the new level in when smoothing is on', async () => {
        const { view, deck, frames } = setup({ smooth: true });
        const source = makeSource();
        view.setView([120, 120], 0, { animate: false });
        view.addTileSlot('base', { order: DRAW_ORDER.base, source });
        source.settle(0);
        await tick();
        view.setView([120, 120], 1, { animate: false });
        source.settle(1);
        await tick();
        frames.flush();
        const fading = deck.layers().filter((l) => l.id.startsWith('base:1/'));
        assert.ok(fading.length > 0);
        assert.ok(deck.layers().some((l) => l.id === 'base:0/0/0'),
                  'the old level stays under the fade');
        assert.ok(fading.every((l) => l.props.opacity < 1));
        frames.flush(200);
        assert.ok(!deck.layers().some((l) => l.id === 'base:0/0/0'));
    });

    it('defers tile updates while dragging when asked to', async () => {
        const { view, deck, frames } = setup({ defer: true });
        const source = makeSource();
        view.setView([40, 40], 3);
        view.addTileSlot('base', { order: DRAW_ORDER.base, source });
        const before = new Set(source.loads.keys());
        const d = deck.decks[0];
        d.props.onInteractionStateChange({ isDragging: true });
        d.props.onViewStateChange({ viewState: { target: [200, 200, 0], zoom: 3 } });
        assert.deepEqual(new Set(source.loads.keys()), before,
                         'nothing new mid-drag');
        d.props.onInteractionStateChange({ isDragging: false });
        await tick();
        frames.flush();
        assert.ok([...source.loads.keys()].some((k) => !before.has(k)));
    });

    it('keeps the shown level through a deferred pinch across levels',
       async () => {
        const { view, deck, frames } = setup({ defer: true });
        const source = makeSource();
        view.setView([120, 120], 3, { animate: false });
        view.addTileSlot('base', { order: DRAW_ORDER.base, source });
        source.settle(3);
        await tick();
        frames.flush();
        const d = deck.decks[0];
        d.props.onInteractionStateChange({ isZooming: true });
        d.props.onViewStateChange({ viewState: { target: [120, 120, 0],
                                                 zoom: 4.4 } });
        frames.flush();
        const snap = view.debugSnapshot();
        assert.equal(snap.pendingZ, null);
        assert.equal(snap.shownZ, 3);
        assert.ok(tileIds(deck).some((id) => id.startsWith('base:3/')),
                  'level 3 still drawn mid-gesture');
        d.props.onInteractionStateChange({ isZooming: false });
        await tick();
        frames.flush(400);
        assert.ok([...source.loads.values()].some((l) => l.z === 4));
    });

    // Options > "Double buffering" off, via applyPrefs (main.js).
    it('shows a level held back by double buffering once it is turned off',
       async () => {
        const { view, prefs, frames } = setup();
        const source = makeSource();
        view.setView([120, 120], 2, { animate: false });
        view.addTileSlot('base', { order: DRAW_ORDER.base, source });
        source.settle(2);
        await tick();
        frames.flush();
        view.zoomAround(null, 1);
        frames.flush();
        assert.equal(view.debugSnapshot().pendingZ, 3);
        prefs.doubleBuffer = false;
        view.applyPrefs();
        assert.equal(view.debugSnapshot().pendingZ, null);
        assert.equal(view.debugSnapshot().shownZ, 3);
    });

    it('refreshes a slot in place and hides it on request', async () => {
        const { view, deck, frames } = setup();
        const source = makeSource();
        view.setView([120, 120], 0);
        const slot = view.addTileSlot('base', { order: DRAW_ORDER.base, source });
        source.settle(0);
        await tick();
        slot.refresh();
        frames.flush();
        assert.ok(tileIds(deck).includes('base:0/0/0'),
                  'old image shown while the refresh loads');
        slot.setVisible(false);
        frames.flush();
        assert.equal(tileIds(deck).length, 0);
    });
});

// The level above is kept loaded around a view at rest, so a zoom out finds
// its tiles already there.
describe('LayoutView prefetch of the level above', () => {
    function atRest(z = 3, opts = {}) {
        const ctx = setup(opts);
        const source = makeSource();
        // Tracks every request, including those settled or cancelled since.
        const requested = [];
        const released = [];
        const load = source.load.bind(source);
        source.load = (c, o) => {
            requested.push({ ...c, background: o.background });
            return load(c, o);
        };
        source.release = (d) => released.push(d?.image?.key);
        ctx.view.setView([120, 120], z, { animate: false });
        ctx.view.addTileSlot('base', { order: DRAW_ORDER.base, source });
        return { ...ctx, source, requested, released };
    }

    it('loads z - 1 around the view in the background once at rest', async () => {
        const { source, requested } = atRest(3);
        source.settle(3);
        await tick();
        const above = requested.filter((r) => r.z === 2);
        assert.ok(above.length > 0);
        assert.ok(above.every((r) => r.background));
    });

    it('does not load it while the view moves', () => {
        const { view, requested, frames } = atRest(3, { smooth: true });
        const before = requested.length;
        view.panByPx(100, 0);
        assert.ok(view.isAnimating());
        frames.flush(100);
        assert.ok(view.isAnimating());
        assert.ok(!requested.slice(before).some((r) => r.z === 2));
        frames.flush(300);
        assert.ok(requested.slice(before).some((r) => r.z === 2));
    });

    // One tile update per instant pan, at rest: an update while still moving
    // would cancel the level above only to request it again.
    it('keeps the level above loading through instant pans', async () => {
        const { view, source, requested } = atRest(3);
        source.settle(3);
        await tick();
        const above = () => [...source.loads.values()].filter((l) => l.z === 2);
        const loading = above().length;
        assert.ok(loading > 0);
        const before = requested.length;
        for (let i = 0; i < 3; i++) {
            view.panByPx(10, 0, { animate: false });
        }
        assert.equal(above().length, loading, 'none cancelled');
        assert.ok(!requested.slice(before).some((r) => r.z === 2),
                  'none requested again');
    });

    it('zooms out with no blank and no new request for the level above',
       async () => {
        const { view, source, requested, frames } = atRest(3, { smooth: true });
        source.settle(3);
        await tick();
        source.settle(2);
        await tick();
        const before = requested.length;
        view.zoomAround(null, -1);
        frames.flush(50);
        assert.equal(view.debugSnapshot().coverage, 1, 'no blank mid-zoom');
        frames.flush(400);
        await tick();
        frames.flush(200);
        assert.equal(view.getZoom(), 2);
        assert.equal(view.debugSnapshot().shownZ, 2);
        assert.ok(!requested.slice(before).some((r) => r.z === 2
                                                 && !r.background),
                  'level 2 was already loaded');
    });

    it('waits for a trackpad gesture to end before loading it', async () => {
        const { view, source, requested, released, frames } = atRest(3);
        source.settle(3);
        await tick();
        const pending = () => [...source.loads.values()].filter((l) => l.z === 2);
        assert.ok(pending().length > 0, 'level 2 loading at rest');
        view.zoomAround(null, 0.4, { animate: false, settle: false });
        const before = requested.length;
        view.zoomAround(null, 0.3, { animate: false, settle: false });
        assert.equal(pending().length, 0, 'background work stops mid-gesture');
        assert.ok(!requested.slice(before).some((r) => r.z === 2));
        view.zoomAround(null, 4 - view.getZoom());
        // Settled on level 4: once it is shown, level 3 stays behind it.
        source.settle(4);
        await tick();
        await tick();
        frames.flush();
        assert.equal(view.debugSnapshot().shownZ, 4);
        assert.ok(!released.some((k) => k.startsWith('3/')),
                  'level 3 kept through the prune');
    });

    it('does not rebuild the layers for tiles of the level above', async () => {
        const { view, deck, source, frames } = atRest(3);
        source.settle(3);
        await tick();
        frames.flush();
        const d = deck.decks[0];
        let builds = 0;
        const setProps = d.setProps.bind(d);
        d.setProps = (props) => {
            builds += props.layers ? 1 : 0;
            setProps(props);
        };
        source.settle(2);
        await tick();
        frames.flush();
        assert.equal(builds, 0);
        view.redraw();
        frames.flush();
        assert.equal(builds, 1);
    });

    it('keeps the level above through a prune', async () => {
        const { view, source, frames } = atRest(3);
        source.settle(3);
        await tick();
        source.settle(2);
        await tick();
        frames.flush();
        const tiles = view.debugSnapshot().slots[0].tiles;
        view.redraw();
        frames.flush();
        assert.equal(view.debugSnapshot().slots[0].tiles, tiles);
    });
});

// A long jump flies: it pulls out, travels and closes in, once the level it
// pulls out to has arrived, so no part of the way is blank.
describe('LayoutView flights', () => {
    function flying(opts = { smooth: true }) {
        const ctx = setup(opts);
        const source = makeSource();
        const requested = [];
        const load = source.load.bind(source);
        source.load = (c, o) => {
            requested.push({ ...c, background: o.background });
            return load(c, o);
        };
        ctx.view.setView([40, 40], 5, { animate: false });
        ctx.view.addTileSlot('base', { order: DRAW_ORDER.base, source });
        return { ...ctx, source, requested };
    }

    it('waits for the pulled-out level, then pulls out and closes in',
       async () => {
        const { view, source, requested, frames } = flying();
        source.settle(5);
        await tick();
        const before = requested.length;
        view.setView([200, 200], 5);
        assert.ok(view.isAnimating());
        const apex = requested.slice(before).filter(
            (r) => r.z < 4 && !r.background);
        assert.ok(apex.length > 0, 'the pulled-out level, in the foreground');
        const apexZ = apex[0].z;
        frames.flush(100);
        assert.deepEqual(view.getCenter(), [40, 40], 'still waiting');
        source.settle(apexZ);
        await tick();
        frames.flush(16);
        let lowest = view.getZoom();
        for (let i = 0; i < 40; i++) {
            frames.flush(16);
            lowest = Math.min(lowest, view.getZoom());
            if (i === 20) {
                assert.equal(view.debugSnapshot().coverage, 1,
                             'no blank mid-flight');
            }
        }
        assert.ok(lowest < 4, `pulled out to ${lowest}`);
        frames.flush(1000);
        assert.ok(!view.isAnimating());
        assert.deepEqual(view.getCenter().map(Math.round), [200, 200]);
        assert.equal(view.getZoom(), 5);
    });

    // A jump at the same level used to drop what was on screen while waiting.
    it('keeps the view drawn while it waits to set off', async () => {
        const { view, source, frames } = flying();
        source.settle(5);
        await tick();
        frames.flush();
        view.setView([200, 200], 5);
        frames.flush(100);
        assert.deepEqual(view.getCenter(), [40, 40], 'still waiting');
        assert.equal(view.debugSnapshot().coverage, 1);
    });

    it('draws the pulled-out level under the shown one on the way', async () => {
        const { view, source, requested, frames, deck } = flying();
        source.settle(5);
        await tick();
        const before = requested.length;
        view.setView([200, 200], 5);
        const apexZ = requested.slice(before).find((r) => r.z < 4).z;
        source.settle(apexZ);
        await tick();
        frames.flush(16);
        frames.flush(200);
        assert.ok(deck.layers().some((l) => l.id.startsWith(`base:under:${apexZ}/`)));
    });

    // The destination can still be loading when the flight lands; the
    // pulled-out level stays under it until it is shown.
    it('keeps the pulled-out level under the view until the destination shows',
       async () => {
        const { view, source, requested, frames, deck } = flying();
        source.settle(5);
        await tick();
        const before = requested.length;
        view.setView([200, 200], 5);
        const apexZ = requested.slice(before).find((r) => r.z < 4).z;
        source.settle(apexZ);
        await tick();
        frames.flush(16);
        frames.flush(2000);
        frames.flush(16);
        assert.ok(!view.isAnimating(), 'landed');
        assert.ok(!view.debugSnapshot().slots[0].settled, 'destination loading');
        assert.equal(view.debugSnapshot().coverage, 1, 'no blank after landing');
        assert.ok(deck.layers().some((l) => l.id.startsWith(`base:under:${apexZ}/`)));
        source.settle(5);
        await tick();
        frames.flush(16);
        frames.flush(500);
        assert.ok(!deck.layers().some((l) => l.id.startsWith(`base:under:${apexZ}/`)),
                  'gone once the destination shows');
    });

    it('sets off anyway when the pulled-out level is slow', () => {
        const { view, frames } = flying();
        view.setView([200, 200], 5);
        frames.flush(100);
        assert.deepEqual(view.getCenter(), [40, 40]);
        frames.flush(250);
        frames.flush(16);
        assert.notDeepEqual(view.getCenter(), [40, 40]);
    });

    it('slides a short jump as before', () => {
        const { view, requested, frames } = flying();
        const before = requested.length;
        view.setView([60, 40], 5);
        assert.ok(!requested.slice(before).some((r) => r.z < 4));
        frames.flush(100);
        assert.equal(view.getZoom(), 5);
        assert.notDeepEqual(view.getCenter(), [40, 40]);
    });

    it('cuts straight there without smoothing', () => {
        const { view } = flying({ smooth: false });
        view.setView([200, 200], 5);
        assert.ok(!view.isAnimating());
        assert.deepEqual(view.getCenter().map(Math.round), [200, 200]);
    });
});

describe('LayoutView drag and inertia', () => {
    // A drag as deck's controller reports it: interaction on, a view state
    // per frame, interaction off and then the pan-end view state.
    async function drag(deck, frames, view, { steps = 5, dx = 10 } = {}) {
        const p = deck.decks[0].props;
        p.onInteractionStateChange({ isDragging: true });
        for (let i = 0; i < steps; i++) {
            frames.t += 16;
            const [x, y] = view.getCenter();
            p.onViewStateChange({ viewState: { target: [x + dx, y],
                                               zoom: view.getZoom() } });
        }
        p.onInteractionStateChange({ isDragging: false });
        p.onViewStateChange({ viewState: { target: view.getCenter(),
                                           zoom: view.getZoom() } });
        await tick();
    }

    function track(view) {
        const events = [];
        view.on('movestart moveend', () => events.push(view.isMoving()));
        return events;
    }

    // deck's inertia only runs if its transition props come back through the
    // controlled viewState; left on, a release never ended the move.
    it("keeps deck's own inertia off even with smoothing", () => {
        const { deck } = setup({ smooth: true });
        assert.equal(deck.decks[0].props.views.props.controller.inertia, false);
    });

    it('glides on after a fast release with smoothing, then ends the move', async () => {
        const { view, deck, frames } = setup({ smooth: true });
        view.setView([120, 120], 0, { animate: false });
        const moves = track(view);
        await drag(deck, frames, view);
        const released = view.getCenter()[0];
        assert.ok(view.isAnimating(), 'gliding');
        assert.ok(view.isMoving());
        frames.flush(400);
        assert.ok(!view.isAnimating());
        assert.ok(!view.isMoving(), 'moveend fired');
        assert.ok(view.getCenter()[0] > released + 10, 'carried on the drag');
        assert.deepEqual(moves, [true, false]);
    });

    it('stops dead on a slow release', async () => {
        const { view, deck, frames } = setup({ smooth: true });
        view.setView([120, 120], 0, { animate: false });
        await drag(deck, frames, view, { dx: 0.1 });
        assert.ok(!view.isAnimating());
        assert.ok(!view.isMoving());
    });

    it('stops dead without smoothing', async () => {
        const { view, deck, frames } = setup({ smooth: false });
        view.setView([120, 120], 0, { animate: false });
        await drag(deck, frames, view);
        assert.ok(!view.isAnimating());
        assert.ok(!view.isMoving());
    });

    it('does not glide after holding still before the release', async () => {
        const { view, deck, frames } = setup({ smooth: true });
        view.setView([120, 120], 0, { animate: false });
        const p = deck.decks[0].props;
        p.onInteractionStateChange({ isDragging: true });
        for (let i = 0; i < 5; i++) {
            frames.t += 16;
            const [x, y] = view.getCenter();
            p.onViewStateChange({ viewState: { target: [x + 10, y], zoom: 0 } });
        }
        frames.t += 500;
        p.onInteractionStateChange({ isDragging: false });
        await tick();
        assert.ok(!view.isAnimating());
        assert.ok(!view.isMoving());
    });

    it('a new drag stops the glide', async () => {
        const { view, deck, frames } = setup({ smooth: true });
        view.setView([120, 120], 0, { animate: false });
        await drag(deck, frames, view);
        assert.ok(view.isAnimating());
        await drag(deck, frames, view, { dx: 0.1 });
        assert.ok(!view.isAnimating());
        assert.ok(!view.isMoving());
    });
});

describe('LayoutView shapes, markers and capture', () => {
    it('draws rect overlays as fills and dashed strokes above the tiles', () => {
        const { view, deck, frames } = setup();
        view.setOverlay('selection', [{
            kind: 'rect', rect: [0, 0, 10, 10],
            stroke: toRgba('#ff0'), width: 2, dash: [6, 4],
            fill: toRgba('#ff0', 0.25),
        }]);
        frames.flush();
        const layers = deck.layers();
        const fill = layers.find((l) => l instanceof deck.SolidPolygonLayer);
        const dash = layers.find((l) => l instanceof deck.PathLayer);
        assert.ok(fill && dash);
        assert.deepEqual(dash.props.getDashArray(dash.props.data[0]), [3, 2]);
        assert.equal(dash.props.extensions[0].opts.dash, true);
        view.setOverlay('selection', null);
        frames.flush();
        assert.equal(deck.layers().length, 0);
    });

    it('positions markers at their world point', () => {
        const { view, frames } = setup();
        view.setView([120, 120], 1);
        frames.flush();
        const el = document.createElement('div');
        view.addMarker('labels', 'a', [100, 100], el);
        const [x, y] = project([100, 100], view.getViewState());
        assert.equal(el.style.transform, `translate(${x}px, ${y}px)`);
        view.setMarkerGroupVisible('labels', false);
        assert.equal(el.style.display, 'none');
        view.clearMarkers('labels');
        assert.equal(el.parentNode, null);
    });

    it('converts colours to RGBA', () => {
        assert.deepEqual(toRgba('#ff0'), [255, 255, 0, 255]);
        assert.deepEqual(toRgba('#00ffff', 0.5), [0, 255, 255, 128]);
        assert.deepEqual(toRgba([1, 2, 3]), [1, 2, 3, 255]);
    });

    // The copy is taken in deck's frame: the buffer is not kept past it.
    it('copies the frame deck draws with the current layers', async () => {
        const deck = makeFakeDeck();
        const { view } = setup({
            deck,
            copyCanvas: (c) => ({ of: c, layers: deck.decks[0].drawn }),
        });
        // deck has no canvas until its device is up.
        assert.equal(await view.captureCanvas(), null);
        await tick();
        const d = deck.decks[0];
        let shot;
        view.captureCanvas().then((s) => { shot = s; });
        await tick();
        assert.equal(shot, undefined, 'waits for deck to draw');
        d.frame();
        await tick();
        assert.equal(shot.image.of, d.canvas);
        assert.equal(shot.image.layers, d.props.layers,
                     'copied in the frame, with the layers just built');
        assert.ok(shot.rect);
    });

    it('copies after a redraw of its own when no frame comes', async () => {
        const deck = makeFakeDeck();
        const { view } = setup({
            deck,
            copyCanvas: (c) => ({ of: c, layers: deck.decks[0].drawn }),
        });
        await tick();
        const d = deck.decks[0];
        const redraws = d.redraws;
        const shot = await view.captureCanvas({ timeoutMs: 10 });
        assert.equal(d.redraws, redraws + 1);
        assert.equal(shot.image.of, d.canvas);
        assert.equal(shot.image.layers, d.props.layers);
    });

    it('keeps the drawing buffer only when asked to', () => {
        const webgl = (opts) => setup(opts).deck.decks[0]
            .props.deviceProps.webgl.preserveDrawingBuffer;
        assert.equal(webgl({}), false);
        assert.equal(webgl({ preserveDrawingBuffer: true }), true);
    });

    it('settles once nothing is loading or moving', async () => {
        const { view, frames } = setup();
        const source = makeSource();
        view.setView([120, 120], 0);
        view.addTileSlot('base', { order: DRAW_ORDER.base, source });
        frames.flush();
        let settled = false;
        view.whenTilesSettled().then(() => { settled = true; });
        await new Promise((resolve) => setTimeout(resolve, 50));
        assert.ok(!settled);
        source.settle(0);
        await new Promise((resolve) => setTimeout(resolve, 80));
        assert.ok(settled);
    });

    // A page whose CDN script failed still has a working view object.
    it('rebuilds the deck when the WebGL context is lost', async () => {
        const { view, deck, frames } = setup();
        const source = makeSource();
        view.setView([120, 120], 0);
        view.addTileSlot('base', { order: DRAW_ORDER.base, source });
        source.settle(0);
        await tick();
        frames.flush();
        const events = [];
        view.on('contextlost contextrestored', () => events.push(events.length));
        const old = deck.decks[0];
        const lost = new window.Event('webglcontextlost', { cancelable: true });
        old.getCanvas().dispatchEvent(lost);
        assert.ok(lost.defaultPrevented);
        assert.ok(old.finalized);
        assert.equal(deck.decks.length, 2);
        assert.equal(events.length, 2);
        frames.flush();
        // The tiles are drawn again on the new deck without a new request.
        assert.deepEqual(tileIds(deck), ['base:0/0/0']);
        assert.equal(source.loads.size, 0);
        assert.deepEqual(deck.decks[1].props.viewState.target.slice(0, 2),
                         view.getCenter());
        // The new deck is watched too, once its device is up.
        await tick();
        deck.decks[1].getCanvas().dispatchEvent(
            new window.Event('webglcontextlost', { cancelable: true }));
        assert.equal(deck.decks.length, 3);
    });

    it('keeps working without deck', async () => {
        const frames = makeFrames();
        const view = createLayoutView(makeContainer(), {
            deckApi: null, raf: frames.raf, now: frames.now,
        });
        view.setView([1, 2], 3);
        frames.flush();
        assert.equal(view.getZoom(), 3);
        assert.equal(await view.captureCanvas(), null);
    });
});

describe('LayoutView renderer', () => {
    it('reports what draws once the deck is up', async () => {
        const { view } = setup();
        const seen = [];
        view.on('renderer', (info) => seen.push(info));
        assert.equal(view.getRendererInfo(), null);
        await tick();
        const fake = { gpuType: 'discrete', renderer: 'Fake GPU', vendor: 'Fake' };
        assert.deepEqual(seen, [fake]);
        assert.deepEqual(view.getRendererInfo(), fake);
    });

    it('reports a software renderer, and again after a lost context', async () => {
        const deck = makeFakeDeck();
        deck.deviceInfo = { gpuType: 'cpu', renderer: 'SwiftShader' };
        const { view } = setup({ deck });
        const seen = [];
        view.on('renderer', (info) => seen.push(info.gpuType));
        await tick();
        assert.deepEqual(view.getRendererInfo(),
                         { gpuType: 'cpu', renderer: 'SwiftShader', vendor: '' });
        deck.deviceInfo = { gpuType: 'integrated' };
        deck.decks[0].getCanvas().dispatchEvent(
            new window.Event('webglcontextlost', { cancelable: true }));
        await tick();
        assert.deepEqual(seen, ['cpu', 'integrated']);
        assert.equal(view.getRendererInfo().gpuType, 'integrated');
    });
});
