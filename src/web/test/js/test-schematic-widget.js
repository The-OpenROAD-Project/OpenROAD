// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import {
    SchematicWidget, canonicalizeCell, canonicalizeForSkin, scopeCssSelector,
} from '../../src/schematic-widget.js';

globalThis.CSS = globalThis.CSS || { escape: (value) => String(value) };
globalThis.requestAnimationFrame = globalThis.requestAnimationFrame
    || ((callback) => callback());

const svgNS = 'http://www.w3.org/2000/svg';

// Helper: a server-style cell object.
function cell(extra) {
    return Object.assign({
        hide_name: 0,
        attributes: { ref: 'g1' },
        parameters: {},
    }, extra);
}

function rect(left, top, right, bottom) {
    return {
        left,
        top,
        right,
        bottom,
        width: right - left,
        height: bottom - top,
    };
}

function makeWidget(appState = {}) {
    const container = { element: document.createElement('div') };
    document.body.appendChild(container.element);
    const widget = new SchematicWidget(container, appState);
    return { widget, container };
}

function makeInteractiveCell(widget, instName) {
    const svg = document.createElementNS(svgNS, 'svg');
    const group = document.createElementNS(svgNS, 'g');
    const path = document.createElementNS(svgNS, 'path');

    group.id = `cell_${instName}`;
    group.getBBox = () => ({ x: 0, y: 0, width: 30, height: 30 });
    group.getBoundingClientRect = () => rect(0, 0, 30, 30);
    group.appendChild(path);
    svg.appendChild(group);

    widget.svgContainer.replaceChildren(svg);
    widget._svgEl = svg;
    widget._svgIdToInstName.set(group.id, instName);

    return { group, path };
}

function makeClassOnlyInteractiveCell(widget, instName) {
    const svg = document.createElementNS(svgNS, 'svg');
    const group = document.createElementNS(svgNS, 'g');
    const path = document.createElementNS(svgNS, 'path');

    group.getBBox = () => ({ x: 0, y: 0, width: 30, height: 30 });
    group.getBoundingClientRect = () => rect(0, 0, 30, 30);
    path.setAttribute('class', `cell_${instName}`);
    group.appendChild(path);
    svg.appendChild(group);

    widget.svgContainer.replaceChildren(svg);
    widget._svgEl = svg;
    widget._currentNetlist = {
        modules: {
            top: {
                cells: {
                    [instName]: {},
                },
            },
        },
    };

    return { group, path };
}

function makeCoordinateMappedCell(widget, instName) {
    const svg = document.createElementNS(svgNS, 'svg');
    const group = document.createElementNS(svgNS, 'g');
    const path = document.createElementNS(svgNS, 'path');

    group.id = `cell_${instName}`;
    group.getBBox = () => ({ x: 0, y: 0, width: 30, height: 30 });
    group.getBoundingClientRect = () => rect(10, 10, 40, 40);
    path.getBoundingClientRect = () => rect(10, 10, 40, 40);
    group.appendChild(path);
    svg.appendChild(group);

    widget.svgContainer.replaceChildren(svg);
    widget._svgEl = svg;
    widget._registerSvgCellHitRecord(instName, group);

    return { svg, group, path };
}

function doubleClickEvent(target) {
    return {
        target,
        defaultPrevented: false,
        propagationStopped: false,
        preventDefault() {
            this.defaultPrevented = true;
        },
        stopPropagation() {
            this.propagationStopped = true;
        },
    };
}

const netlist = {
    modules: {
        top: {
            cells: {
                u1: {},
            },
        },
    },
};

describe('SchematicWidget schematic navigation', () => {
    it('expands the current schematic from a double-clicked cell shape', async () => {
        const requests = [];
        const currentNetlist = {
            modules: {
                top: {
                    attributes: {},
                    ports: {
                        shared_port: { direction: 'input', bits: [5] },
                    },
                    cells: {
                        u1: {
                            type: 'BUF_X1',
                            connections: { Z: [5] },
                        },
                    },
                    netnames: {
                        shared: { hide_name: 0, bits: [5], attributes: {} },
                    },
                },
            },
        };
        const expandedCone = {
            modules: {
                top: {
                    attributes: {},
                    ports: {
                        added_port: { direction: 'output', bits: [3] },
                    },
                    cells: {
                        u2: {
                            type: 'BUF_X1',
                            connections: { A: [2], Z: [3] },
                        },
                    },
                    netnames: {
                        shared: { hide_name: 0, bits: [2], attributes: {} },
                        added: { hide_name: 0, bits: [3], attributes: {} },
                    },
                },
            },
        };
        const appState = {
            showDbu: false,
            websocketManager: {
                readyPromise: Promise.resolve(),
                request: (request) => {
                    requests.push(request);
                    if (request.type === 'schematic_cone') {
                        return Promise.resolve(expandedCone);
                    }
                    return Promise.resolve({ selected: [{ name: request.inst_name }] });
                },
            },
            updateInspector() {},
            focusComponent() {},
            redrawAllLayers() {},
        };
        const { widget, container } = makeWidget(appState);
        const { path } = makeInteractiveCell(widget, 'u2');
        const event = doubleClickEvent(path);
        const rendered = [];

        widget._netlistsvgReady = true;
        widget._currentNetlist = currentNetlist;
        widget.renderNetlist = async (data) => {
            rendered.push(data);
        };
        widget.controls.querySelector('#schematic-fanin-depth').value = '2';
        widget.controls.querySelector('#schematic-fanout-depth').value = '3';

        const didExpand = await widget._handleCellDoubleClick(event);

        assert.equal(didExpand, true);
        assert.equal(event.defaultPrevented, true);
        assert.equal(event.propagationStopped, true);
        assert.equal(appState.selectedInstanceName, 'u2');
        assert.deepEqual(requests[0], {
            type: 'schematic_inspect',
            inst_name: 'u2',
            use_dbu: false,
        });
        assert.deepEqual(requests[1], {
            type: 'schematic_cone',
            inst_name: 'u2',
            fanin_depth: 2,
            fanout_depth: 3,
        });
        assert.deepEqual(rendered, [{
            modules: {
                top: {
                    attributes: {},
                    ports: {
                        shared_port: { direction: 'input', bits: [5] },
                        added_port: { direction: 'output', bits: [6] },
                    },
                    cells: {
                        u1: {
                            type: 'BUF_X1',
                            connections: { Z: [5] },
                        },
                        u2: {
                            type: 'BUF_X1',
                            connections: { A: [5], Z: [6] },
                        },
                    },
                    netnames: {
                        shared: { hide_name: 0, bits: [5], attributes: {} },
                        added: { hide_name: 0, bits: [6], attributes: {} },
                    },
                },
            },
        }]);
        assert.deepEqual(expandedCone.modules.top.cells.u2.connections, { A: [2], Z: [3] });
        container.element.remove();
    });

    it('keeps existing cells when an expanded cone overlaps the current schematic', () => {
        const { widget, container } = makeWidget();
        const currentNetlist = {
            modules: {
                top: {
                    cells: {
                        u1: {
                            type: 'existing',
                            connections: { Z: [2] },
                        },
                    },
                    netnames: {
                        n1: { hide_name: 0, bits: [2], attributes: {} },
                    },
                },
            },
        };
        const expandedCone = {
            modules: {
                top: {
                    cells: {
                        u1: {
                            type: 'new-copy',
                            connections: { Z: [2] },
                        },
                        u2: {
                            type: 'added',
                            connections: { A: [2], Z: [3] },
                        },
                    },
                    netnames: {
                        n1: { hide_name: 0, bits: [2], attributes: {} },
                        n2: { hide_name: 0, bits: [3], attributes: {} },
                    },
                },
            },
        };

        const merged = widget._mergeSchematicNetlists(currentNetlist, expandedCone);

        assert.equal(merged.modules.top.cells.u1.type, 'existing');
        assert.equal(merged.modules.top.cells.u2.type, 'added');
        assert.deepEqual(merged.modules.top.cells.u2.connections, { A: [2], Z: [3] });
        assert.deepEqual(merged.modules.top.netnames.n2.bits, [3]);
        container.element.remove();
    });

    it('expands when the clicked shape only has a cell class', async () => {
        const requests = [];
        const centeredNetlist = {
            modules: {
                top: {
                    cells: {
                        input24: {},
                    },
                },
            },
        };
        const appState = {
            websocketManager: {
                readyPromise: Promise.resolve(),
                request: (request) => {
                    requests.push(request);
                    if (request.type === 'schematic_cone') {
                        return Promise.resolve(centeredNetlist);
                    }
                    return Promise.resolve({ selected: [{ name: request.inst_name }] });
                },
            },
        };
        const { widget, container } = makeWidget(appState);
        const { path } = makeClassOnlyInteractiveCell(widget, 'input24');
        const event = doubleClickEvent(path);

        widget._netlistsvgReady = true;
        widget.renderNetlist = async () => {};

        const didCenter = await widget._handleCellDoubleClick(event);

        assert.equal(didCenter, true);
        assert.equal(appState.selectedInstanceName, 'input24');
        assert.equal(requests[0].type, 'schematic_inspect');
        assert.equal(requests[0].inst_name, 'input24');
        assert.equal(requests[1].type, 'schematic_cone');
        assert.equal(requests[1].inst_name, 'input24');
        container.element.remove();
    });

    it('handles the second mousedown of a double-click on a shape', async () => {
        const requests = [];
        const appState = {
            websocketManager: {
                readyPromise: Promise.resolve(),
                request: (request) => {
                    requests.push(request);
                    if (request.type === 'schematic_cone') {
                        return Promise.resolve(netlist);
                    }
                    return Promise.resolve({ selected: [{ name: request.inst_name }] });
                },
            },
        };
        const { widget, container } = makeWidget(appState);
        const { path } = makeClassOnlyInteractiveCell(widget, 'u1');
        const event = new window.MouseEvent('mousedown', {
            bubbles: true,
            button: 0,
            detail: 2,
        });

        widget._netlistsvgReady = true;
        widget.renderNetlist = async () => {};
        path.dispatchEvent(event);
        await new Promise((resolve) => setTimeout(resolve, 0));

        assert.equal(appState.selectedInstanceName, 'u1');
        assert.equal(requests[0].type, 'schematic_inspect');
        assert.equal(requests[1].type, 'schematic_cone');
        assert.equal(requests[1].inst_name, 'u1');
        container.element.remove();
    });

    it('uses pointer coordinates to find the clicked schematic cell', async () => {
        const requests = [];
        const appState = {
            websocketManager: {
                readyPromise: Promise.resolve(),
                request: (request) => {
                    requests.push(request);
                    if (request.type === 'schematic_cone') {
                        return Promise.resolve(netlist);
                    }
                    return Promise.resolve({ selected: [{ name: request.inst_name }] });
                },
            },
        };
        const { widget, container } = makeWidget(appState);
        const { svg } = makeCoordinateMappedCell(widget, 'u1');
        const event = {
            target: svg,
            clientX: 20,
            clientY: 20,
            timeStamp: 1000,
            preventDefault() {},
            stopPropagation() {},
        };

        widget._netlistsvgReady = true;
        widget.renderNetlist = async () => {};

        const didCenter = await widget._handleCellDoubleClick(event);

        assert.equal(didCenter, true);
        assert.equal(appState.selectedInstanceName, 'u1');
        assert.equal(requests[1].type, 'schematic_cone');
        assert.equal(requests[1].inst_name, 'u1');
        container.element.remove();
    });
});

describe('canonicalizeCell', () => {
    it('maps simple gates to OpenROAD skin types and A1/A2/Y pids', () => {
        const got = canonicalizeCell(cell({
            type: 'NOR2_X1', gate_kind: 'nor',
            port_directions: { A1: 'input', A2: 'input', ZN: 'output' },
            connections: { A1: [2], A2: [3], ZN: [4] },
        }));
        assert.equal(got.type, 'openroad_nor2');
        assert.deepEqual(Object.keys(got.connections), ['A1', 'A2', 'Y']);
        assert.deepEqual(got.connections, { A1: [2], A2: [3], Y: [4] });
        assert.deepEqual(got.port_directions, { A1: 'input', A2: 'input', Y: 'output' });
        // The real pin names are kept (pid -> name) so the symbol can be labelled.
        assert.deepEqual(got.port_labels, { A1: 'A1', A2: 'A2', Y: 'ZN' });
    });

    it('maps an inverter to openroad_inverter with A/Y', () => {
        const got = canonicalizeCell(cell({
            type: 'INV_X1', gate_kind: 'not',
            port_directions: { A: 'input', ZN: 'output' },
            connections: { A: [7], ZN: [8] },
        }));
        assert.equal(got.type, 'openroad_inverter');
        assert.deepEqual(got.connections, { A: [7], Y: [8] });
    });

    it('maps a buffer to openroad_buffer', () => {
        const got = canonicalizeCell(cell({
            type: 'BUF_X1', gate_kind: 'buf',
            port_directions: { A: 'input', Z: 'output' },
            connections: { A: [1], Z: [2] },
        }));
        assert.equal(got.type, 'openroad_buffer');
        assert.deepEqual(got.connections, { A: [1], Y: [2] });
    });

    it('maps a 3-input gate to an OpenROAD per-arity symbol', () => {
        const got = canonicalizeCell(cell({
            type: 'NAND3_X1', gate_kind: 'nand',
            port_directions: { A1: 'input', A2: 'input', A3: 'input', ZN: 'output' },
            connections: { A1: [2], A2: [3], A3: [4], ZN: [5] },
        }));
        assert.equal(got.type, 'openroad_nand3');
        assert.deepEqual(got.connections, { A1: [2], A2: [3], A3: [4], Y: [5] });
        assert.deepEqual(got.port_labels, { A1: 'A1', A2: 'A2', A3: 'A3', Y: 'ZN' });
    });

    it('uses backend gate_ports to preserve Liberty input order', () => {
        const got = canonicalizeCell(cell({
            type: 'NAND2_X1',
            gate_kind: 'nand',
            gate_ports: { A1: 'B', A2: 'A', Y: 'ZN' },
            port_directions: { A: 'input', B: 'input', ZN: 'output' },
            connections: { A: [2], B: [3], ZN: [4] },
        }));

        assert.equal(got.type, 'openroad_nand2');
        assert.deepEqual(got.connections, { A1: [3], A2: [2], Y: [4] });
        assert.deepEqual(got.port_labels, { A1: 'B', A2: 'A', Y: 'ZN' });
    });

    it('maps DFF cells to OpenROAD register symbols using gate_ports', () => {
        const got = canonicalizeCell(cell({
            type: 'DFF_X1',
            gate_kind: 'dff',
            gate_ports: { D: 'D', CK: 'CK', Q: 'Q', QN: 'QN' },
            port_directions: {
                D: 'input',
                CK: 'input',
                Q: 'output',
                QN: 'output',
            },
            connections: { D: [1], CK: [2], Q: [3], QN: [4] },
        }));

        assert.equal(got.type, 'openroad_dff');
        assert.deepEqual(got.connections, { D: [1], CK: [2], Q: [3], QN: [4] });
        assert.deepEqual(got.port_labels, {
            D: 'D',
            CK: 'CK',
            Q: 'Q',
            QN: 'QN',
        });
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
        assert.equal(got.type, 'openroad_nand2');
        assert.deepEqual(Object.keys(got.connections).sort(), ['A1', 'A2', 'Y']);
    });

    it('leaves compound AOI/OAI gates as labelled boxes', () => {
        const aoi = cell({
            type: 'AOI21_X2', gate_kind: 'aoi',
            gate_terms: [['A'], ['B1', 'B2']],
            port_directions: { A: 'input', B1: 'input', B2: 'input', ZN: 'output' },
            connections: { A: [4], B1: [8], B2: [11], ZN: [12] },
        });
        const oai = cell({
            type: 'OAI22_X1', gate_kind: 'oai',
            gate_terms: [['A1', 'A2'], ['B1', 'B2']],
            port_directions: {
                A1: 'input', A2: 'input', B1: 'input', B2: 'input', ZN: 'output',
            },
            connections: { A1: [1], A2: [2], B1: [3], B2: [4], ZN: [5] },
        });
        assert.strictEqual(canonicalizeCell(aoi), aoi);
        assert.strictEqual(canonicalizeCell(oai), oai);
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
        assert.equal(cells._983_.type, 'openroad_nor2');  // gate rewritten
        assert.equal(cells.myff.type, 'DFF_X1');       // non-gate untouched
        // Input is not mutated.
        assert.equal(json.modules.top.cells._983_.type, 'NOR2_X1');
    });

    it('returns the input unchanged when there is no top module', () => {
        const json = { foo: 1 };
        assert.strictEqual(canonicalizeForSkin(json), json);
    });
});

describe('SchematicWidget label placement', () => {
    it('creates OpenROAD port labels from skin pin markers', () => {
        const { widget, container } = makeWidget();
        const svg = document.createElementNS(svgNS, 'svg');
        const group = document.createElementNS(svgNS, 'g');
        group.getBBox = () => ({ x: 0, y: 0, width: 30, height: 30 });

        const inputMarker = document.createElementNS(svgNS, 'g');
        inputMarker.setAttribute('s:pid', 'A1');
        inputMarker.setAttribute('s:x', '0');
        inputMarker.setAttribute('s:y', '7');
        const outputMarker = document.createElementNS(svgNS, 'g');
        outputMarker.setAttribute('s:pid', 'Y');
        outputMarker.setAttribute('s:x', '30');
        outputMarker.setAttribute('s:y', '15');
        group.append(inputMarker, outputMarker);
        svg.appendChild(group);
        widget._svgEl = svg;

        widget._updateOpenRoadPortLabels(group, {
            type: 'openroad_nand2',
            port_labels: { A1: 'A1', Y: 'ZN' },
        });

        const input = group.querySelector('text[data-openroad-port="A1"]');
        const output = group.querySelector('text[data-openroad-port="Y"]');
        assert.equal(input.textContent, 'A1');
        assert.equal(input.style.fontSize, '10px');
        assert.equal(input.style.textAnchor, 'end');
        assert.equal(output.textContent, 'ZN');
        assert.equal(output.style.fontWeight, 'bold');
        container.element.remove();
    });
});

// netlistsvg's skin injects a <style> with unscoped element selectors that
// would otherwise leak document-wide (e.g. `svg { fill:none }` clobbering every
// inline-SVG icon in the app). scopeCssSelector() prefixes each selector with
// the widget container so the skin only styles the schematic.
describe('scopeCssSelector', () => {
    const SCOPE = '.schematic-widget';

    it('prefixes a bare element selector', () => {
        assert.equal(scopeCssSelector('svg', SCOPE), '.schematic-widget svg');
    });

    it('prefixes each selector in a comma list independently', () => {
        assert.equal(
            scopeCssSelector('svg, text, .splitjoinBody', SCOPE),
            '.schematic-widget svg, .schematic-widget text, '
                + '.schematic-widget .splitjoinBody');
    });

    it('trims whitespace around list members', () => {
        assert.equal(
            scopeCssSelector('  svg ,  text  ', SCOPE),
            '.schematic-widget svg, .schematic-widget text');
    });

    it('is idempotent: already-scoped selectors are left untouched', () => {
        const once = scopeCssSelector('svg, text', SCOPE);
        assert.equal(scopeCssSelector(once, SCOPE), once);
    });

    it('does not re-prefix a selector that already starts with the scope', () => {
        assert.equal(
            scopeCssSelector('.schematic-widget svg', SCOPE),
            '.schematic-widget svg');
        // The scope followed by a combinator or class/pseudo is still scoped.
        assert.equal(
            scopeCssSelector('.schematic-widget>svg', SCOPE),
            '.schematic-widget>svg');
        assert.equal(
            scopeCssSelector('.schematic-widget.active', SCOPE),
            '.schematic-widget.active');
        assert.equal(
            scopeCssSelector('.schematic-widget:hover', SCOPE),
            '.schematic-widget:hover');
    });

    it('still scopes a different class that shares the scope as a prefix', () => {
        // `.schematic-widget-foo` is a distinct class, not the scope itself.
        assert.equal(
            scopeCssSelector('.schematic-widget-foo', SCOPE),
            '.schematic-widget .schematic-widget-foo');
    });

    it('still scopes a selector that merely contains the scope word elsewhere',
       () => {
           // The scope only counts as "already applied" at the start, so a
           // descendant reference to it elsewhere must still be prefixed.
           assert.equal(
               scopeCssSelector('div .schematic-widget', SCOPE),
               '.schematic-widget div .schematic-widget');
       });
});
