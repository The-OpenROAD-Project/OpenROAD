// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { SchematicWidget } from '../../src/schematic-widget.js';

globalThis.CSS = globalThis.CSS || { escape: (value) => String(value) };
globalThis.requestAnimationFrame = globalThis.requestAnimationFrame
    || ((callback) => callback());

const svgNS = 'http://www.w3.org/2000/svg';

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

function addPortMarker(group, port, x, y) {
    const marker = document.createElementNS(svgNS, 'g');
    marker.setAttribute('s:pid', port);
    marker.setAttribute('s:x', String(x));
    marker.setAttribute('s:y', String(y));
    marker.setAttribute('transform', `translate(${x}, ${y})`);
    group.appendChild(marker);
    return marker;
}

function addPortLabel(group, port, x, y, side, labelX = null) {
    const marker = addPortMarker(group, port, x, y);
    const label = document.createElementNS(svgNS, 'text');
    label.setAttribute('data-openroad-port', port);
    label.setAttribute('data-openroad-port-side', side);
    label.setAttribute('x', String(labelX ?? (side === 'left' ? -3 : 4)));
    label.setAttribute('y', '-4');
    marker.appendChild(label);
    return label;
}

function makeSchematic(widget, addWire) {
    const svg = document.createElementNS(svgNS, 'svg');
    const group = document.createElementNS(svgNS, 'g');
    const symbolPath = document.createElementNS(svgNS, 'path');
    const label = document.createElementNS(svgNS, 'text');

    group.id = 'cell_u1';
    group.getBBox = () => ({ x: 0, y: 0, width: 30, height: 30 });
    group.getBoundingClientRect = () => rect(0, 0, 30, 30);

    symbolPath.getBoundingClientRect = () => rect(0, -20, 30, -12);
    group.appendChild(symbolPath);

    label.setAttribute('data-openroad-label', 'instance');
    label.setAttribute('data-openroad-label-line-count', '1');
    label.getBoundingClientRect = () => {
        const x = parseFloat(label.getAttribute('x') || '0');
        const y = parseFloat(label.getAttribute('y') || '0');
        const width = 20;
        const anchor = label.style.textAnchor || 'middle';
        const left = anchor === 'middle'
            ? x - width / 2
            : (anchor === 'end' ? x - width : x);
        return rect(left, y - 6, left + width, y - 2);
    };
    group.appendChild(label);
    svg.appendChild(group);

    if (addWire) {
        const wire = document.createElementNS(svgNS, 'path');
        wire.getBoundingClientRect = () => rect(0, -20, 30, -12);
        svg.appendChild(wire);
    }

    widget.svgContainer.replaceChildren(svg);
    widget._svgEl = svg;

    return { label };
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

    it('remaps OpenROAD symbol connections when merging an expanded cone', () => {
        const { widget, container } = makeWidget();
        const currentNetlist = {
            modules: {
                top: {
                    cells: {
                        u1: {
                            type: 'existing',
                            connections: { A: [2], Y: [3] },
                        },
                    },
                    netnames: {
                        shared: { hide_name: 0, bits: [2], attributes: {} },
                        used1: { hide_name: 0, bits: [3], attributes: {} },
                        used2: { hide_name: 0, bits: [4], attributes: {} },
                    },
                },
            },
        };
        const expandedCone = {
            modules: {
                top: {
                    cells: {
                        dff1: {
                            type: 'DFF_X1',
                            connections: { D: [2], CK: [3], Q: [4], QN: [5] },
                            attributes: {
                                openroad_symbol_type: 'openroad_dff',
                                openroad_symbol_connections: {
                                    D: [2],
                                    CK: [3],
                                    Q: [4],
                                    QN: [5],
                                },
                            },
                        },
                    },
                    netnames: {
                        shared: { hide_name: 0, bits: [2], attributes: {} },
                        clk: { hide_name: 0, bits: [3], attributes: {} },
                        q: { hide_name: 0, bits: [4], attributes: {} },
                        qn: { hide_name: 0, bits: [5], attributes: {} },
                    },
                },
            },
        };

        const merged = widget._mergeSchematicNetlists(currentNetlist, expandedCone);
        const mergedDff = merged.modules.top.cells.dff1;

        assert.deepEqual(mergedDff.connections, { D: [2], CK: [5], Q: [6], QN: [7] });
        assert.deepEqual(mergedDff.attributes.openroad_symbol_connections, {
            D: [2],
            CK: [5],
            Q: [6],
            QN: [7],
        });
        assert.deepEqual(
            expandedCone.modules.top.cells.dff1.attributes.openroad_symbol_connections,
            { D: [2], CK: [3], Q: [4], QN: [5] });
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
        assert.deepEqual(requests[0], {
            type: 'schematic_inspect',
            inst_name: 'input24',
        });
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

describe('SchematicWidget label placement', () => {
    it('lowers the three-input NAND input port labels', () => {
        const { widget, container } = makeWidget();
        const skin = widget._addOpenRoadSkinSymbols(
            '<svg xmlns:s="https://github.com/nturley/netlistsvg"></svg>');
        const nand3Start = skin.indexOf('s:type="openroad_nand3"');
        const nand3End = skin.indexOf('s:type="openroad_or3"', nand3Start);
        const nand3 = skin.slice(nand3Start, nand3End);

        assert.match(nand3, /x="-3" y="-2" data-openroad-port="A1"/);
        assert.match(nand3, /x="-3" y="-2" data-openroad-port="A2"/);
        assert.match(nand3, /x="-3" y="-2" data-openroad-port="A3"/);
        container.element.remove();
    });

    it('uses original library port names on OpenROAD symbol pin labels', () => {
        const { widget, container } = makeWidget();
        const group = document.createElementNS(svgNS, 'g');
        const input1 = document.createElementNS(svgNS, 'text');
        const input2 = document.createElementNS(svgNS, 'text');
        const output = document.createElementNS(svgNS, 'text');

        input1.setAttribute('data-openroad-port', 'A1');
        input2.setAttribute('data-openroad-port', 'A2');
        output.setAttribute('data-openroad-port', 'Y');
        group.append(input1, input2, output);

        widget._updateOpenRoadPortLabels(group, {
            attributes: {
                openroad_input_ports: ['A', 'B'],
                openroad_output_port: 'ZN',
            },
        });

        assert.equal(input1.textContent, 'A');
        assert.equal(input2.textContent, 'B');
        assert.equal(output.textContent, 'ZN');
        assert.equal(output.style.fontSize, '10px');
        assert.equal(output.style.fontWeight, 'bold');
        assert.ok(output.style.fontFamily.includes('Courier New'));
        container.element.remove();
    });

    it('places OpenROAD symbol pin labels from NetlistSVG pin markers', () => {
        const { widget, container } = makeWidget();
        const group = document.createElementNS(svgNS, 'g');

        group.getBBox = () => ({ x: 0, y: 0, width: 30, height: 20 });
        const input = addPortLabel(group, 'A', 0, 10, 'left');
        const output = addPortLabel(group, 'Y', 30, 10, 'right', 0);

        widget._updateOpenRoadPortLabels(group, {
            attributes: {
                openroad_input_port: 'IN',
                openroad_output_port: 'ZN',
            },
        });

        assert.equal(input.textContent, 'IN');
        assert.equal(input.getAttribute('x'), '-3');
        assert.equal(input.getAttribute('y'), '-4');
        assert.equal(input.style.textAnchor, 'end');
        assert.equal(output.textContent, 'ZN');
        assert.equal(output.getAttribute('x'), '0');
        assert.equal(output.getAttribute('y'), '-4');
        assert.equal(output.style.textAnchor, 'start');
        container.element.remove();
    });

    it('preserves generic box input label anchoring from NetlistSVG classes', () => {
        const { widget, container } = makeWidget();
        const input = document.createElementNS(svgNS, 'text');
        const nodeLabel = document.createElementNS(svgNS, 'text');

        input.setAttribute('class', 'inputPortLabel cell_u1');
        nodeLabel.setAttribute('class', 'nodelabel cell_u1');

        widget._stylePortLabel(input);
        widget._stylePortLabel(nodeLabel);

        assert.equal(input.style.textAnchor, 'end');
        assert.equal(input.getAttribute('text-anchor'), 'end');
        assert.equal(nodeLabel.style.textAnchor, 'middle');
        assert.equal(nodeLabel.getAttribute('text-anchor'), 'middle');
        container.element.remove();
    });

    it('preserves custom template offsets for bubble output labels', () => {
        const { widget, container } = makeWidget();
        const group = document.createElementNS(svgNS, 'g');

        group.getBBox = () => ({ x: 0, y: 0, width: 34, height: 20 });
        const output = addPortLabel(group, 'Y', 34, 10, 'right', 0);

        widget._updateOpenRoadPortLabels(group, {
            attributes: {
                openroad_output_port: 'ZN',
            },
        });

        assert.equal(output.textContent, 'ZN');
        assert.equal(output.getAttribute('x'), '0');
        assert.equal(output.getAttribute('y'), '-4');
        assert.equal(output.style.textAnchor, 'start');
        container.element.remove();
    });

    it('keeps template port side when measured bounds include wires', () => {
        const { widget, container } = makeWidget();
        const group = document.createElementNS(svgNS, 'g');

        group.getBBox = () => ({ x: -100, y: 0, width: 130, height: 20 });
        const input = addPortLabel(group, 'A', 0, 10, 'left');

        widget._updateOpenRoadPortLabels(group, {
            attributes: {
                openroad_input_port: 'A',
            },
        });

        assert.equal(input.getAttribute('x'), '-3');
        assert.equal(input.getAttribute('y'), '-4');
        assert.equal(input.style.textAnchor, 'end');
        container.element.remove();
    });

    it('moves OpenROAD symbol pin labels away from overlapping shapes', () => {
        const { widget, container } = makeWidget();
        const group = document.createElementNS(svgNS, 'g');
        const output = document.createElementNS(svgNS, 'text');

        group.getBBox = () => ({ x: 0, y: 0, width: 10, height: 10 });
        output.setAttribute('data-openroad-port', 'Y');
        output.setAttribute('data-openroad-port-side', 'right');
        output.setAttribute('x', '5');
        output.setAttribute('y', '5');
        group.append(output);

        widget._updateOpenRoadPortLabels(group, {
            attributes: {
                openroad_output_port: 'ZN',
            },
        });

        const bbox = widget._textBBox(output);
        assert.equal(output.textContent, 'ZN');
        assert.equal(output.style.fontSize, '10px');
        assert.equal(output.style.fontWeight, 'bold');
        assert.ok(bbox.x >= 12);
        container.element.remove();
    });

    it('keeps the instance label above the symbol when no wire blocks it', () => {
        const { widget, container } = makeWidget();
        const { label } = makeSchematic(widget, false);

        widget._layoutInstanceLabels(netlist);

        assert.equal(label.getAttribute('x'), '15');
        assert.equal(label.getAttribute('y'), '-14');
        assert.equal(label.style.textAnchor, 'middle');
        container.element.remove();
    });

    it('moves the top-priority label when a wire crosses that slot', () => {
        const { widget, container } = makeWidget();
        const { label } = makeSchematic(widget, true);

        widget._layoutInstanceLabels(netlist);

        assert.equal(label.getAttribute('x'), '15');
        assert.equal(label.getAttribute('y'), '44');
        assert.equal(label.style.textAnchor, 'middle');
        container.element.remove();
    });
});
