// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Renders design names that hold markup characters through the real
// netlistsvg and skin, and checks they come out as the names themselves.

import './setup-dom.js';

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { createRequire } from 'node:module';

import { SchematicWidget } from '../../src/schematic-widget.js';

// jsdom has no CSS.escape; escaping every non-word character is enough here.
globalThis.CSS = globalThis.CSS
    || { escape: (value) => String(value).replace(/[^\w-]/g, (ch) => `\\${ch}`) };

const require = createRequire(import.meta.url);
const netlistsvg = require('netlistsvg');
const SKIN = readFileSync(new URL('../../src/openroad_skin.svg', import.meta.url), 'utf8');

const INST = 'x"><img src=x onerror=alert(1)>';
const TYPE = 'BUF<X1>';

function netlist() {
    return { modules: { top: {
        ports: {
            'in<0>': { direction: 'input', bits: [2] },
            out: { direction: 'output', bits: [3] },
        },
        cells: { [INST]: {
            type: TYPE,
            port_directions: { 'A"': 'input', Z: 'output' },
            connections: { 'A"': [2], Z: [3] },
        } },
        netnames: { 'n&1': { bits: [2] } },
    } } };
}

function makeWidget() {
    const element = document.createElement('div');
    document.body.appendChild(element);
    const widget = new SchematicWidget({ element }, {});
    widget.netlistsvg = netlistsvg;
    widget.skin = SKIN;
    widget._netlistsvgReady = true;
    return widget;
}

describe('schematic names', () => {
    it('renders names with markup characters as text', async () => {
        const widget = makeWidget();
        assert.equal(await widget.renderNetlist(netlist()), true);

        const svg = widget.svgContainer;
        assert.equal(svg.querySelector('img'), null, 'a name injected an element');
        for (const el of svg.querySelectorAll('*')) {
            for (const attr of el.attributes) {
                assert.ok(!attr.name.startsWith('on'),
                          `a name injected the attribute ${attr.name}`);
            }
        }

        const group = document.getElementById(`cell_${INST}`);
        assert.ok(group, 'the cell group does not carry the instance name');
        assert.equal(widget._svgIdToInstName.get(group.id), INST);
        assert.ok(svg.textContent.includes(TYPE), 'the cell type is not shown');
    });
});
