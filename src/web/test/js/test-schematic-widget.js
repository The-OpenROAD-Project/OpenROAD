// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import {
    SchematicWidget, canonicalizeCell, canonicalizeForSkin, scopeCssSelector,
} from '../../src/schematic-widget.js';
import { beginSelection } from '../../src/ui-utils.js';

globalThis.CSS = globalThis.CSS || { escape: (value) => String(value) };
globalThis.requestAnimationFrame = globalThis.requestAnimationFrame
    || ((callback) => callback());

const svgNS = 'http://www.w3.org/2000/svg';

function cell(extra) {
    return Object.assign({
        hide_name: 0,
        attributes: { ref: 'g1' },
        parameters: {},
    }, extra);
}

// jsdom does not compute SVG layout, so tests stub getBoundingClientRect()
// with this DOMRect-like helper when checking schematic click targets.
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

// Build one cell; identify controls where netlistsvg exposes its id.
function makeCell(widget, instName, { identify = 'id', bounds = rect(0, 0, 30, 30) } = {}) {
    const svg = document.createElementNS(svgNS, 'svg');
    const group = document.createElementNS(svgNS, 'g');
    const path = document.createElementNS(svgNS, 'path');

    if (identify !== 'class') {
        group.id = `cell_${instName}`;
    } else {
        path.setAttribute('class', `cell_${instName}`);
    }
    group.getBBox = () => ({ x: 0, y: 0, width: 30, height: 30 });
    group.getBoundingClientRect = () => bounds;
    path.getBoundingClientRect = () => bounds;
    group.appendChild(path);
    svg.appendChild(group);

    widget.svgContainer.replaceChildren(svg);
    widget._svgEl = svg;
    if (identify === 'id') {
        widget._svgIdToInstName.set(group.id, instName);
    } else {
        widget._registerSvgCellHitTargets([instName]);
    }

    return {
        svg,
        group,
        path,
        hitTarget: group.querySelector('rect[data-openroad-hit-target]'),
    };
}

const makeInteractiveCell = (widget, instName) => makeCell(widget, instName);

const makeClassOnlyInteractiveCell = (widget, instName) =>
    makeCell(widget, instName, { identify: 'class' });

const makeHitTargetCell = (widget, instName) =>
    makeCell(widget, instName, {
        identify: 'registered',
        bounds: rect(10, 10, 40, 40),
    });

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

describe('SchematicWidget controls', () => {
    it('uses standard symbols without offering a box-view selector', () => {
        const { widget, container } = makeWidget();

        assert.equal(widget.controls.querySelector('#schematic-view-style'), null);
        assert.equal(widget.controls.textContent.includes('Boxes'), false);
        container.element.remove();
    });
});

describe('SchematicWidget SVG content bounds', () => {
    it('measures cell groups without also measuring their child shapes', () => {
        const { widget, container } = makeWidget();
        const svg = document.createElementNS(svgNS, 'svg');
        const cellGroup = document.createElementNS(svgNS, 'g');
        const cellPath = document.createElementNS(svgNS, 'path');
        const cellLabel = document.createElementNS(svgNS, 'text');
        const wireGroup = document.createElementNS(svgNS, 'g');
        const wire = document.createElementNS(svgNS, 'path');
        const topLabel = document.createElementNS(svgNS, 'text');

        cellGroup.id = 'cell_u1';
        cellGroup.append(cellPath, cellLabel);
        wireGroup.appendChild(wire);
        svg.append(cellGroup, wireGroup, topLabel);
        widget.svgContainer.replaceChildren(svg);
        widget._svgEl = svg;

        const elements = Array.from(widget._svgContentElements());
        assert.deepEqual(elements, [cellGroup, wire, topLabel]);
        container.element.remove();
    });

});

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

    it('goes back to the previous schematic after a double-click expansion', async () => {
        const requests = [];
        const currentNetlist = {
            modules: {
                top: {
                    attributes: {},
                    ports: {},
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
                    ports: {},
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
            selectedInstanceName: 'u1',
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
        };
        const { widget, container } = makeWidget(appState);
        const { path } = makeInteractiveCell(widget, 'u2');
        const rendered = [];
        const backButton = widget.controls.querySelector('#schematic-back');

        widget._netlistsvgReady = true;
        widget._currentNetlist = currentNetlist;
        widget.renderNetlist = async (data) => {
            rendered.push(data);
            widget._currentNetlist = data;
            return true;
        };

        assert.equal(backButton.disabled, true);

        const didExpand = await widget._handleCellDoubleClick(doubleClickEvent(path));

        assert.equal(didExpand, true);
        assert.equal(backButton.disabled, false);
        assert.equal(widget._schematicHistory.length, 1);
        assert.equal(appState.selectedInstanceName, 'u2');

        const didGoBack = await widget._goBackSchematic();

        assert.equal(didGoBack, true);
        assert.equal(backButton.disabled, true);
        assert.equal(widget._schematicHistory.length, 0);
        assert.equal(appState.selectedInstanceName, 'u1');
        assert.deepEqual(rendered[1], currentNetlist);
        assert.deepEqual(requests[2], {
            type: 'schematic_inspect',
            inst_name: 'u1',
            use_dbu: false,
        });
        container.element.remove();
    });

    it('does not add history when a double-click expansion returns no cells', async () => {
        const currentNetlist = {
            modules: {
                top: {
                    cells: {
                        u1: {},
                    },
                },
            },
        };
        const emptyCone = {
            modules: {
                top: {
                    cells: {},
                },
            },
        };
        const appState = {
            selectedInstanceName: 'u1',
            websocketManager: {
                readyPromise: Promise.resolve(),
                request: (request) => {
                    if (request.type === 'schematic_cone') {
                        return Promise.resolve(emptyCone);
                    }
                    return Promise.resolve({ selected: [{ name: request.inst_name }] });
                },
            },
        };
        const { widget, container } = makeWidget(appState);
        const { path } = makeInteractiveCell(widget, 'u2');
        const backButton = widget.controls.querySelector('#schematic-back');
        let renderCalls = 0;

        widget._netlistsvgReady = true;
        widget._currentNetlist = currentNetlist;
        widget.renderNetlist = async () => {
            renderCalls += 1;
            return true;
        };

        const didExpand = await widget._handleCellDoubleClick(doubleClickEvent(path));

        assert.equal(didExpand, false);
        assert.equal(renderCalls, 0);
        assert.equal(widget._schematicHistory.length, 0);
        assert.equal(backButton.disabled, true);
        container.element.remove();
    });

    it('drops an expansion when another schematic replaces its base', async () => {
        let releaseCone;
        const appState = {
            websocketManager: {
                readyPromise: Promise.resolve(),
                request: () => new Promise(resolve => { releaseCone = resolve; }),
            },
        };
        const { widget, container } = makeWidget(appState);
        const netlist = name => ({ modules: { top: { cells: { [name]: {} } } } });
        widget._netlistsvgReady = true;
        widget._currentNetlist = netlist('base');
        let renderCalls = 0;
        widget.renderNetlist = async () => { renderCalls++; };

        const pending = widget._expandFromInstance('u2');
        await Promise.resolve();
        widget._currentNetlist = netlist('replacement');
        releaseCone(netlist('u2'));

        assert.equal(await pending, false);
        assert.equal(renderCalls, 0);
        container.element.remove();
    });

    it('clears schematic back history after refresh renders a fresh cone', async () => {
        const requests = [];
        const previousNetlist = {
            modules: {
                top: {
                    cells: {
                        u1: {},
                    },
                },
            },
        };
        const refreshedNetlist = {
            modules: {
                top: {
                    cells: {
                        u3: {},
                    },
                },
            },
        };
        const appState = {
            selectedInstanceName: 'u3',
            websocketManager: {
                readyPromise: Promise.resolve(),
                request: (request) => {
                    requests.push(request);
                    return Promise.resolve(refreshedNetlist);
                },
            },
        };
        const { widget, container } = makeWidget(appState);
        const backButton = widget.controls.querySelector('#schematic-back');

        widget._netlistsvgReady = true;
        widget._currentNetlist = previousNetlist;
        widget._pushSchematicHistory({
            netlist: previousNetlist,
            selectedInstanceName: 'u1',
        });
        widget.renderNetlist = async (data) => {
            widget._currentNetlist = data;
            return true;
        };

        assert.equal(backButton.disabled, false);

        const didRefresh = await widget.refresh();

        assert.equal(didRefresh, true);
        assert.equal(backButton.disabled, true);
        assert.equal(widget._schematicHistory.length, 0);
        assert.deepEqual(requests[0], {
            type: 'schematic_cone',
            inst_name: 'u3',
            fanin_depth: 1,
            fanout_depth: 1,
        });
        container.element.remove();
    });

    it('drops an inspect response that another panel has superseded', async () => {
        let releaseInspect;
        const inspected = [];
        const appState = {
            showDbu: false,
            websocketManager: {
                request: () => new Promise((resolve) => {
                    releaseInspect = resolve;
                }),
            },
            updateInspector: (data) => inspected.push(data),
            focusComponent() {},
        };
        const { widget, container } = makeWidget(appState);

        const pending = widget._fetchInspect('u1');
        // Another panel takes the selection while the request is in flight.
        beginSelection(appState);
        releaseInspect({ selected: [{ name: 'u1' }] });

        assert.equal(await pending, false);
        assert.deepEqual(inspected, []);
        container.element.remove();
    });

    it('applies an inspect response that still owns the selection', async () => {
        const inspected = [];
        const focused = [];
        const appState = {
            showDbu: false,
            websocketManager: {
                request: () => Promise.resolve({ selected: [{ name: 'u1' }] }),
            },
            updateInspector: (data) => inspected.push(data),
            focusComponent: (name) => focused.push(name),
        };
        const { widget, container } = makeWidget(appState);

        assert.equal(await widget._fetchInspect('u1'), true);
        assert.deepEqual(inspected, [{ selected: [{ name: 'u1' }] }]);
        assert.deepEqual(focused, ['Inspector']);
        container.element.remove();
    });

    it('keeps existing cells when an expanded cone overlaps the current schematic', () => {
        const { widget, container } = makeWidget();
        const currentNetlist = {
            modules: {
                top: {
                    attributes: { source_line: 9000 },
                    ports: {
                        in: { direction: 'input', bits: [2], offset: 100 },
                    },
                    cells: {
                        u1: {
                            type: 'existing',
                            parameters: { WIDTH: 512 },
                            connections: { Z: [2], D: [7] },
                        },
                    },
                    netnames: {
                        n1: { hide_name: 0, bits: [2], attributes: { width: 1024 } },
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
                            connections: { A: [2], Z: [3], X: [7] },
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
        const n2Bit = merged.modules.top.netnames.n2.bits[0];
        assert.deepEqual(merged.modules.top.cells.u2.connections.A, [2]);
        assert.deepEqual(merged.modules.top.cells.u2.connections.Z, [n2Bit]);
        assert.notEqual(merged.modules.top.cells.u2.connections.X[0], 7);
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

    it('uses transparent hit targets to find the clicked schematic cell', async () => {
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
        const { hitTarget } = makeHitTargetCell(widget, 'u1');
        const event = {
            target: hitTarget,
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

    it('restores the current selection when Back rendering fails', async () => {
        const appState = { selectedInstanceName: 'current' };
        const { widget, container } = makeWidget(appState);
        widget._pushSchematicHistory({
            netlist,
            selectedInstanceName: 'previous',
        });
        widget.renderNetlist = async () => false;

        assert.equal(await widget._goBackSchematic(), false);

        assert.equal(appState.selectedInstanceName, 'current');
        assert.equal(widget._schematicHistory.length, 1);
        assert.equal(widget._schematicHistory[0].selectedInstanceName, 'previous');
        container.element.remove();
    });
});

describe('SchematicWidget render ordering', () => {
    function gateNetlist() {
        return { modules: { top: { cells: {
            u1: cell({
                type: 'BUF_X1',
                gate_kind: 'buf',
                port_directions: { A: 'input', Z: 'output' },
                connections: { A: [1], Z: [2] },
            }),
        } } } };
    }

    it('keeps the newest schematic when layouts finish out of order', async () => {
        const { widget, container } = makeWidget();
        const pending = [];
        let renderCount = 0;
        widget.skin = '<svg></svg>';
        widget.netlistsvg = {
            render(_skin, json) {
                assert.equal(json.modules.top.cells.u1.type, '$_BUF_');
                renderCount += 1;
                let resolve;
                const promise = new Promise(resolvePromise => {
                    resolve = resolvePromise;
                });
                pending.push({ render: renderCount, resolve });
                return promise;
            },
        };
        const source = gateNetlist();

        const first = widget.renderNetlist(source);
        const second = widget.renderNetlist(source);

        assert.deepEqual(pending.map(render => render.render), [1, 2]);
        pending[1].resolve('<svg data-render="2"></svg>');
        assert.equal(await second, true);
        pending[0].resolve('<svg data-render="1"></svg>');
        assert.equal(await first, false);
        assert.equal(widget._svgEl.getAttribute('data-render'), '2');
        container.element.remove();
    });

    it('drops deferred layout work from a superseded render', async () => {
        const originalRequestAnimationFrame = globalThis.requestAnimationFrame;
        const frames = [];
        globalThis.requestAnimationFrame = callback => frames.push(callback);
        try {
            const { widget, container } = makeWidget();
            const fitted = [];
            let renderCount = 0;
            widget.skin = '<svg></svg>';
            widget.netlistsvg = {
                render() {
                    renderCount += 1;
                    return Promise.resolve(
                        `<svg data-render="${renderCount}"></svg>`);
                },
            };
            widget.fitView = () => {
                fitted.push(widget._svgEl.getAttribute('data-render'));
            };

            assert.equal(await widget.renderNetlist(gateNetlist()), true);
            frames.shift()();
            assert.equal(frames.length, 1);

            assert.equal(await widget.renderNetlist(gateNetlist()), true);
            while (frames.length > 0) {
                frames.shift()();
            }

            assert.deepEqual(fitted, ['2']);
            container.element.remove();
        } finally {
            globalThis.requestAnimationFrame = originalRequestAnimationFrame;
        }
    });
});

describe('SchematicWidget cell hit target layout', () => {
    for (const failedIndex of [null, 1]) {
        const description = failedIndex === null
            ? 'batches body measurements before inserting hit targets'
            : 'restores all text and skips only a cell whose measurement fails';
        it(description, async () => {
            const { widget, container } = makeWidget();
            const originalRequestAnimationFrame = globalThis.requestAnimationFrame;
            globalThis.requestAnimationFrame = () => {};
            try {
                const cellCount = 20;
                const cells = Object.fromEntries(Array.from(
                    { length: cellCount }, (_, index) => [`u${index}`, {}]));
                widget.skin = '<svg></svg>';
                widget.netlistsvg = {
                    render: () => Promise.resolve(`<svg>${Object.keys(cells).map(
                        name => `<g id="cell_${name}"><path/><text>${name}</text>`
                            + '<text>port</text></g>').join('')}</svg>`),
                };

                const events = [];
                const displays = [];
                const labels = [];
                const groups = [];
                const measuredHidden = [];
                let layoutDirty = true;
                let layoutFlushes = 0;
                const ensureLabels = widget._ensureOpenRoadSymbolLabels.bind(widget);
                widget._ensureOpenRoadSymbolLabels = json => {
                    ensureLabels(json);
                    groups.push(...widget._svgEl.querySelectorAll('g'));
                    labels.push(...widget._svgEl.querySelectorAll('text'));
                    labels.forEach((label, index) => {
                        const display = [null, 'inline', 'none'][index % 3];
                        displays.push(display);
                        if (display !== null) label.setAttribute('display', display);
                        const setAttribute = label.setAttribute.bind(label);
                        const removeAttribute = label.removeAttribute.bind(label);
                        label.setAttribute = (name, value) => {
                            if (name === 'display') {
                                events.push('write');
                                layoutDirty = true;
                            }
                            setAttribute(name, value);
                        };
                        label.removeAttribute = name => {
                            if (name === 'display') {
                                events.push('write');
                                layoutDirty = true;
                            }
                            removeAttribute(name);
                        };
                    });
                    groups.forEach((group, index) => {
                        group.getBBox = () => {
                            events.push('measure');
                            if (layoutDirty) layoutFlushes += 1;
                            layoutDirty = false;
                            measuredHidden.push(labels.every(
                                label => label.getAttribute('display') === 'none'));
                            if (index === failedIndex) throw new Error('measurement failed');
                            return { x: index, y: 2, width: 30, height: 40 };
                        };
                        const insertBefore = group.insertBefore.bind(group);
                        group.insertBefore = (element, before) => {
                            events.push('insert');
                            layoutDirty = true;
                            return insertBefore(element, before);
                        };
                    });
                };

                assert.equal(await widget.renderNetlist({ modules: { top: { cells } } }), true);

                const targetCount = cellCount - (failedIndex === null ? 0 : 1);
                assert.deepEqual(events, [
                    ...Array(labels.length).fill('write'),
                    ...Array(cellCount).fill('measure'),
                    ...Array(labels.length).fill('write'),
                    ...Array(targetCount).fill('insert'),
                ]);
                assert.equal(layoutFlushes, 1);
                assert.equal(measuredHidden.every(Boolean), true);
                assert.deepEqual(labels.map(label => label.getAttribute('display')), displays);
                groups.forEach((group, index) => {
                    const target = group.querySelector('rect[data-openroad-hit-target]');
                    assert.equal(Boolean(target), index !== failedIndex);
                    if (target) {
                        assert.equal(group.firstChild, target);
                        assert.equal(target.getAttribute('x'), String(index));
                        assert.equal(target.getAttribute('y'), '2');
                        assert.equal(target.getAttribute('width'), '30');
                        assert.equal(target.getAttribute('height'), '40');
                        assert.equal(target.getAttribute('class'), `cell_u${index}`);
                    }
                    assert.equal(widget._svgIdToInstName.get(`cell_u${index}`), `u${index}`);
                    assert.equal(widget._svgIdToInstName.get(`u${index}`), `u${index}`);
                });
            } finally {
                globalThis.requestAnimationFrame = originalRequestAnimationFrame;
                container.element.remove();
            }
        });
    }
});

describe('SchematicWidget timing path overlay', () => {
    // A timing_report data_nodes entry; `inst` is what the overlay joins on.
    function node(inst, extra = {}) {
        return { pin: `${inst}/A`, inst, clk: false, ...extra };
    }

    function timingPath(nodes, slack = -0.25) {
        return { slack, data_nodes: nodes, capture_nodes: [] };
    }

    // Put several named cells into one SVG so path ordering can be checked.
    function makeCells(widget, instNames) {
        const svg = document.createElementNS(svgNS, 'svg');
        const groups = {};
        for (const instName of instNames) {
            const group = document.createElementNS(svgNS, 'g');
            group.id = `cell_${instName}`;
            group.getBBox = () => ({ x: 0, y: 0, width: 30, height: 30 });
            const path = document.createElementNS(svgNS, 'path');
            path.setAttribute('class', `cell_${instName}`);
            group.appendChild(path);
            svg.appendChild(group);
            groups[instName] = group;
        }
        widget.svgContainer.replaceChildren(svg);
        widget._svgEl = svg;
        return { svg, groups };
    }

    function overlayIn(group) {
        return group.querySelectorAll('.schematic-timing-node');
    }

    // CSS colors can serialize as hex or rgb across jsdom versions.
    function assertColor(actual, expected) {
        const normalize = (color) => {
            const style = document.createElement('span').style;
            style.color = color;
            return style.color;
        };
        assert.equal(normalize(actual), normalize(expected));
    }

    it('colors each path cell by styling the actual SVG shape', () => {
        const { widget, container } = makeWidget();
        const { groups } = makeCells(widget, ['u1', 'u2']);

        widget.showTimingPath(timingPath([node('u1'), node('u2')]));

        assert.equal(overlayIn(groups.u1).length, 1);
        assert.equal(overlayIn(groups.u2).length, 1);
        assertColor(groups.u1.querySelector('path').style.stroke, '#ff0000');
        assertColor(groups.u2.querySelector('path').style.stroke, '#ff0000');
        assert.equal(groups.u1.querySelector('text'), null);
        container.element.remove();
    });

    function legendOf(widget) {
        return widget.controls.querySelector('#schematic-timing-legend');
    }

    it('uses exact layout colors for launch clock and data cells', () => {
        const { widget, container } = makeWidget();
        const { groups } = makeCells(widget, ['ff1', 'u1']);

        widget.showTimingPath(timingPath([
            node('ff1', { clk: true }),
            node('u1'),
        ]));

        assertColor(groups.ff1.querySelector('path').style.stroke, '#00ffff');
        assertColor(groups.u1.querySelector('path').style.stroke, '#ff0000');
        container.element.remove();
    });

    it('shows the timing color key once cells are colored', () => {
        const { widget, container } = makeWidget();
        const { groups } = makeCells(widget, ['u1']);

        widget.showTimingPath(timingPath([node('u1')]));

        const legend = legendOf(widget);
        assert.equal(legend.hidden, false);
        assert.match(legend.textContent, /launch/);
        assert.match(legend.textContent, /data/);
        assert.match(legend.textContent, /capture/);
        const drawn = groups.u1.querySelector('path');
        const swatch = legend.querySelectorAll('line')[1];
        assertColor(swatch.getAttribute('stroke'), drawn.style.stroke);
        container.element.remove();
    });

    it('treats a cell on both the clock and data legs as a data cell', () => {
        const { widget, container } = makeWidget();
        const { groups } = makeCells(widget, ['ff1', 'u1']);

        widget.showTimingPath(timingPath([
            { pin: 'in1', clk: false },   // block port: no instance, skipped
            node('ff1', { clk: true }),
            node('ff1'),
            node('u1'),
        ]));

        assert.equal(groups.ff1.querySelector('path').style.stroke,
                     groups.u1.querySelector('path').style.stroke);
        container.element.remove();
    });

    it('restores colored SVG shapes when passed null', () => {
        const { widget, container } = makeWidget();
        const { groups } = makeCells(widget, ['u1']);
        const shape = groups.u1.querySelector('path');
        widget._currentNetlist = netlistWith('u1');
        shape.style.stroke = '#123456';
        shape.style.strokeWidth = '1';

        widget.showTimingPath(timingPath([node('u1')]));
        assertColor(shape.style.stroke, '#ff0000');
        assert.equal(shape.style.strokeWidth, '1');

        widget.showTimingPath(null);
        assert.equal(overlayIn(groups.u1).length, 0);
        assertColor(shape.style.stroke, '#123456');
        assert.equal(shape.style.strokeWidth, '1');
        assert.equal(legendOf(widget).hidden, true);
        assert.equal(widget.controls.querySelector('#schematic-status').textContent,
                     '1 cell');
        container.element.remove();
    });

    it('replaces the previous overlay instead of stacking onto it', () => {
        const { widget, container } = makeWidget();
        const { groups } = makeCells(widget, ['u1']);

        widget.showTimingPath(timingPath([node('u1')]));
        widget.showTimingPath(timingPath([node('u1')]));

        assert.equal(overlayIn(groups.u1).length, 1);
        container.element.remove();
    });

    // Dark mode inverts the whole SVG at the root unless this class is set,
    // so it has to track whether any shape is being held out of the inversion.
    it('marks the container only while overlay shapes are in the SVG', () => {
        const { widget, container } = makeWidget();
        const { groups } = makeCells(widget, ['u1']);
        const marked = () =>
            widget.svgContainer.classList.contains('schematic-timing-active');

        assert.equal(marked(), false);

        widget.showTimingPath(timingPath([node('u1')]));
        assert.equal(overlayIn(groups.u1).length, 1);
        assert.equal(marked(), true);

        widget.showTimingPath(null);
        assert.equal(marked(), false);
        container.element.remove();
    });

    it('leaves the container unmarked when no path cell is on screen', () => {
        const { widget, container } = makeWidget();
        makeCells(widget, ['other']);

        widget.showTimingPath(timingPath([node('u1')]));

        assert.equal(
            widget.svgContainer.classList.contains('schematic-timing-active'),
            false);
        container.element.remove();
    });

    it('reports when the path has no cells in the current schematic', () => {
        const { widget, container } = makeWidget();
        makeCells(widget, ['other']);

        widget.showTimingPath(timingPath([node('u1'), node('u2')]));

        const status = widget.controls.querySelector('#schematic-status');
        assert.match(status.textContent, /none of its 2 cells/);
        assert.equal(legendOf(widget).hidden, true);
        container.element.remove();
    });

    it('reports how many of the path cells were highlighted', () => {
        const { widget, container } = makeWidget();
        makeCells(widget, ['u1']);

        widget.showTimingPath(timingPath([node('u1'), node('u2')]));

        const status = widget.controls.querySelector('#schematic-status');
        assert.match(status.textContent, /1 of 2 cells/);
        container.element.remove();
    });

    it('colors schematic wires using the rendered net bit class', () => {
        const { widget, container } = makeWidget();
        const { svg } = makeCells(widget, ['u1', 'u2']);
        const wire = document.createElementNS(svgNS, 'line');
        wire.setAttribute('class', 'net_7 width_1');
        wire.style.strokeWidth = '1';
        svg.appendChild(wire);
        const path = timingPath([
            node('u1', { pin: 'u1/Z' }),
            node('u2', { pin: 'u2/A' }),
        ]);
        widget._currentNetlist = {
            modules: { top: {
                cells: {
                    u1: { connections: { Z: [7] } },
                    u2: { connections: { A: [7] } },
                },
                ports: {},
            } },
        };

        widget.showTimingPath(path, path.data_nodes);
        assertColor(wire.style.stroke, '#ff0000');
        assert.equal(wire.style.strokeWidth, '1');
        container.element.remove();
    });

    it('colors capture-path cells and wires green', () => {
        const { widget, container } = makeWidget();
        const { svg, groups } = makeCells(widget, ['cap1', 'cap2']);
        const wire = document.createElementNS(svgNS, 'line');
        wire.setAttribute('class', 'net_9 width_1');
        svg.appendChild(wire);
        const path = timingPath([]);
        path.capture_nodes = [
            node('cap1', { pin: 'cap1/CK', clk: true }),
            node('cap2', { pin: 'cap2/A', clk: true }),
        ];
        widget._currentNetlist = {
            modules: { top: {
                cells: {
                    cap1: { connections: { CK: [9] } },
                    cap2: { connections: { A: [9] } },
                },
                ports: {},
            } },
        };

        widget.showTimingPath(path, path.capture_nodes);

        assertColor(groups.cap1.querySelector('path').style.stroke, '#00ff00');
        assertColor(wire.style.stroke, '#00ff00');
        container.element.remove();
    });

    // A widget wired to a fake server, ready to render path schematics.
    function makePathWidget(netlistOrError) {
        const requests = [];
        const appState = {
            websocketManager: {
                request(msg) {
                    requests.push(msg);
                    return netlistOrError instanceof Error
                        ? Promise.reject(netlistOrError)
                        : Promise.resolve(netlistOrError);
                },
            },
        };
        const { widget, container } = makeWidget(appState);
        widget._netlistsvgReady = true;
        const rendered = [];
        widget.renderNetlist = (json) => {
            rendered.push(json);
            return Promise.resolve(true);
        };
        return { widget, container, requests, rendered };
    }

    function netlistWith(...instNames) {
        const cells = {};
        for (const n of instNames) cells[n] = { type: 'BUF_X1' };
        return { modules: { top: { cells } } };
    }

    function renderedPathSvg(instNames) {
        const cells = instNames.map(instName =>
            `<g id="cell_${instName}"><path class="cell_${instName}"></path></g>`)
            .join('');
        return `<svg>${cells}</svg>`;
    }

    // Exercise renderNetlist's timing reapplication, only replacing server
    // requests and netlistsvg's asynchronous SVG layout.
    function makeRenderingPathWidget(request = msg => {
        if (msg.type === 'schematic_path') {
            return Promise.resolve(netlistWith(...msg.inst_names));
        }
        if (msg.type === 'schematic_cone') {
            return Promise.resolve(netlistWith(msg.inst_name));
        }
        return Promise.resolve({ selected: [{ name: msg.inst_name }] });
    }) {
        const { widget, container } = makeWidget({
            websocketManager: { request },
        });
        widget._netlistsvgReady = true;
        widget.fitView = () => {};
        widget.netlistsvg = {
            render: (_skin, json) => Promise.resolve(renderedPathSvg(
                Object.keys(json.modules.top.cells))),
        };
        return { widget, container };
    }

    function assertPlainSchematic(widget, instNames) {
        assert.deepEqual(Object.keys(widget._currentNetlist.modules.top.cells),
                         instNames);
        assert.equal(widget._timingPath, null);
        assert.equal(widget.svgContainer.querySelector('.schematic-empty-state'), null);
        assert.equal(widget.svgContainer.classList.contains('schematic-empty'), false);
        assert.equal(widget._svgEl.getAttribute('aria-hidden'), null);
        assert.equal(widget._svgEl.querySelector('.schematic-timing-node'), null);
        assert.equal(legendOf(widget).hidden, true);
        assert.equal(widget.controls.querySelector('#schematic-status').textContent,
                     `${instNames.length} cell${instNames.length === 1 ? '' : 's'}`);
    }

    it('keeps the original view reachable after many path and detail-tab selections', async () => {
        const { widget, container } = makeRenderingPathWidget();
        await widget.renderNetlist(netlistWith('previous'));

        for (let i = 0; i < 55; i++) {
            const path = timingPath([node(`data${i}`)]);
            path.capture_nodes = [node(`capture${i}`, { clk: true })];
            await widget.showTimingPath(path, path.data_nodes);
            await widget.showTimingPath(path, path.data_nodes);
            await widget.showTimingPath(path, path.capture_nodes);
        }

        assert.equal(widget._schematicHistory.length, 1);
        assert.ok(widget._schematicHistory[0].netlist.modules.top.cells.previous);
        assert.equal(await widget._goBackSchematic(), true);
        assertPlainSchematic(widget, ['previous']);
        assert.equal(widget._schematicHistory.length, 0);
        assert.equal(widget.controls.querySelector('#schematic-back').disabled, true);
        container.element.remove();
    });

    for (const navigation of ['Refresh', 'Back']) {
        it(`lets ${navigation} replace an empty capture state with a schematic`, async () => {
            const { widget, container } = makeRenderingPathWidget();
            await widget.renderNetlist(netlistWith('previous'));
            const path = timingPath([node('data')]);
            await widget.showTimingPath(path, path.data_nodes);
            await widget.showTimingPath(path, path.capture_nodes);
            assert.ok(widget.svgContainer.querySelector('.schematic-empty-state'));

            widget.appState.selectedInstanceName = 'refreshed';
            const didRender = await (navigation === 'Refresh'
                ? widget.refresh() : widget._goBackSchematic());

            assert.equal(didRender, true);
            assertPlainSchematic(widget,
                [navigation === 'Refresh' ? 'refreshed' : 'previous']);
            assert.equal(widget._schematicHistory.length, 0);
            assert.equal(widget.controls.querySelector('#schematic-back').disabled, true);
            container.element.remove();
        });
    }

    for (const emptyKind of ['capture path', 'server response']) {
        it(`preserves the previous view when the first selected ${emptyKind} is empty`, async () => {
            const { widget, container } = makeRenderingPathWidget(
                () => Promise.resolve(netlistWith()));
            await widget.renderNetlist(netlistWith('previous'));
            const path = timingPath([node('missing')]);

            await widget.showTimingPath(path,
                emptyKind === 'capture path' ? path.capture_nodes : path.data_nodes);

            assert.ok(widget.svgContainer.querySelector('.schematic-empty-state'));
            assert.equal(widget.controls.querySelector('#schematic-back').disabled, false);
            assert.equal(widget._schematicHistory.length, 1);
            assert.equal(await widget._goBackSchematic(), true);
            assertPlainSchematic(widget, ['previous']);
            assert.equal(widget._schematicHistory.length, 0);
            container.element.remove();
        });
    }

    for (const navigation of ['Refresh', 'Back']) {
        for (const pendingStage of ['response', 'layout']) {
            it(`ignores a timing ${pendingStage} that finishes after ${navigation}`, async () => {
                let release;
                const { widget, container } = makeRenderingPathWidget(msg => {
                    if (msg.type === 'schematic_path' && pendingStage === 'response') {
                        return new Promise(resolve => { release = resolve; });
                    }
                    return Promise.resolve(netlistWith(
                        msg.type === 'schematic_path' ? 'late' : msg.inst_name));
                });
                await widget.renderNetlist(netlistWith('previous'));
                widget._pushSchematicHistory({
                    netlist: netlistWith('previous'),
                    selectedInstanceName: null,
                });
                if (pendingStage === 'layout') {
                    const render = widget.netlistsvg.render;
                    widget.netlistsvg.render = (skin, json) => {
                        if (json.modules.top.cells.late) {
                            return new Promise(resolve => { release = resolve; });
                        }
                        return render(skin, json);
                    };
                }

                const pendingPath = widget.showTimingPath(timingPath([node('late')]));
                // Let a resolved path response enter netlistsvg layout.
                await Promise.resolve();
                widget.appState.selectedInstanceName = 'refreshed';
                assert.equal(await (navigation === 'Refresh'
                    ? widget.refresh() : widget._goBackSchematic()), true);

                release(pendingStage === 'response'
                    ? netlistWith('late') : renderedPathSvg(['late']));
                assert.equal(await pendingPath, false);
                assertPlainSchematic(widget,
                    [navigation === 'Refresh' ? 'refreshed' : 'previous']);
                assert.equal(widget._schematicHistory.length, 0);
                container.element.remove();
            });
        }
    }

    for (const navigation of ['Refresh', 'Back']) {
        it(`keeps a newer timing selection when pending ${navigation} finishes`, async () => {
            let release;
            const { widget, container } = makeRenderingPathWidget(msg => {
                if (msg.type === 'schematic_cone') {
                    return new Promise(resolve => { release = resolve; });
                }
                return Promise.resolve(netlistWith(...msg.inst_names));
            });
            await widget.renderNetlist(netlistWith('base'));
            widget.appState.selectedInstanceName = 'base';
            if (navigation === 'Back') {
                widget._pushSchematicHistory({
                    netlist: netlistWith('previous'),
                    selectedInstanceName: 'previous',
                });
                const render = widget.netlistsvg.render;
                widget.netlistsvg.render = (skin, json) => {
                    if (json.modules.top.cells.previous) {
                        return new Promise(resolve => { release = resolve; });
                    }
                    return render(skin, json);
                };
            }

            const pendingNavigation = navigation === 'Refresh'
                ? widget.refresh() : widget._goBackSchematic();
            await Promise.resolve();
            widget.appState.selectedInstanceName = 'selected';
            const path = timingPath([node('selected')]);
            await widget.showTimingPath(path);

            release(navigation === 'Refresh'
                ? netlistWith('base') : renderedPathSvg(['previous']));
            assert.equal(await pendingNavigation, false);
            assert.strictEqual(widget._timingPath, path);
            assert.equal(widget.appState.selectedInstanceName, 'selected');
            assert.deepEqual(Object.keys(widget._currentNetlist.modules.top.cells),
                             ['selected']);
            assertColor(widget._svgEl.querySelector('path').style.stroke, '#ff0000');
            assert.equal(legendOf(widget).hidden, false);
            assert.equal(widget._schematicHistory.length, 1);
            assert.ok(widget._schematicHistory[0].netlist.modules.top.cells.base);
            container.element.remove();
        });
    }

    it('ignores a schematic response for an older timing path', async () => {
        const releases = [];
        const appState = {
            websocketManager: {
                request: () => new Promise(resolve => releases.push(resolve)),
            },
        };
        const { widget, container } = makeWidget(appState);
        widget._netlistsvgReady = true;
        const rendered = [];
        widget.renderNetlist = (json) => {
            rendered.push(json);
            return Promise.resolve(true);
        };

        const older = widget.showTimingPath(timingPath([node('old')]));
        const newer = widget.showTimingPath(timingPath([node('new')]));
        releases[1](netlistWith('new'));
        await newer;
        releases[0](netlistWith('old'));
        await older;

        assert.equal(rendered.length, 1);
        assert.ok(rendered[0].modules.top.cells.new);
        container.element.remove();
    });

    it('does not commit a timing render superseded during layout', async () => {
        const { widget, container } = makeWidget();
        let finishRender;
        widget.netlistsvg = {
            render: () => new Promise(resolve => { finishRender = resolve; }),
        };
        const previous = netlistWith('previous');
        widget._currentNetlist = previous;
        let current = true;

        const pending = widget.renderNetlist(
            netlistWith('stale'), () => current);
        current = false;
        finishRender('<svg></svg>');

        assert.equal(await pending, false);
        assert.strictEqual(widget._currentNetlist, previous);
        container.element.remove();
    });

    it('requests a schematic of the path cells and renders it', async () => {
        const { widget, container, requests, rendered } =
            makePathWidget(netlistWith('u1', 'u2'));

        await widget.showTimingPath(timingPath([node('u1'), node('u2')]));

        const req = requests.find(r => r.type === 'schematic_path');
        assert.ok(req, 'schematic_path was requested');
        assert.deepEqual(req.inst_names, ['u1', 'u2']);
        assert.equal(rendered.length, 1);
        assert.ok(rendered[0].modules.top.cells.u1);
        container.element.remove();
    });

    it('draws the node list it is given, so capture path works too', async () => {
        const { widget, container, requests } =
            makePathWidget(netlistWith('cap1'));
        const path = timingPath([node('u1')]);
        path.capture_nodes = [node('cap1')];

        await widget.showTimingPath(path, path.capture_nodes);

        const req = requests.find(r => r.type === 'schematic_path');
        assert.deepEqual(req.inst_names, ['cap1']);
        container.element.remove();
    });

    it('pushes the previous view so Back returns to it', async () => {
        const { widget, container } = makePathWidget(netlistWith('u1'));
        widget._currentNetlist = netlistWith('previous');

        await widget.showTimingPath(timingPath([node('u1')]));

        assert.equal(widget._schematicHistory.length, 1);
        assert.ok(widget._schematicHistory[0].netlist.modules.top.cells.previous);
        container.element.remove();
    });

    it('replaces a data schematic with an empty capture state and restores it', async () => {
        const requests = [];
        const appState = {
            websocketManager: {
                request(msg) {
                    requests.push(msg);
                    return Promise.resolve(netlistWith('u1'));
                },
            },
        };
        const { widget, container } = makeWidget(appState);
        widget._netlistsvgReady = true;
        widget.fitView = () => {};
        widget.netlistsvg = {
            render: (_skin, json) => Promise.resolve(renderedPathSvg(
                Object.keys(json.modules.top.cells))),
        };
        const path = timingPath([node('u1')]);

        await widget.showTimingPath(path, path.data_nodes);
        const dataSvg = widget._svgEl;
        assertColor(dataSvg.querySelector('path').style.stroke, '#ff0000');
        assert.equal(legendOf(widget).hidden, false);

        await widget.showTimingPath(path, path.capture_nodes);
        assert.equal(requests.length, 1,
                     'an empty capture path does not contact the server');
        assert.equal(dataSvg.getAttribute('aria-hidden'), 'true');
        assert.ok(widget.svgContainer.classList.contains('schematic-empty'));
        assert.equal(widget.svgContainer.querySelector('.schematic-empty-state')
            .textContent, 'No capture path for this output endpoint.');
        assert.equal(legendOf(widget).hidden, true);

        await widget.showTimingPath(path, path.data_nodes);
        assert.equal(requests.length, 2);
        assert.ok(!widget.svgContainer.classList.contains('schematic-empty'));
        assert.equal(widget.svgContainer.querySelector('.schematic-empty-state'), null);
        assert.equal(widget._svgEl.getAttribute('aria-hidden'), null);
        assertColor(widget._svgEl.querySelector('path').style.stroke, '#ff0000');
        assert.equal(legendOf(widget).hidden, false);
        container.element.remove();
    });

    it('hides the prior schematic when the server returns no cells', async () => {
        const { widget, container } = makeWidget({
            websocketManager: {
                request: () => Promise.resolve(netlistWith()),
            },
        });
        const { svg, groups } = makeCells(widget, ['old']);
        await widget.showTimingPath(timingPath([node('old')]));
        assertColor(groups.old.querySelector('path').style.stroke, '#ff0000');

        widget._netlistsvgReady = true;
        await widget.showTimingPath(timingPath([node('new')]));

        assert.equal(svg.getAttribute('aria-hidden'), 'true');
        assert.equal(widget.svgContainer.querySelector('.schematic-empty-state')
            .textContent, 'No schematic cells found for this timing path.');
        assert.equal(legendOf(widget).hidden, true);
        container.element.remove();
    });

    it('does not let an older response replace an empty capture state', async () => {
        let release;
        const { widget, container } = makeWidget({
            websocketManager: {
                request: () => new Promise(resolve => { release = resolve; }),
            },
        });
        widget._netlistsvgReady = true;
        const rendered = [];
        widget.renderNetlist = json => {
            rendered.push(json);
            return Promise.resolve(true);
        };
        const path = timingPath([node('u1')]);

        const pendingData = widget.showTimingPath(path, path.data_nodes);
        await widget.showTimingPath(path, path.capture_nodes);
        release(netlistWith('u1'));
        await pendingData;

        assert.equal(rendered.length, 0);
        assert.equal(widget.svgContainer.querySelector('.schematic-empty-state')
            .textContent, 'No capture path for this output endpoint.');
        container.element.remove();
    });

    it('clears an empty state when timing selection is cleared', async () => {
        const { widget, container } = makeWidget();
        const path = timingPath([]);

        await widget.showTimingPath(path, path.capture_nodes);
        assert.ok(widget.svgContainer.querySelector('.schematic-empty-state'));

        await widget.showTimingPath(null);
        assert.equal(widget.svgContainer.querySelector('.schematic-empty-state'), null);
        assert.ok(!widget.svgContainer.classList.contains('schematic-empty'));
        assert.equal(widget.controls.querySelector('#schematic-status').textContent,
                     'Select an instance in the layout to view its schematic.');
        container.element.remove();
    });

    it('falls back to annotating the current view when the request fails', async () => {
        const { widget, container } = makePathWidget(new Error('boom'));
        const { groups } = makeCells(widget, ['u1']);

        await widget.showTimingPath(timingPath([node('u1')]));

        // The timing styling still went on whatever was already rendered.
        assert.equal(overlayIn(groups.u1).length, 1);
        container.element.remove();
    });

    it('leaves the selection highlight in place', () => {
        const { widget, container } = makeWidget();
        const { groups } = makeCells(widget, ['u1']);

        widget._highlightCellGroup(groups.u1);
        widget.showTimingPath(timingPath([node('u1')]));

        assert.ok(groups.u1.querySelector('#_schematic_highlight'),
                  'selection highlight survives the timing overlay');
        assert.equal(overlayIn(groups.u1).length, 1);
        assert.equal(groups.u1.querySelector('#_schematic_highlight')
            .getAttribute('stroke'), '#e05a00');

        // ...and clearing the timing overlay leaves the selection alone.
        widget.showTimingPath(null);
        assert.ok(groups.u1.querySelector('#_schematic_highlight'));
        container.element.remove();
    });

    it('clears cell selection on a background click without clearing timing colors', async () => {
        const requests = [];
        const { widget, container } = makeWidget({
            websocketManager: {
                request: request => {
                    requests.push(request);
                    return Promise.resolve({ selected: [{ name: request.inst_name }] });
                },
            },
        });
        const { svg, group, path } = makeInteractiveCell(widget, 'u1');
        const wire = document.createElementNS(svgNS, 'line');
        svg.appendChild(wire);
        const timing = timingPath([node('u1')]);
        await widget.showTimingPath(timing);
        widget._toggleSelectMode();
        const zoomTo = widget.controls.querySelector('#schematic-zoom-to');

        for (const target of [svg, wire]) {
            path.dispatchEvent(new window.MouseEvent('mousedown', {
                bubbles: true,
                button: 0,
                detail: 1,
            }));
            assert.ok(group.querySelector('#_schematic_highlight'));
            assert.strictEqual(widget._selectedCell, group);
            assert.equal(zoomTo.disabled, false);
            const inspectCount = requests.length;
            assert.equal(requests.at(-1).type, 'schematic_inspect');

            target.dispatchEvent(new window.MouseEvent('mousedown', {
                bubbles: true,
                button: 0,
                detail: 1,
            }));

            assert.equal(group.querySelector('#_schematic_highlight'), null);
            assert.equal(widget._selectedCell, null);
            assert.equal(zoomTo.disabled, true);
            assert.equal(requests.length, inspectCount);
            assert.strictEqual(widget._timingPath, timing);
            assertColor(path.style.stroke, '#ff0000');
            assert.equal(overlayIn(group).length, 1);
            assert.equal(legendOf(widget).hidden, false);
        }
        container.element.remove();
    });

    it('maps simple gates to upstream skin types and A/B/Y pids', () => {
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

    it('maps an inverter to the upstream not symbol with A/Y', () => {
        const got = canonicalizeCell(cell({
            type: 'INV_X1', gate_kind: 'not',
            port_directions: { A: 'input', ZN: 'output' },
            connections: { A: [7], ZN: [8] },
        }));
        assert.equal(got.type, '$_NOT_');
        assert.deepEqual(got.connections, { A: [7], Y: [8] });
    });

    it('maps a buffer to the upstream buffer symbol', () => {
        const got = canonicalizeCell(cell({
            type: 'BUF_X1', gate_kind: 'buf',
            port_directions: { A: 'input', Z: 'output' },
            connections: { A: [1], Z: [2] },
        }));
        assert.equal(got.type, '$_BUF_');
        assert.deepEqual(got.connections, { A: [1], Y: [2] });
    });

    it('maps a 3-input gate to the upstream per-arity symbol', () => {
        const got = canonicalizeCell(cell({
            type: 'NAND3_X1', gate_kind: 'nand',
            port_directions: { A1: 'input', A2: 'input', A3: 'input', ZN: 'output' },
            connections: { A1: [2], A2: [3], A3: [4], ZN: [5] },
        }));
        assert.equal(got.type, 'nand3');
        assert.deepEqual(got.connections, { A: [2], B: [3], C: [4], Y: [5] });
        assert.deepEqual(got.port_labels, { A: 'A1', B: 'A2', C: 'A3', Y: 'ZN' });
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

        assert.equal(got.type, '$_DFF_');
        assert.deepEqual(got.connections, { D: [1], C: [2], Q: [3], QN: [4] });
        assert.deepEqual(got.port_labels, {
            D: 'D',
            C: 'CK',
            Q: 'Q',
            QN: 'QN',
        });
    });

    it('maps reset/set DFF cells to matching skin register symbols', () => {
        const dffr = canonicalizeCell(cell({
            type: 'DFFR_X1',
            gate_kind: 'dffr',
            gate_ports: { D: 'D', CK: 'CK', RN: 'RN', Q: 'Q' },
            port_directions: { D: 'input', CK: 'input', RN: 'input', Q: 'output' },
            connections: { D: [1], CK: [2], RN: [3], Q: [4] },
        }));
        const dffs = canonicalizeCell(cell({
            type: 'DFFS_X1',
            gate_kind: 'dffs',
            gate_ports: { D: 'D', CK: 'CK', SN: 'SN', Q: 'Q' },
            port_directions: { D: 'input', CK: 'input', SN: 'input', Q: 'output' },
            connections: { D: [1], CK: [2], SN: [3], Q: [4] },
        }));

        assert.equal(dffr.type, '$dffr');
        assert.deepEqual(dffr.connections, { D: [1], C: [2], RN: [3], Q: [4] });
        assert.deepEqual(dffr.port_labels, { D: 'D', C: 'CK', RN: 'RN', Q: 'Q' });
        assert.equal(dffs.type, '$dffs');
        assert.deepEqual(dffs.connections, { D: [1], C: [2], SN: [3], Q: [4] });
        assert.deepEqual(dffs.port_labels, { D: 'D', C: 'CK', SN: 'SN', Q: 'Q' });
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

    it('maps supported compound AOI/OAI gates to upstream skin symbols', () => {
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
        const gotAoi = canonicalizeCell(aoi);
        const gotOai = canonicalizeCell(oai);

        assert.equal(gotAoi.type, 'aoi21');
        assert.deepEqual(gotAoi.connections, { A: [4], B: [8], C: [11], Y: [12] });
        assert.deepEqual(gotAoi.port_labels, { A: 'A', B: 'B1', C: 'B2', Y: 'ZN' });
        assert.equal(gotOai.type, 'oai22');
        assert.deepEqual(gotOai.connections, { A: [1], B: [2], C: [3], D: [4], Y: [5] });
        assert.deepEqual(gotOai.port_labels, {
            A: 'A1', B: 'A2', C: 'B1', D: 'B2', Y: 'ZN',
        });
    });

    it('leaves cells without gate_kind unchanged', () => {
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
        assert.equal(cells._983_.type, '$_NOR_');      // gate rewritten
        assert.equal(cells.myff.type, 'DFF_X1');       // non-gate untouched
        // Input is not mutated.
        assert.equal(json.modules.top.cells._983_.type, 'NOR2_X1');
    });

    it('returns the input unchanged when there is no top module', () => {
        const json = { foo: 1 };
        assert.strictEqual(canonicalizeForSkin(json), json);
    });
});

describe('SchematicWidget wire obstacles', () => {
    it('excludes wires nested anywhere inside a cell group', () => {
        const { widget, container } = makeWidget();
        const svg = document.createElementNS(svgNS, 'svg');
        const cellGroup = document.createElementNS(svgNS, 'g');
        const skinLayer = document.createElementNS(svgNS, 'g');
        const cellStroke = document.createElementNS(svgNS, 'path');
        cellStroke.getBoundingClientRect = () => rect(0, 0, 10, 10);
        skinLayer.appendChild(cellStroke);
        cellGroup.appendChild(skinLayer);

        const netWire = document.createElementNS(svgNS, 'line');
        netWire.getBoundingClientRect = () => rect(20, 30, 60, 30);
        const degenerate = document.createElementNS(svgNS, 'polyline');
        degenerate.getBoundingClientRect = () => rect(70, 70, 70, 70);
        svg.append(cellGroup, netWire, degenerate);
        widget._svgEl = svg;

        assert.deepEqual(widget._wireObstacleRects([cellGroup]), [{
            left: 15, right: 65, top: 25, bottom: 35, width: 50, height: 10,
        }]);
        container.element.remove();
    });

    it('keeps cell membership checks off the cell count', () => {
        const { widget, container } = makeWidget();
        const svg = document.createElementNS(svgNS, 'svg');
        const cellGroups = [];
        const wires = [];
        for (let index = 0; index < 40; index++) {
            cellGroups.push(svg.appendChild(
                document.createElementNS(svgNS, 'g')));
            const wire = document.createElementNS(svgNS, 'path');
            wire.getBoundingClientRect = () => rect(index, 0, index + 10, 10);
            wires.push(svg.appendChild(wire));
        }
        widget._svgEl = svg;

        let probes = 0;
        for (const group of cellGroups) {
            const contains = group.contains.bind(group);
            group.contains = (element) => {
                probes += 1;
                return contains(element);
            };
        }

        assert.equal(widget._wireObstacleRects(cellGroups).length, wires.length);
        // Testing each wire against every cell would probe 40 x 40 times.
        assert.ok(probes <= wires.length,
                  `membership probes scaled with cell count (${probes})`);
        container.element.remove();
    });
});

describe('SchematicWidget label placement', () => {
    function makeLabelCells(widget, displays = [null, 'inline', 'none', null]) {
        const svg = document.createElementNS(svgNS, 'svg');
        const cells = {};
        const groups = [];
        const labels = [];
        displays.forEach((display, index) => {
            const instName = `u${index}`;
            const group = document.createElementNS(svgNS, 'g');
            const label = document.createElementNS(svgNS, 'text');
            group.id = `cell_${instName}`;
            group.getBBox = () => ({ x: 0, y: 0, width: 30, height: 30 });
            group.getBoundingClientRect = () => rect(index * 100, 0, index * 100 + 30, 30);
            label.setAttribute('data-openroad-label', 'instance');
            label.setAttribute('x', '15');
            label.setAttribute('y', '-4');
            if (display !== null) label.setAttribute('display', display);
            label.getBoundingClientRect = () => {
                const x = Number(label.getAttribute('x')) + index * 100;
                const y = Number(label.getAttribute('y'));
                return rect(x - 4, y - 6, x + 4, y);
            };
            group.appendChild(label);
            svg.appendChild(group);
            cells[instName] = {};
            groups.push(group);
            labels.push(label);
        });
        widget.svgContainer.replaceChildren(svg);
        widget._svgEl = svg;
        return { svg, groups, labels, netlist: { modules: { top: { cells } } } };
    }

    it('keeps label layout flushes constant as the cell count grows', () => {
        const { widget, container } = makeWidget();
        const displays = Array.from({ length: 20 }, (_, index) =>
            [null, 'inline', 'none'][index % 3]);
        const { groups, labels, netlist } = makeLabelCells(widget, displays);
        let layoutDirty = true;
        let layoutFlushes = 0;
        const readLayout = () => {
            if (layoutDirty) layoutFlushes += 1;
            layoutDirty = false;
        };
        // Model a layout flush when a geometry read follows a label write.
        for (const label of labels) {
            const setAttribute = label.setAttribute.bind(label);
            const removeAttribute = label.removeAttribute.bind(label);
            label.setAttribute = (name, value) => {
                layoutDirty = true;
                setAttribute(name, value);
            };
            label.removeAttribute = (name) => {
                layoutDirty = true;
                removeAttribute(name);
            };
        }
        for (const element of [...groups, ...labels]) {
            for (const method of ['getBBox', 'getBoundingClientRect']) {
                if (typeof element[method] !== 'function') continue;
                const measure = element[method].bind(element);
                element[method] = () => {
                    readLayout();
                    return measure();
                };
            }
        }
        widget._layoutInstanceLabels(netlist);

        assert.ok(layoutFlushes <= 7, `label layout flushed layout ${layoutFlushes} times`);
        assert.deepEqual(labels.map((label) => label.getAttribute('display')), displays);
        container.element.remove();
    });

    it('restores original label display attributes if obstacle measurement fails', () => {
        const { widget, container } = makeWidget();
        const { svg, labels, netlist } = makeLabelCells(widget);
        const obstacle = document.createElementNS(svgNS, 'text');
        obstacle.getBoundingClientRect = () => { throw new Error('measurement failed'); };
        svg.appendChild(obstacle);

        assert.throws(() => widget._layoutInstanceLabels(netlist), /measurement failed/);

        assert.deepEqual(labels.map((label) => label.getAttribute('display')),
            [null, 'inline', 'none', null]);
        container.element.remove();
    });

    it('avoids wires and fixed text while skipping an unmeasurable cell', () => {
        const { widget, container } = makeWidget();
        const { svg, groups, labels, netlist } = makeLabelCells(widget, [null, 'inline', null]);
        const wire = document.createElementNS(svgNS, 'path');
        wire.getBoundingClientRect = () => rect(-15, -23, 45, -17);
        const portLabel = document.createElementNS(svgNS, 'text');
        portLabel.getBoundingClientRect = () => rect(85, -23, 145, -17);
        svg.append(wire, portLabel);
        groups[2].getBBox = () => { throw new Error('unmeasurable cell'); };

        widget._layoutInstanceLabels(netlist);

        assert.equal(labels[0].getAttribute('y'), '44');
        assert.equal(labels[1].getAttribute('y'), '44');
        assert.equal(labels[2].getAttribute('y'), '-4');
        assert.deepEqual(labels.map((label) => label.getAttribute('display')), [null, 'inline', null]);
        container.element.remove();
    });

    it('creates OpenROAD port labels from skin pin markers', () => {
        const { widget, container } = makeWidget();
        const svg = document.createElementNS(svgNS, 'svg');
        const group = document.createElementNS(svgNS, 'g');
        group.getBBox = () => ({ x: 0, y: 0, width: 30, height: 30 });

        const inputMarker = document.createElementNS(svgNS, 'g');
        inputMarker.setAttribute('s:pid', 'A');
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
            type: '$_NAND_',
            port_labels: { A: 'A1', Y: 'ZN' },
            port_directions: { A: 'input', Y: 'output' },
        });

        const input = group.querySelector('text[data-openroad-port="A"]');
        const output = group.querySelector('text[data-openroad-port="Y"]');
        assert.equal(input.textContent, 'A1');
        assert.equal(input.parentElement, group);
        assert.equal(input.getAttribute('x'), '-3');
        assert.equal(input.getAttribute('y'), '3');
        assert.equal(input.style.fontSize, '5px');
        assert.equal(input.style.textAnchor, 'end');
        assert.equal(output.textContent, 'ZN');
        assert.equal(output.getAttribute('x'), '34');
        assert.equal(output.getAttribute('y'), '11');
        assert.equal(output.style.pointerEvents, 'none');
        container.element.remove();
    });

    it('places labels using translated skin pin markers', () => {
        const { widget, container } = makeWidget();
        const svg = document.createElementNS(svgNS, 'svg');
        const group = document.createElementNS(svgNS, 'g');
        const wrapper = document.createElementNS(svgNS, 'g');
        const marker = document.createElementNS(svgNS, 'g');

        wrapper.setAttribute('transform', 'translate(20, 8)');
        marker.setAttribute('s:pid', 'A');
        marker.setAttribute('s:x', '3');
        marker.setAttribute('s:y', '5');
        wrapper.appendChild(marker);
        group.appendChild(wrapper);
        svg.appendChild(group);
        widget._svgEl = svg;

        widget._updateOpenRoadPortLabels(group, {
            port_labels: { A: 'A1' },
            port_directions: { A: 'input' },
        });

        const input = group.querySelector('text[data-openroad-port="A"]');
        assert.equal(input.textContent, 'A1');
        assert.equal(input.getAttribute('x'), '20');
        assert.equal(input.getAttribute('y'), '9');
        assert.equal(input.style.textAnchor, 'end');
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


// syncCone only reads `detail` and writes the two depth inputs, so exercise it
// on a stand-in rather than building a whole widget (which needs Leaflet and a
// live socket).
describe('SchematicWidget.syncCone', () => {
    function makeStub() {
        const controls = document.createElement('div');
        controls.innerHTML =
            '<input id="schematic-fanin-depth"  type="number" value="1" min="0" max="10">' +
            '<input id="schematic-fanout-depth" type="number" value="1" min="0" max="10">';
        const stub = {
            controls,
            refreshed: 0,
            refresh() { this.refreshed++; },
            appState: {},
        };
        return stub;
    }
    const depths = (s) => [
        s.controls.querySelector('#schematic-fanin-depth').value,
        s.controls.querySelector('#schematic-fanout-depth').value,
    ];

    it('translates the timing cone\'s "0 = unlimited" to this view\'s max', () => {
        const s = makeStub();
        // The timing cone panel's own default: both directions on, both
        // depths 0 meaning unlimited.  Copied verbatim this used to mean "do
        // not expand", collapsing the schematic to the target instance.
        SchematicWidget.prototype.syncCone.call(s, {
            inst_name: 'b0', fanin: true, fanout: true,
            fanin_depth: 0, fanout_depth: 0,
        });
        assert.deepEqual(depths(s), ['10', '10']);
        assert.equal(s.refreshed, 1);
    });

    it('zeroes a direction the timing cone has switched off', () => {
        const s = makeStub();
        SchematicWidget.prototype.syncCone.call(s, {
            inst_name: 'b0', fanin: false, fanout: true, fanout_depth: 3,
        });
        assert.deepEqual(depths(s), ['0', '3']);
    });

    it('passes a finite depth through, clamped to the control maximum', () => {
        const s = makeStub();
        SchematicWidget.prototype.syncCone.call(s, {
            inst_name: 'b0', fanin: true, fanout: true,
            fanin_depth: 2, fanout_depth: 50,
        });
        assert.deepEqual(depths(s), ['2', '10']);
    });
});
