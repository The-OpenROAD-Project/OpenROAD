// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import {
    computeClockTreeLayout,
    kNodeSpacing, kTopMargin, kBottomMargin, kLeftMargin, kRightMargin,
} from '../../src/clock-tree-widget.js';

describe('computeClockTreeLayout', () => {
    it('returns empty layout for empty nodes', () => {
        const result = computeClockTreeLayout({
            nodes: [], min_arrival: 0, max_arrival: 0,
        });
        assert.equal(result.layout.length, 0);
        assert.equal(result.layoutWidth, 0);
        assert.equal(result.layoutHeight, 0);
        assert.equal(result.sceneHeight, 0);
    });

    it('returns empty layout for null nodes', () => {
        const result = computeClockTreeLayout({
            nodes: null, min_arrival: 0, max_arrival: 0,
        });
        assert.equal(result.layout.length, 0);
    });

    it('returns empty layout for undefined nodes', () => {
        const result = computeClockTreeLayout({
            min_arrival: 0, max_arrival: 0,
        });
        assert.equal(result.layout.length, 0);
    });

    it('positions a single root node at top', () => {
        const result = computeClockTreeLayout({
            nodes: [{
                id: 0, parent_id: -1, name: 'clk', type: 'root',
                arrival: 0, delay: 0, fanout: 0, level: 0,
            }],
            min_arrival: 0,
            max_arrival: 1,
            time_unit: 'ns',
        });
        assert.equal(result.layout.length, 1);
        const root = result.layout[0];
        assert.equal(root.id, 0);
        // arrival == min_arrival → y at top margin
        assert.equal(root.y, kTopMargin);
        // single node, width=1, bin [0,1), center at 0.5
        assert.equal(root.x, kLeftMargin + 0.5 * kNodeSpacing);
        assert.equal(result.timeUnit, 'ns');
    });

    it('positions children by subtree width', () => {
        // Root (width=2) → two leaf children (width=1 each)
        const result = computeClockTreeLayout({
            nodes: [
                { id: 0, parent_id: -1, name: 'root', type: 'root',
                  arrival: 0, delay: 0, fanout: 2, level: 0 },
                { id: 1, parent_id: 0, name: 'reg1', type: 'register',
                  arrival: 0.5, delay: 0, fanout: 0, level: 1 },
                { id: 2, parent_id: 0, name: 'reg2', type: 'register',
                  arrival: 1.0, delay: 0, fanout: 0, level: 1 },
            ],
            min_arrival: 0,
            max_arrival: 1,
        });
        assert.equal(result.layout.length, 3);

        // Root has width 2, centered at bin 1.0
        const root = result.layout.find(n => n.id === 0);
        assert.equal(root.x, kLeftMargin + 1 * kNodeSpacing);

        // reg1 has width 1, bin [0,1), center 0.5
        const reg1 = result.layout.find(n => n.id === 1);
        assert.equal(reg1.x, kLeftMargin + 0.5 * kNodeSpacing);

        // reg2 has width 1, bin [1,2), center 1.5
        const reg2 = result.layout.find(n => n.id === 2);
        assert.equal(reg2.x, kLeftMargin + 1.5 * kNodeSpacing);

        // reg2 (later arrival) should be below reg1 (earlier arrival)
        assert.ok(reg2.y > reg1.y);
    });

    it('computes correct layout dimensions', () => {
        const result = computeClockTreeLayout({
            nodes: [
                { id: 0, parent_id: -1, name: 'root', type: 'root',
                  arrival: 0, delay: 0, fanout: 3, level: 0 },
                { id: 1, parent_id: 0, name: 'a', type: 'register',
                  arrival: 1, delay: 0, fanout: 0, level: 1 },
                { id: 2, parent_id: 0, name: 'b', type: 'register',
                  arrival: 1, delay: 0, fanout: 0, level: 1 },
                { id: 3, parent_id: 0, name: 'c', type: 'register',
                  arrival: 1, delay: 0, fanout: 0, level: 1 },
            ],
            min_arrival: 0,
            max_arrival: 1,
        });
        // Total width = 3 leaves
        assert.equal(result.layoutWidth,
            kLeftMargin + 3 * kNodeSpacing + kRightMargin);
        // Height = top + sceneHeight + bottom
        const expectedScene = Math.max(200, 3 * kNodeSpacing * 0.6);
        assert.equal(result.layoutHeight,
            kTopMargin + expectedScene + kBottomMargin);
    });

    it('builds layoutById map', () => {
        const result = computeClockTreeLayout({
            nodes: [
                { id: 0, parent_id: -1, name: 'clk', type: 'root',
                  arrival: 0, delay: 0, fanout: 1, level: 0 },
                { id: 1, parent_id: 0, name: 'buf', type: 'buffer',
                  arrival: 0.1, delay: 0.05, fanout: 0, level: 1 },
            ],
            min_arrival: 0,
            max_arrival: 0.2,
        });
        assert.ok(result.layoutById instanceof Map);
        assert.ok(result.layoutById.has(0));
        assert.ok(result.layoutById.has(1));
        assert.equal(result.layoutById.get(1).name, 'buf');
    });

    it('preserves node metadata in layout', () => {
        const result = computeClockTreeLayout({
            nodes: [{
                id: 5, parent_id: -1, name: 'mybuf', pin_name: 'Z',
                type: 'buffer', arrival: 0.3, delay: 0.1,
                fanout: 4, level: 2,
            }],
            min_arrival: 0,
            max_arrival: 1,
        });
        const item = result.layout[0];
        assert.equal(item.name, 'mybuf');
        assert.equal(item.pin_name, 'Z');
        assert.equal(item.type, 'buffer');
        assert.equal(item.arrival, 0.3);
        assert.equal(item.delay, 0.1);
        assert.equal(item.fanout, 4);
        assert.equal(item.level, 2);
        assert.equal(item.parent_id, -1);
    });

    it('handles deep tree hierarchy', () => {
        // root → buf1 → buf2 → reg (single chain)
        const result = computeClockTreeLayout({
            nodes: [
                { id: 0, parent_id: -1, name: 'clk', type: 'root',
                  arrival: 0, delay: 0, fanout: 1, level: 0 },
                { id: 1, parent_id: 0, name: 'buf1', type: 'buffer',
                  arrival: 0.1, delay: 0.05, fanout: 1, level: 1 },
                { id: 2, parent_id: 1, name: 'buf2', type: 'buffer',
                  arrival: 0.2, delay: 0.05, fanout: 1, level: 2 },
                { id: 3, parent_id: 2, name: 'reg', type: 'register',
                  arrival: 0.3, delay: 0, fanout: 0, level: 3 },
            ],
            min_arrival: 0,
            max_arrival: 0.3,
        });
        assert.equal(result.layout.length, 4);

        // Single chain: all have width 1, so all same x
        const xs = result.layout.map(n => n.x);
        assert.ok(xs.every(x => x === xs[0]),
            'all nodes in single chain should have same x');

        // Y values should increase with arrival
        for (let i = 0; i < result.layout.length - 1; i++) {
            assert.ok(result.layout[i].y < result.layout[i + 1].y,
                `node ${i} should be above node ${i + 1}`);
        }
    });

    it('handles asymmetric tree', () => {
        // root → buf1 (→ reg1, reg2), reg3
        const result = computeClockTreeLayout({
            nodes: [
                { id: 0, parent_id: -1, name: 'root', type: 'root',
                  arrival: 0, delay: 0, fanout: 2, level: 0 },
                { id: 1, parent_id: 0, name: 'buf1', type: 'buffer',
                  arrival: 0.1, delay: 0.05, fanout: 2, level: 1 },
                { id: 2, parent_id: 1, name: 'reg1', type: 'register',
                  arrival: 0.3, delay: 0, fanout: 0, level: 2 },
                { id: 3, parent_id: 1, name: 'reg2', type: 'register',
                  arrival: 0.3, delay: 0, fanout: 0, level: 2 },
                { id: 4, parent_id: 0, name: 'reg3', type: 'register',
                  arrival: 0.2, delay: 0, fanout: 0, level: 1 },
            ],
            min_arrival: 0,
            max_arrival: 0.3,
        });
        assert.equal(result.layout.length, 5);

        // buf1 subtree has width 2, reg3 has width 1 → total width 3
        assert.equal(result.layoutWidth,
            kLeftMargin + 3 * kNodeSpacing + kRightMargin);

        // buf1 centered over its 2 children, reg3 in its own bin
        const buf1 = result.layout.find(n => n.id === 1);
        const reg1 = result.layout.find(n => n.id === 2);
        const reg2 = result.layout.find(n => n.id === 3);
        const reg3 = result.layout.find(n => n.id === 4);

        // buf1 centered between reg1 and reg2
        assert.equal(buf1.x, (reg1.x + reg2.x) / 2);
        // reg3 should be to the right of reg2
        assert.ok(reg3.x > reg2.x);
    });

    it('defaults timeUnit to empty string', () => {
        const result = computeClockTreeLayout({
            nodes: [{ id: 0, parent_id: -1, name: 'clk', type: 'root',
                      arrival: 0, delay: 0, fanout: 0, level: 0 }],
            min_arrival: 0,
            max_arrival: 1,
        });
        assert.equal(result.timeUnit, '');
    });

    it('handles equal min and max arrival', () => {
        // When min == max, timeRange defaults to 1 to avoid division by zero
        const result = computeClockTreeLayout({
            nodes: [
                { id: 0, parent_id: -1, name: 'clk', type: 'root',
                  arrival: 0.5, delay: 0, fanout: 1, level: 0 },
                { id: 1, parent_id: 0, name: 'reg', type: 'register',
                  arrival: 0.5, delay: 0, fanout: 0, level: 1 },
            ],
            min_arrival: 0.5,
            max_arrival: 0.5,
        });
        assert.equal(result.layout.length, 2);
        // Both nodes have same arrival → same y
        assert.equal(result.layout[0].y, result.layout[1].y);
    });
});
