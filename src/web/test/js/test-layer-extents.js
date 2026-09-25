// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';

import {
    LayerExtents, tileMayHaveContent,
} from '../../src/layer-extents.js';

// metal1 fills the left half of the grid, metal9 has nothing, and _instances
// is a pseudo layer the server does not list.
const RESPONSE = {
    supported: true,
    layers: { metal1: [0, 0, 0.5, 1], metal9: null },
};

function adopted(resp = RESPONSE) {
    const extents = new LayerExtents();
    assert.equal(extents.apply(extents.invalidate(), resp), true);
    return extents;
}

describe('LayerExtents.mayHaveContent', () => {
    it('requests everything until extents arrive', () => {
        const extents = new LayerExtents();
        assert.equal(
            extents.mayHaveContent('metal9', { x: 0, y: 0, z: 0 }, {}), true);
    });

    it('skips a layer the server reports empty', () => {
        const extents = adopted();
        for (const coords of [{ x: 0, y: 0, z: 0 }, { x: 5, y: 3, z: 4 }]) {
            assert.equal(extents.mayHaveContent('metal9', coords, {}), false);
        }
    });

    it('requests tiles that overlap the extent and skips the rest', () => {
        const extents = adopted();
        // z=2: four columns; metal1 covers the left two.
        assert.equal(
            extents.mayHaveContent('metal1', { x: 0, y: 3, z: 2 }, {}), true);
        assert.equal(
            extents.mayHaveContent('metal1', { x: 1, y: 0, z: 2 }, {}), true);
        assert.equal(
            extents.mayHaveContent('metal1', { x: 3, y: 1, z: 2 }, {}), false);
    });

    it('keeps tiles next to the extent, which the apron can reach', () => {
        const extents = adopted();
        // Column 2 at z=2 starts exactly where metal1 ends.
        assert.equal(
            extents.mayHaveContent('metal1', { x: 2, y: 0, z: 2 }, {}), true);
        // A shape a sliver short of the tile edge still counts.
        const near = adopted({ supported: true,
                               layers: { metal1: [0, 0, 0.49, 1] } });
        assert.equal(
            near.mayHaveContent('metal1', { x: 2, y: 0, z: 2 }, {}), true);
    });

    it('always requests layers the server did not list', () => {
        const extents = adopted();
        assert.equal(
            extents.mayHaveContent('_instances', { x: 3, y: 3, z: 2 }, {}),
            true);
    });

    for (const flag of ['tracks_pref', 'tracks_non_pref', 'debug',
                        'debug_renderers']) {
        it(`skips nothing while ${flag} is on`, () => {
            const extents = adopted();
            assert.equal(extents.mayHaveContent(
                'metal9', { x: 0, y: 0, z: 0 }, { [flag]: true }), true);
        });
    }

    it('counts a gated source only while its flag is not off', () => {
        // RX carries nothing but master obstructions, which draw only with
        // Blockages on.
        const extents = adopted({
            supported: true,
            layers: { RX: null },
            gated: { blockages: { RX: [0, 0, 1, 1] }, fills: {} },
        });
        const coords = { x: 0, y: 0, z: 0 };
        assert.equal(
            extents.mayHaveContent('RX', coords, { blockages: true }), true);
        assert.equal(
            extents.mayHaveContent('RX', coords, { blockages: false }), false);
        // A flag the request leaves out takes the server default; do not
        // assume it is off.
        assert.equal(extents.mayHaveContent('RX', coords, {}), true);
    });

    it('tests a gated extent against the tile like any other', () => {
        const extents = adopted({
            supported: true,
            layers: { M6: null },
            gated: { routing_obstructions: { M6: [0, 0, 0.25, 0.25] } },
        });
        const vis = { routing_obstructions: true };
        assert.equal(
            extents.mayHaveContent('M6', { x: 0, y: 0, z: 2 }, vis), true);
        assert.equal(
            extents.mayHaveContent('M6', { x: 3, y: 3, z: 2 }, vis), false);
    });

    it('invalidate() drops the gated extents too', () => {
        const extents = adopted({
            supported: true,
            layers: { RX: null },
            gated: { blockages: { RX: [0, 0, 1, 1] } },
        });
        const generation = extents.invalidate();
        extents.apply(generation, { supported: true, layers: { RX: null } });
        assert.equal(extents.mayHaveContent(
            'RX', { x: 0, y: 0, z: 0 }, { blockages: true }), false);
    });

    it('skips nothing when the server does not support extents', () => {
        const extents = new LayerExtents();
        assert.equal(
            extents.apply(extents.invalidate(), { supported: false }), false);
        assert.equal(
            extents.mayHaveContent('metal9', { x: 0, y: 0, z: 0 }, {}), true);
    });

    it('treats a malformed extent as having content', () => {
        const extents = adopted({ supported: true, layers: { metal1: [0, 1] } });
        assert.equal(
            extents.mayHaveContent('metal1', { x: 3, y: 3, z: 2 }, {}), true);
    });
});

describe('LayerExtents: staying current across design edits', () => {
    it('invalidate() drops the extents, so a redraw requests everything', () => {
        const extents = adopted();
        extents.invalidate();
        assert.equal(
            extents.mayHaveContent('metal9', { x: 0, y: 0, z: 0 }, {}), true);
    });

    it('drops a reply to a fetch issued before the latest invalidate', () => {
        const extents = new LayerExtents();
        const stale = extents.invalidate();
        const current = extents.invalidate();
        assert.equal(extents.apply(stale, RESPONSE), false);
        assert.equal(
            extents.mayHaveContent('metal9', { x: 0, y: 0, z: 0 }, {}), true);
        assert.equal(extents.apply(current, RESPONSE), true);
        assert.equal(
            extents.mayHaveContent('metal9', { x: 0, y: 0, z: 0 }, {}), false);
    });

    it('refetch() adopts the reply to the latest fetch only', async () => {
        const extents = new LayerExtents();
        const replies = [];
        const request = (msg) => {
            assert.deepEqual(msg, { type: 'layer_extents' });
            return new Promise((resolve) => replies.push(resolve));
        };
        // A refresh push lands while the first fetch is still in flight: the
        // first reply describes the design before the edit that put shapes on
        // metal9, and must not win.
        const first = extents.refetch(request);
        const second = extents.refetch(request);
        replies[1]({ supported: true,
                     layers: { metal1: [0, 0, 0.5, 1], metal9: [0, 0, 1, 1] } });
        replies[0](RESPONSE);
        assert.equal(await first, false);
        assert.equal(await second, true);
        assert.equal(
            extents.mayHaveContent('metal9', { x: 0, y: 0, z: 0 }, {}), true);
    });

    it('refetch() skips nothing when the request fails', async () => {
        const extents = adopted();
        const ok = await extents.refetch(() => Promise.reject(new Error('x')));
        assert.equal(ok, false);
        assert.equal(
            extents.mayHaveContent('metal9', { x: 0, y: 0, z: 0 }, {}), true);
    });
});

describe('tileMayHaveContent', () => {
    it('requests everything when the app has no extents', () => {
        assert.equal(tileMayHaveContent({ app: null }, 'metal9',
                                        { x: 0, y: 0, z: 0 }), true);
        assert.equal(tileMayHaveContent({ app: {} }, 'metal9',
                                        { x: 0, y: 0, z: 0 }), true);
    });

    it('consults the app extents with the layer visibility', () => {
        const app = { layerExtents: adopted() };
        const coords = { x: 0, y: 0, z: 0 };
        assert.equal(tileMayHaveContent({ app, visibility: {} },
                                        'metal9', coords), false);
        assert.equal(tileMayHaveContent(
            { app, visibility: { tracks_pref: true } }, 'metal9', coords),
                     true);
    });
});
