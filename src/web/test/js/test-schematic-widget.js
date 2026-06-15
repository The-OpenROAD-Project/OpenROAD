// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import {
    canonicalizeCell, canonicalizeForSkin,
} from '../../src/schematic-widget.js';

// Helper: a server-style cell object.
function cell(extra) {
    return Object.assign({
        hide_name: 0,
        attributes: { ref: 'g1' },
        parameters: {},
    }, extra);
}

describe('canonicalizeCell', () => {
    it('maps simple gates to skin types and A/B/Y pids', () => {
        const got = canonicalizeCell(cell({
            type: 'NOR2_X1', gate_kind: 'nor',
            port_directions: { A1: 'input', A2: 'input', ZN: 'output' },
            connections: { A1: [2], A2: [3], ZN: [4] },
        }));
        assert.equal(got.type, '$_NOR_');
        assert.deepEqual(Object.keys(got.connections), ['A', 'B', 'Y']);
        assert.deepEqual(got.connections, { A: [2], B: [3], Y: [4] });
        assert.deepEqual(got.port_directions, { A: 'input', B: 'input', Y: 'output' });
        // The real pin names are kept (pid -> name) so the symbol can be labelled.
        assert.deepEqual(got.port_labels, { A: 'A1', B: 'A2', Y: 'ZN' });
    });

    it('maps an inverter to $_NOT_ with A/Y', () => {
        const got = canonicalizeCell(cell({
            type: 'INV_X1', gate_kind: 'not',
            port_directions: { A: 'input', ZN: 'output' },
            connections: { A: [7], ZN: [8] },
        }));
        assert.equal(got.type, '$_NOT_');
        assert.deepEqual(got.connections, { A: [7], Y: [8] });
    });

    it('maps a buffer to $_BUF_', () => {
        const got = canonicalizeCell(cell({
            type: 'BUF_X1', gate_kind: 'buf',
            port_directions: { A: 'input', Z: 'output' },
            connections: { A: [1], Z: [2] },
        }));
        assert.equal(got.type, '$_BUF_');
        assert.deepEqual(got.connections, { A: [1], Y: [2] });
    });

    it('maps a 3-input gate to a per-arity symbol (nand3)', () => {
        const got = canonicalizeCell(cell({
            type: 'NAND3_X1', gate_kind: 'nand',
            port_directions: { A1: 'input', A2: 'input', A3: 'input', ZN: 'output' },
            connections: { A1: [2], A2: [3], A3: [4], ZN: [5] },
        }));
        assert.equal(got.type, 'nand3');
        assert.deepEqual(got.connections, { A: [2], B: [3], C: [4], Y: [5] });
        assert.deepEqual(got.port_labels, { A: 'A1', B: 'A2', C: 'A3', Y: 'ZN' });
    });

    it('leaves an unsupported-width gate (>4 inputs) as a box', () => {
        const orig = cell({
            type: 'NAND5_X1', gate_kind: 'nand',
            port_directions: {
                A1: 'input', A2: 'input', A3: 'input', A4: 'input', A5: 'input',
                ZN: 'output',
            },
            connections: { A1: [1], A2: [2], A3: [3], A4: [4], A5: [5], ZN: [6] },
        });
        const got = canonicalizeCell(orig);
        assert.equal(got.type, 'NAND5_X1');        // unchanged -> generic box
        assert.equal(got.port_labels, undefined);
    });

    it('drops power/ground pins not part of the gate', () => {
        const got = canonicalizeCell(cell({
            type: 'NAND2_X1', gate_kind: 'nand',
            port_directions: {
                A1: 'input', A2: 'input', ZN: 'output',
                VDD: 'inout', VSS: 'inout',
            },
            connections: { A1: [2], A2: [3], ZN: [4], VDD: [5], VSS: [6] },
        }));
        assert.equal(got.type, '$_NAND_');
        assert.deepEqual(Object.keys(got.connections).sort(), ['A', 'B', 'Y']);
    });

    it('maps AOI21 to its symbol with the literal as A and the AND pair as B,C', () => {
        const got = canonicalizeCell(cell({
            type: 'AOI21_X2', gate_kind: 'aoi',
            gate_terms: [['A'], ['B1', 'B2']],
            port_directions: { A: 'input', B1: 'input', B2: 'input', ZN: 'output' },
            connections: { A: [4], B1: [8], B2: [11], ZN: [12] },
        }));
        assert.equal(got.type, 'aoi21');
        // smallest term (literal A) -> pid A; the AND pair -> B, C; output -> Y
        assert.deepEqual(got.connections, { A: [4], B: [8], C: [11], Y: [12] });
        // Real pin names preserved per port id for labelling.
        assert.deepEqual(got.port_labels, { A: 'A', B: 'B1', C: 'B2', Y: 'ZN' });
    });

    it('maps OAI22 to its symbol with four inputs A..D', () => {
        const got = canonicalizeCell(cell({
            type: 'OAI22_X1', gate_kind: 'oai',
            gate_terms: [['A1', 'A2'], ['B1', 'B2']],
            port_directions: {
                A1: 'input', A2: 'input', B1: 'input', B2: 'input', ZN: 'output',
            },
            connections: { A1: [1], A2: [2], B1: [3], B2: [4], ZN: [5] },
        }));
        assert.equal(got.type, 'oai22');
        assert.deepEqual(Object.keys(got.connections), ['A', 'B', 'C', 'D', 'Y']);
    });

    it('leaves unsupported compound arities unchanged (labelled box)', () => {
        const orig = cell({
            type: 'AOI211_X1', gate_kind: 'aoi',
            gate_terms: [['A'], ['B'], ['C1', 'C2']],
            port_directions: { A: 'input', B: 'input', C1: 'input', C2: 'input', ZN: 'output' },
            connections: { A: [1], B: [2], C1: [3], C2: [4], ZN: [5] },
        });
        const got = canonicalizeCell(orig);
        assert.equal(got.type, 'AOI211_X1');           // type unchanged
        assert.ok(got.connections.C1 && got.connections.ZN);  // real pins kept
        assert.equal(got.port_labels, undefined);      // no remap -> no labels
    });

    it('leaves cells without a gate_kind unchanged', () => {
        const orig = cell({
            type: 'DFF_X1',
            port_directions: { D: 'input', CK: 'input', Q: 'output' },
            connections: { D: [1], CK: [2], Q: [3] },
        });
        assert.strictEqual(canonicalizeCell(orig), orig);
    });
});

describe('canonicalizeForSkin', () => {
    it('rewrites recognised cells but preserves instance-name keys', () => {
        const json = { modules: { top: { cells: {
            _983_: {
                type: 'NOR2_X1', gate_kind: 'nor', attributes: { ref: '_983_' },
                port_directions: { A1: 'input', A2: 'input', ZN: 'output' },
                connections: { A1: [2], A2: [3], ZN: [4] },
            },
            myff: {
                type: 'DFF_X1', attributes: { ref: 'myff' },
                port_directions: { D: 'input', Q: 'output' },
                connections: { D: [4], Q: [5] },
            },
        } } } };
        const out = canonicalizeForSkin(json);
        const cells = out.modules.top.cells;
        // Keys (instance names) preserved.
        assert.deepEqual(Object.keys(cells).sort(), ['_983_', 'myff']);
        assert.equal(cells._983_.type, '$_NOR_');     // gate rewritten
        assert.equal(cells.myff.type, 'DFF_X1');       // non-gate untouched
        // Input is not mutated.
        assert.equal(json.modules.top.cells._983_.type, 'NOR2_X1');
    });

    it('returns the input unchanged when there is no top module', () => {
        const json = { foo: 1 };
        assert.strictEqual(canonicalizeForSkin(json), json);
    });
});
