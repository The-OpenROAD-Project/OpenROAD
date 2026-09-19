// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import {
    buildTreeIndex, computeEffectiveColors, fmtArea, fmtInt, isHidden,
    serializeColorMap,
} from '../../src/color-tree.js';

// A ── B ── C  plus a sibling D of B, which is the shape that distinguishes
// "nearest collapsed ancestor" from "highest collapsed ancestor".
const NODES = [
    { id: 0, parent_id: -1, odb_id: 10, color: [1, 1, 1] },
    { id: 1, parent_id: 0, odb_id: 11, color: [2, 2, 2] },
    { id: 2, parent_id: 1, odb_id: 12, color: [3, 3, 3] },
    { id: 3, parent_id: 0, odb_id: 13, color: [4, 4, 4] },
];

function makeState(nodes = NODES) {
    const state = new Map();
    for (const n of nodes) {
        state.set(n.odb_id, { color: n.color, effectiveColor: n.color,
                              visible: true });
    }
    return state;
}

describe('buildTreeIndex', () => {
    it('indexes children and emits DFS rows with depth', () => {
        const { childrenMap, nodeMap, rows } = buildTreeIndex(NODES);
        assert.deepEqual(childrenMap.get(0), [1, 3]);
        assert.deepEqual(childrenMap.get(1), [2]);
        assert.equal(nodeMap.get(2).odb_id, 12);
        assert.deepEqual(rows.map(r => [r.id, r.depth]),
                         [[0, 0], [1, 1], [2, 2], [3, 1]]);
    });

    it('emits every parent before its children', () => {
        const { rows } = buildTreeIndex(NODES);
        const seen = new Set();
        for (const row of rows) {
            const node = NODES.find(n => n.id === row.id);
            if (node.parent_id >= 0) {
                assert.ok(seen.has(node.parent_id),
                          `parent of ${row.id} must come first`);
            }
            seen.add(row.id);
        }
    });
});

describe('isHidden', () => {
    it('hides a node under any collapsed ancestor', () => {
        const { nodeMap } = buildTreeIndex(NODES);
        assert.equal(isHidden(nodeMap.get(2), nodeMap, new Set()), false);
        assert.equal(isHidden(nodeMap.get(2), nodeMap, new Set([1])), true);
        // Collapsed grandparent, expanded parent.
        assert.equal(isHidden(nodeMap.get(2), nodeMap, new Set([0])), true);
    });
});

describe('computeEffectiveColors', () => {
    it('leaves every node its own color when nothing is collapsed', () => {
        const { nodeMap, rows } = buildTreeIndex(NODES);
        const state = makeState();
        computeEffectiveColors(rows, nodeMap, state, new Set());
        for (const n of NODES) {
            assert.deepEqual(state.get(n.odb_id).effectiveColor, n.color);
        }
    });

    it('makes a collapsed node paint its whole subtree', () => {
        const { nodeMap, rows } = buildTreeIndex(NODES);
        const state = makeState();
        computeEffectiveColors(rows, nodeMap, state, new Set([1]));
        assert.deepEqual(state.get(12).effectiveColor, [2, 2, 2]);
        // A sibling outside that subtree is untouched.
        assert.deepEqual(state.get(13).effectiveColor, [4, 4, 4]);
    });

    // The case a nearest-ancestor rule gets wrong: B is expanded but sits
    // inside a collapsed A, so C must paint A's color, not its own.
    it('gives the HIGHEST collapsed ancestor the subtree, through expanded '
       + 'nodes', () => {
        const { nodeMap, rows } = buildTreeIndex(NODES);
        const state = makeState();
        computeEffectiveColors(rows, nodeMap, state, new Set([0]));
        assert.deepEqual(state.get(11).effectiveColor, [1, 1, 1]);
        assert.deepEqual(state.get(12).effectiveColor, [1, 1, 1]);
        assert.deepEqual(state.get(13).effectiveColor, [1, 1, 1]);
    });

    it('lets the outer collapse win over an inner one', () => {
        const { nodeMap, rows } = buildTreeIndex(NODES);
        const state = makeState();
        computeEffectiveColors(rows, nodeMap, state, new Set([0, 1]));
        assert.deepEqual(state.get(12).effectiveColor, [1, 1, 1]);
    });

    // Structural rows (the Hierarchy panel's "Leaf instances" folders) carry no
    // color and are absent from the state map; they must not break the chain.
    it('propagates through rows that have no color of their own', () => {
        const nodes = [
            { id: 0, parent_id: -1, odb_id: 10, color: [1, 1, 1] },
            { id: 1, parent_id: 0 },                    // structural, no odb_id
            { id: 2, parent_id: 1, odb_id: 12, color: [3, 3, 3] },
        ];
        const { nodeMap, rows } = buildTreeIndex(nodes);
        const state = makeState(nodes.filter(n => n.odb_id != null));
        computeEffectiveColors(rows, nodeMap, state, new Set([0]));
        assert.deepEqual(state.get(12).effectiveColor, [1, 1, 1]);
    });
});

describe('serializeColorMap', () => {
    it('emits id:r,g,b,a for visible entries only', () => {
        const state = makeState();
        state.get(11).visible = false;
        assert.equal(serializeColorMap(state),
                     '10:1,1,1,100;12:3,3,3,100;13:4,4,4,100');
    });

    it('is empty when nothing is visible', () => {
        const state = makeState();
        for (const st of state.values()) st.visible = false;
        assert.equal(serializeColorMap(state), '');
    });
});

describe('number formatting', () => {
    it('formats counts, blank for missing', () => {
        assert.equal(fmtInt(0), '0');
        assert.equal(fmtInt(42), '42');
        assert.equal(fmtInt(null), '');
    });

    it('formats area in μm², mm² when large, DBU² in DBU mode', () => {
        const app = { showDbu: false, getDbuPerMicron: () => 1000 };
        assert.equal(fmtArea(app, 1.5), '1.500 μm²');
        assert.equal(fmtArea(app, 2e6), '2.000 mm²');
        assert.equal(fmtArea(app, null), '');
        assert.equal(fmtArea({ ...app, showDbu: true }, 2), '2000000');
    });
});
