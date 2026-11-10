// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';

const { TileGrid, tileKey } = await import('../../src/tile-grid.js');
const { growRange } = await import('../../src/view-math.js');

// A source whose loads the test settles by hand.
function makeSource() {
    const loads = [];
    const released = [];
    const source = {
        load(coords, { background, onPartial }) {
            let resolve;
            let reject;
            const promise = new Promise((res, rej) => {
                resolve = res;
                reject = rej;
            });
            const load = { ...coords, key: tileKey(coords.z, coords.x, coords.y),
                           background, onPartial, resolve, reject,
                           cancelled: false, promoted: false };
            load.handle = {
                promise,
                cancel() {
                    load.cancelled = true;
                    reject(new Error('cancelled'));
                },
                promote() {
                    load.promoted = true;
                },
            };
            loads.push(load);
            return load.handle;
        },
        release(drawable) {
            released.push(drawable);
        },
    };
    return { source, loads, released };
}

const tick = () => new Promise((resolve) => setTimeout(resolve, 0));
const R = (x0, x1, y0, y1, z = 3) => ({ z, x0, x1, y0, y1 });

describe('TileGrid.update', () => {
    it('loads the on-screen tiles first, nearest the centre first', () => {
        const { source, loads } = makeSource();
        const grid = new TileGrid({ source });
        const range = R(2, 4, 2, 4);
        grid.update({ z: 3, range, ring: growRange(range) });
        const onScreen = loads.filter((l) => !l.background);
        assert.equal(onScreen.length, 9);
        assert.equal(onScreen[0].key, '3/3/3');
        assert.equal(loads.length, 25);
        assert.ok(loads.slice(0, 9).every((l) => !l.background),
                  'the ring goes out after the view');
    });

    it('requests ring tiles at background priority', () => {
        const { source, loads } = makeSource();
        const grid = new TileGrid({ source });
        const range = R(2, 3, 2, 3);
        grid.update({ z: 3, range, ring: growRange(range) });
        const ring = loads.filter((l) => l.background);
        assert.equal(ring.length, 16 - 4);
        assert.ok(ring.every((l) => l.x < 2 || l.x > 3 || l.y < 2 || l.y > 3));
    });

    it('promotes a ring tile that comes on screen while still loading', () => {
        const { source, loads } = makeSource();
        const grid = new TileGrid({ source });
        const range = R(2, 3, 2, 3);
        grid.update({ z: 3, range, ring: growRange(range) });
        const right = loads.find((l) => l.key === '3/4/2');
        assert.ok(right.background);
        const moved = R(3, 4, 2, 3);
        grid.update({ z: 3, range: moved, ring: growRange(moved) });
        assert.ok(right.promoted);
        assert.equal(loads.filter((l) => l.key === '3/4/2').length, 1,
                     'promoted, not re-requested');
    });

    it('cancels loads that leave the ring', () => {
        const { source, loads } = makeSource();
        const grid = new TileGrid({ source });
        grid.update({ z: 3, range: R(0, 1, 0, 1) });
        grid.update({ z: 3, range: R(5, 6, 5, 6) });
        assert.ok(loads.slice(0, 4).every((l) => l.cancelled));
        assert.equal(grid.tiles.size, 4);
    });

    // keepBuffer: a loaded tile just off screen survives a small pan, so
    // panning back finds it again.
    it('keeps loaded tiles within keepBuffer of the view', async () => {
        const { source, loads, released } = makeSource();
        const grid = new TileGrid({ source, keepBuffer: 2 });
        grid.update({ z: 3, range: R(0, 0, 0, 0) });
        loads[0].resolve({ id: 'a' });
        await tick();
        grid.update({ z: 3, range: R(2, 2, 0, 0) });
        assert.ok(grid.tiles.has('3/0/0'));
        grid.update({ z: 3, range: R(3, 3, 0, 0) });
        assert.ok(!grid.tiles.has('3/0/0'));
        assert.deepEqual(released, [{ id: 'a' }]);
    });

    it('drops loads of another level but keeps its loaded tiles', async () => {
        const { source, loads } = makeSource();
        const grid = new TileGrid({ source });
        grid.update({ z: 3, range: R(0, 1, 0, 0) });
        loads[0].resolve({ id: 'old' });
        await tick();
        grid.update({ z: 4, range: R(0, 1, 0, 0, 4) });
        assert.ok(loads[1].cancelled, 'the unfinished old-level load');
        assert.ok(grid.tiles.has(loads[0].key), 'the finished one stays');
        assert.deepEqual(grid.drawableTilesAt(3).map((t) => t.key),
                         [loads[0].key]);
    });
});

describe('TileGrid loading results', () => {
    it('stores what arrives and reports the change', async () => {
        const { source, loads } = makeSource();
        const changes = [];
        const grid = new TileGrid({ source, onChange: (t) => changes.push(t.key) });
        grid.update({ z: 3, range: R(0, 0, 0, 0) });
        loads[0].resolve({ id: 'a' });
        await tick();
        assert.deepEqual(grid.tiles.get('3/0/0').drawable, { id: 'a' });
        assert.deepEqual(changes, ['3/0/0']);
    });

    it('treats an empty tile as settled with nothing to draw', async () => {
        const { source, loads } = makeSource();
        const grid = new TileGrid({ source });
        grid.update({ z: 3, range: R(0, 0, 0, 0) });
        loads[0].resolve(null);
        await tick();
        assert.ok(grid.isSettled());
        assert.equal(grid.drawableTilesAt(3).length, 0);
        assert.ok(!grid.hasContentAt(3));
    });

    // A dropped request must not hold a zoom swap forever.
    it('counts a failed tile as settled', async () => {
        const { source, loads } = makeSource();
        const grid = new TileGrid({ source });
        grid.update({ z: 3, range: R(0, 0, 0, 0) });
        loads[0].reject(new Error('boom'));
        await tick();
        assert.ok(grid.isSettled());
    });

    it('is settled while only ring tiles are loading', async () => {
        const { source, loads } = makeSource();
        const grid = new TileGrid({ source });
        const range = R(2, 2, 2, 2);
        grid.update({ z: 3, range, ring: growRange(range) });
        assert.ok(!grid.isSettled());
        loads.find((l) => !l.background).resolve({ id: 'c' });
        await tick();
        assert.ok(grid.isSettled());
        assert.ok(grid.stats().loading > 0);
    });

    it('shows partial paints and keeps the last one at the end', async () => {
        const { source, loads, released } = makeSource();
        const grid = new TileGrid({ source });
        grid.update({ z: 3, range: R(0, 0, 0, 0) });
        loads[0].onPartial({ id: 'p1' });
        assert.deepEqual(grid.tiles.get('3/0/0').drawable, { id: 'p1' });
        loads[0].onPartial({ id: 'p2' });
        assert.deepEqual(released, [{ id: 'p1' }]);
        loads[0].resolve(undefined);
        await tick();
        assert.deepEqual(grid.tiles.get('3/0/0').drawable, { id: 'p2' });
        assert.ok(grid.isSettled());
    });
});

describe('TileGrid.refresh', () => {
    // The refresh contract: re-request in place, keep what is on screen until
    // the replacement arrives.
    it('re-requests in place and swaps only when the new image lands', async () => {
        const { source, loads, released } = makeSource();
        const grid = new TileGrid({ source });
        grid.update({ z: 3, range: R(0, 0, 0, 0) });
        loads[0].resolve({ id: 'v1' });
        await tick();
        grid.refresh();
        assert.equal(loads.length, 2);
        assert.deepEqual(grid.tiles.get('3/0/0').drawable, { id: 'v1' });
        loads[1].resolve({ id: 'v2' });
        await tick();
        assert.deepEqual(grid.tiles.get('3/0/0').drawable, { id: 'v2' });
        assert.deepEqual(released, [{ id: 'v1' }]);
    });

    it('discards a superseded result', async () => {
        const { source, loads, released } = makeSource();
        const grid = new TileGrid({ source });
        grid.update({ z: 3, range: R(0, 0, 0, 0) });
        grid.refresh();
        assert.ok(loads[0].cancelled);
        loads[0].resolve({ id: 'late' });
        loads[1].resolve({ id: 'fresh' });
        await tick();
        assert.deepEqual(grid.tiles.get('3/0/0').drawable, { id: 'fresh' });
        assert.ok(!released.some((d) => d && d.id === 'fresh'));
    });

    // Mid-swap the old level is what is on screen: a refresh (a hover on the
    // overlay, say) must not blank it.
    it('leaves other levels alone', async () => {
        const { source, loads } = makeSource();
        const grid = new TileGrid({ source });
        grid.update({ z: 3, range: R(0, 0, 0, 0) });
        loads[0].resolve({ id: 'old' });
        await tick();
        grid.update({ z: 4, range: R(0, 0, 0, 0, 4) });
        const before = loads.length;
        grid.refresh();
        assert.equal(loads.length, before + 1, 'only the level-4 tile');
        assert.deepEqual(grid.tiles.get('3/0/0').drawable, { id: 'old' });
    });
});

describe('TileGrid hold', () => {
    // An animation heads `range` somewhere else; what is on screen meanwhile
    // must not be dropped.
    it('keeps loaded tiles in hold however far the range moved', async () => {
        const { source, loads } = makeSource();
        const grid = new TileGrid({ source });
        const here = R(2, 3, 2, 3);
        grid.update({ z: 3, range: here });
        for (const l of loads) {
            l.resolve({ key: l.key });
        }
        await tick();
        const far = R(20, 21, 20, 21, 3);
        grid.update({ z: 3, range: far, hold: here });
        assert.equal(grid.drawableTilesAt(3).length, 4);
        grid.update({ z: 3, range: far });
        assert.equal(grid.drawableTilesAt(3).length, 0, 'dropped once off screen');
    });
});

// The level above, kept loaded around the view so a zoom out finds it.
describe('TileGrid extra levels', () => {
    const view = R(2, 4, 2, 4);
    const parent = R(0, 2, 0, 2, 2);

    it('loads an extra level after the view, at background priority', () => {
        const { source, loads } = makeSource();
        const grid = new TileGrid({ source });
        grid.update({ z: 3, range: view, extra: [{ z: 2, range: parent }] });
        const extra = loads.filter((l) => l.z === 2);
        assert.equal(extra.length, 9);
        assert.ok(extra.every((l) => l.background));
        assert.ok(loads.indexOf(extra[0]) >= 9, 'after the on-screen tiles');
    });

    it('keeps extra loads in flight while the level stays extra', () => {
        const { source, loads } = makeSource();
        const grid = new TileGrid({ source });
        grid.update({ z: 3, range: view, extra: [{ z: 2, range: parent }] });
        grid.update({ z: 3, range: view, extra: [{ z: 2, range: parent }] });
        assert.equal(loads.filter((l) => l.z === 2).length, 9, 'no repeats');
        assert.ok(loads.every((l) => !l.cancelled));
        // Moving: no extra level, so its loads in flight are cancelled.
        grid.update({ z: 3, range: view });
        assert.ok(loads.filter((l) => l.z === 2).every((l) => l.cancelled));
    });

    it('serves a zoom out from the extra tiles without new requests', async () => {
        const { source, loads } = makeSource();
        const grid = new TileGrid({ source });
        grid.update({ z: 3, range: view, extra: [{ z: 2, range: parent }] });
        for (const l of loads) {
            l.resolve({ key: l.key });
        }
        await tick();
        const before = loads.length;
        grid.update({ z: 2, range: R(0, 2, 0, 2, 2) });
        assert.equal(loads.length, before);
        assert.ok(grid.isSettled());
    });

    it('refreshes extra tiles with the current level', async () => {
        const { source, loads } = makeSource();
        const grid = new TileGrid({ source });
        grid.update({ z: 3, range: view, extra: [{ z: 2, range: parent }] });
        for (const l of loads) {
            l.resolve({ key: l.key });
        }
        await tick();
        const before = loads.length;
        grid.refresh();
        const again = loads.slice(before);
        assert.equal(again.filter((l) => l.z === 2).length, 9);
        assert.ok(again.filter((l) => l.z === 2).every((l) => l.background));
    });

    // Panning at one zoom must not pile up the level it prefetches.
    it('drops loaded extra tiles beyond its range plus the keep buffer',
       async () => {
        const { source, loads, released } = makeSource();
        const grid = new TileGrid({ source, keepBuffer: 1 });
        grid.update({ z: 3, range: view, extra: [{ z: 2, range: parent }] });
        for (const l of loads) {
            l.resolve({ key: l.key });
        }
        await tick();
        // The extra range moves 2 tiles right: x = 0 is outside it and
        // outside the buffer of 1, x = 1 only outside it.
        grid.update({ z: 3, range: view,
                      extra: [{ z: 2, range: R(2, 4, 0, 2, 2) }] });
        const kept = [...grid.tiles.values()].filter((t) => t.z === 2);
        assert.deepEqual([...new Set(kept.map((t) => t.x))].sort(), [1, 2, 3, 4]);
        assert.equal(released.filter((d) => d.key.startsWith('2/0/')).length, 3);
    });

    // A tile kept from before a refresh must not come back stale when its
    // level becomes the current one.
    it('re-requests a tile older than the last refresh when its level returns',
       async () => {
        const { source, loads } = makeSource();
        const grid = new TileGrid({ source });
        grid.update({ z: 3, range: view, extra: [{ z: 2, range: parent }] });
        for (const l of loads) {
            l.resolve({ key: l.key });
        }
        await tick();
        grid.update({ z: 3, range: view });   // moving: no extra
        grid.refresh();                        // the z = 2 tiles are now stale
        const before = loads.length;
        grid.update({ z: 2, range: R(0, 2, 0, 2, 2) });
        const again = loads.slice(before).filter((l) => l.z === 2);
        assert.equal(again.length, 9);
        assert.ok(!grid.isSettled());
    });
});

describe('TileGrid.refresh off the ring', () => {
    // A tile kept only by the keep buffer is off screen: a refresh leaves it
    // as it is, and it is fetched again once it is back in the ring.
    it('leaves keep-buffer tiles alone until they are back in the ring',
       async () => {
        const { source, loads, released } = makeSource();
        const grid = new TileGrid({ source, keepBuffer: 2 });
        const at = (x0) => {
            const range = R(x0, x0 + 2, 2, 4);
            grid.update({ z: 3, range, ring: growRange(range) });
        };
        at(2);
        for (const l of loads) {
            l.resolve({ key: l.key });
        }
        await tick();
        at(4);   // ring x 3..7: x = 2 is kept by the buffer only
        const kept = grid.tiles.get(tileKey(3, 2, 3));
        assert.ok(kept && kept.drawable && !kept.loading);
        const before = loads.length;
        grid.refresh();
        at(4);
        assert.ok(!loads.slice(before).some((l) => l.key.startsWith('3/2/')),
                  'not fetched off the ring');
        assert.equal(grid.tiles.get(tileKey(3, 2, 3)), kept);
        assert.ok(!released.some((d) => d.key === '3/2/3'), 'image kept');
        at(3);   // ring x 2..6: back in
        assert.ok(loads.slice(before).some((l) => l.key === '3/2/3'),
                  'fetched again in the ring');
        assert.ok(kept.drawable, 'shown until the new one arrives');
    });

    it('does not fetch stale tiles of a returning level off its ring',
       async () => {
        const { source, loads, released } = makeSource();
        const grid = new TileGrid({ source, keepBuffer: 2 });
        const range3 = R(2, 4, 2, 4);
        grid.update({ z: 3, range: range3, ring: growRange(range3) });
        for (const l of loads) {
            l.resolve({ key: l.key });
        }
        await tick();
        grid.update({ z: 4, range: R(4, 8, 4, 8, 4) });
        grid.refresh();   // level 3 goes stale
        const before = loads.length;
        const back = R(3, 5, 2, 4);   // ring x 2..6
        grid.update({ z: 3, range: back, ring: growRange(back) });
        assert.ok(!loads.slice(before).some((l) => l.z === 3 && l.x === 1),
                  'x = 1 is off the ring');
        assert.ok(grid.tiles.get(tileKey(3, 1, 3))?.drawable, 'kept');
        assert.ok(!released.some((d) => d.key === '3/1/3'));
    });
});

describe('TileGrid.prune', () => {
    it('drops other levels, releasing and cancelling them', async () => {
        const { source, loads, released } = makeSource();
        const grid = new TileGrid({ source });
        grid.update({ z: 3, range: R(0, 0, 0, 0) });
        loads[0].resolve({ id: 'old' });
        await tick();
        grid.update({ z: 4, range: R(0, 0, 0, 0, 4) });
        grid.prune([4]);
        assert.ok(!grid.tiles.has('3/0/0'));
        assert.deepEqual(released, [{ id: 'old' }]);
        assert.ok(grid.tiles.has('4/0/0'));
    });

    it('clear() cancels everything', () => {
        const { source, loads } = makeSource();
        const grid = new TileGrid({ source });
        grid.update({ z: 3, range: R(0, 1, 0, 1) });
        grid.clear();
        assert.equal(grid.tiles.size, 0);
        assert.ok(loads.every((l) => l.cancelled));
    });
});
