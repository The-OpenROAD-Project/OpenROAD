// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Renders cells through the same netlistsvg package and skin as the viewer,
// and checks that every pin's wire belongs to that pin's net.

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { createRequire } from 'node:module';

import { compactSvgText } from '../../esbuild.config.mjs';

const require = createRequire(import.meta.url);
const netlistsvg = require('netlistsvg');
const SKIN = readFileSync(new URL('../../src/openroad_skin.svg', import.meta.url), 'utf8');

function cell(type, directions, connections) {
    const ports = {};
    for (const [pin, dir] of Object.entries(directions)) {
        ports[pin.toLowerCase()] = { direction: dir, bits: connections[pin] };
    }
    return {
        modules: { top: {
            ports,
            cells: { u1: { type, port_directions: directions, connections } },
        } },
    };
}

const CELLS = {
    mux: cell('$_MUX_', { A: 'input', B: 'input', S: 'input', Y: 'output' },
              { A: [2], B: [3], S: [5], Y: [4] }),
    dff: cell('$_DFF_P_', { C: 'input', D: 'input', Q: 'output' },
              { C: [3], D: [2], Q: [4] }),
    aoi21: cell('aoi21', { A: 'input', B: 'input', C: 'input', Y: 'output' },
                { A: [2], B: [3], C: [4], Y: [5] }),
};

// The nets whose wires end on each of u1's pins, keyed by pin.
function netsAtPins(svg) {
    const group = svg.match(
        /<g s:type="[^"]*" transform="translate\(([-\d.]+),\s*([-\d.]+)\)"[^>]*id="cell_u1">([\s\S]*?)<\/g>\s*\n/);
    assert.ok(group, 'cell u1 is not in the rendered SVG');
    const [cx, cy] = [Number(group[1]), Number(group[2])];
    const lines = [...svg.matchAll(
        /<line x1="([-\d.]+)" x2="([-\d.]+)" y1="([-\d.]+)" y2="([-\d.]+)" class="(net_\d+)"\/>/g)];
    const nets = {};
    for (const [, sx, sy, pid] of group[3].matchAll(/<g s:x="([-\d.]+)" s:y="([-\d.]+)" s:pid="(\w+)"\/>/g)) {
        const [px, py] = [cx + Number(sx), cy + Number(sy)];
        const at = (x, y) => Math.abs(Number(x) - px) < 0.01 && Math.abs(Number(y) - py) < 0.01;
        nets[pid] = lines.filter(l => at(l[1], l[3]) || at(l[2], l[4])).map(l => l[5]);
    }
    return nets;
}

describe('netlistsvg pins', () => {
    for (const [name, netlist] of Object.entries(CELLS)) {
        it(`wires every ${name} pin to its own net`, async () => {
            const svg = await netlistsvg.render(SKIN, netlist);
            const nets = netsAtPins(svg);
            const { connections } = netlist.modules.top.cells.u1;
            for (const [pin, [bit]] of Object.entries(connections)) {
                assert.ok(pin in nets, `the skin draws no ${pin} pin on ${name}`);
                assert.ok(nets[pin].includes(`net_${bit}`),
                          `${name}.${pin} should end net_${bit}, got ${JSON.stringify(nets)}`);
            }
        });
    }
});

// The bundle carries the skin with the whitespace between its tags removed.
describe('the compacted skin', () => {
    for (const [name, netlist] of Object.entries(CELLS)) {
        it(`draws ${name} as the source skin does`, async () => {
            const source = await netlistsvg.render(SKIN, netlist);
            const compacted = await netlistsvg.render(compactSvgText(SKIN), netlist);
            assert.equal(compacted, source);
        });
    }
});
