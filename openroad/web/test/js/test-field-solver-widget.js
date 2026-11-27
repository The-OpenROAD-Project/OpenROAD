// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it } from 'node:test';
import assert from 'node:assert/strict';

import {
    FieldSolver2DWidget,
    createDefaultWires,
    wiresOverlap,
    hasAnyOverlap,
    resizeWireEdge,
    findNewWirePlacement,
    chargeToColor,
    pickDefaultItfLayer,
    nearestItfLayer,
    snapWireToLayer,
    dielectricColor,
    smoothChargeFace,
    minWidthForWire,
    hasSpacingViolation,
    MIN_WIRE_WIDTH,
} from '../../src/field-solver-widget.js';

function makeContainer() {
    const element = document.createElement('div');
    const handlers = new Map();
    return {
        element,
        on(ev, fn) {
            if (!handlers.has(ev)) handlers.set(ev, []);
            handlers.get(ev).push(fn);
        },
        emit(ev) {
            for (const fn of handlers.get(ev) || []) fn();
        },
    };
}

function makeMockApp(mockResponse) {
    const calls = [];
    return {
        calls,
        websocketManager: {
            async request(payload) {
                calls.push(payload);
                return typeof mockResponse === 'function'
                    ? mockResponse(payload)
                    : mockResponse;
            },
        },
    };
}

describe('FieldSolver2D pure geometry helpers', () => {
    it('creates default scene with target centered at x = 0', () => {
        const wires = createDefaultWires();
        assert.equal(wires.length, 3);
        const target = wires.find(w => w.isTarget);
        assert.ok(target);
        assert.equal(target.x, 0);
        assert.equal(hasAnyOverlap(wires), false);
    });

    it('detects wire overlap accurately', () => {
        const a = { x: 0, z: 0.4, width: 0.2, height: 0.2 };
        const b = { x: 0.15, z: 0.4, width: 0.2, height: 0.2 };
        const c = { x: 0.4, z: 0.4, width: 0.2, height: 0.2 };
        assert.equal(wiresOverlap(a, b), true);
        assert.equal(wiresOverlap(a, c), false);
    });

    it('resizes target wire symmetrically while keeping x = 0', () => {
        const target = { id: 0, name: 'target', x: 0, z: 0.4, width: 0.2, height: 0.2, isTarget: true };
        resizeWireEdge(target, 'right', 0.25);
        assert.equal(target.x, 0);
        assert.equal(target.width, 0.5);

        resizeWireEdge(target, 'left', -0.15);
        assert.equal(target.x, 0);
        assert.equal(target.width, 0.3);

        // Respects minimum width.
        resizeWireEdge(target, 'right', 0.001);
        assert.equal(target.x, 0);
        assert.equal(target.width, MIN_WIRE_WIDTH);
    });

    it('resizes neighbor wire from either edge keeping opposite edge fixed', () => {
        const w = { id: 1, name: 'w1', x: 0.4, z: 0.4, width: 0.2, height: 0.2, isTarget: false };
        // x0 = 0.3, x1 = 0.5
        resizeWireEdge(w, 'right', 0.7);
        assert.equal(w.width, 0.4);
        assert.equal(w.x, 0.5);

        resizeWireEdge(w, 'left', 0.5);
        assert.equal(w.width, 0.2);
        assert.equal(w.x, 0.6);
    });

    it('finds non-overlapping placement for new wires', () => {
        const wires = createDefaultWires();
        const pos = findNewWirePlacement(wires, 0.2, 0.2, 0.4);
        const probe = { x: pos.x, z: pos.z, width: 0.2, height: 0.2 };
        assert.equal(wires.some(w => wiresOverlap(w, probe)), false);
    });

    it('maps positive and negative surface charges to distinct warm/cool colors', () => {
        const pos = chargeToColor(0.5, 1.0);
        const neg = chargeToColor(-0.5, 1.0);
        assert.match(pos, /^rgb\(/);
        assert.match(neg, /^rgb\(/);
        assert.notEqual(pos, neg);
    });

    it('smoothChargeFace enforces sign and suppresses single-bin FRW spikes', () => {
        // Target wire: negative noise clamped to 0, isolated interior spike damped,
        // legitimate corner peaks preserved.
        const raw = [0.8, 0.25, 0.2, 5.0, 0.2, -0.1, 0.85];
        const clean = smoothChargeFace(raw, true);
        assert.equal(clean.length, raw.length);
        assert.ok(clean.every(v => v >= 0));
        assert.ok(clean[3] < 0.5, `expected spike damped, got ${clean[3]}`);
        assert.equal(clean[0], 0.8);
        assert.equal(clean[6], 0.85);

        // Neighbor wire: positive noise clamped to 0.
        const nbClean = smoothChargeFace([-0.5, 0.05, -0.2, -0.5], false);
        assert.ok(nbClean.every(v => v <= 0));
    });
});

describe('FieldSolver2DWidget interactive operations', () => {
    it('adds, selects, resizes, and removes wires while keeping target centered', async () => {
        const mockResp = payload => {
            if (payload.type === 'fsv_get_itf') {
                return { loaded: false };
            }
            return {
                total: 0.15,
                total_sigma: 0.001,
                ground: 0.05,
                ground_sigma: 0.001,
                walks: 65536,
                seconds: 0.012,
                couplings: payload.wires.slice(1).map((w, i) => ({
                    index: i + 1,
                    name: w.name,
                    cap: 0.05,
                    sigma: 0.0005,
                })),
                charges: payload.wires.map((w, i) => ({
                    index: i,
                    name: w.name,
                    bottom: [0.3, 0.2, 0.3],
                    top: [0.2, 0.1, 0.2],
                    left: [0.25, 0.25],
                    right: [0.25, 0.25],
                })),
            };
        };

        const container = makeContainer();
        const app = makeMockApp(mockResp);
        const widget = new FieldSolver2DWidget(container, app);

        // Initial sync + solve on show.
        container.emit('show');
        await new Promise(r => setTimeout(r, 10));
        const solveCalls = app.calls.filter(c => c.type === 'fsv_solve_2d');
        assert.equal(solveCalls.length, 1);
        assert.equal(solveCalls[0].wires[0].x, 0);

        // Add wire.
        const added = widget.addWire();
        assert.ok(added);
        assert.equal(widget.wires.length, 4);
        assert.equal(widget.selectedWire.id, added.id);

        // Cannot remove target wire.
        assert.equal(widget.removeWire(0), false);
        assert.equal(widget.wires.length, 4);

        // Can remove added neighbor wire.
        assert.equal(widget.removeWire(added.id), true);
        assert.equal(widget.wires.length, 3);

        // Target width change via property input keeps x = 0.
        widget.selectWire(0);
        const xInput = widget._propInputs.x;
        const wInput = widget._propInputs.width;
        assert.equal(xInput.disabled, true);
        assert.equal(wInput.disabled, false);

        wInput.value = '0.30';
        wInput.dispatchEvent(new window.Event('change'));
        assert.equal(widget.wires[0].width, 0.3);
        assert.equal(widget.wires[0].x, 0);

        // Unselecting clears selectedWire and disables property inputs.
        widget.selectWire(null);
        assert.equal(widget.selectedWire, null);
        assert.equal(widget._propInputs.width.disabled, true);
        assert.equal(widget._removeBtn.disabled, true);

        widget.destroy();
    });

    it('loads ITF stack, snaps wires to layers, and toggles dielectrics', async () => {
        const mockStack = {
            technology: 'sky130',
            source: 'sky130.itf',
            background_eps: 1.0,
            dielectrics: [
                { name: 'FOX', bottom: 0.0, top: 0.3, eps: 3.9 },
                { name: 'ILD1', bottom: 0.3, top: 1.0, eps: 4.5 },
                { name: 'ILD2', bottom: 1.0, top: 2.0, eps: 4.2 },
            ],
            conductors: [
                { name: 'li1', bottom: 0.5, top: 0.6, thickness: 0.1, min_width: 0.17, min_spacing: 0.17, conformals: [] },
                { name: 'met1', bottom: 0.8, top: 1.16, thickness: 0.36, min_width: 0.14, min_spacing: 0.14, conformals: [{ name: 'C1', side_thickness: 0.03, top_thickness: 0.0, eps: 3.5 }] },
                { name: 'met2', bottom: 1.5, top: 1.86, thickness: 0.36, min_width: 0.14, min_spacing: 0.14, conformals: [] },
            ],
            warnings: [],
        };

        assert.equal(pickDefaultItfLayer(mockStack.conductors).name, 'met1');
        assert.equal(nearestItfLayer(mockStack.conductors, 1.65).name, 'met2');
        assert.match(dielectricColor(3.9, true), /^rgba\(/);

        const app = makeMockApp(payload => {
            if (payload.type === 'fsv_load_itf') return mockStack;
            if (payload.type === 'fsv_clear_itf') return { ok: true };
            return {
                total: 0.18,
                total_sigma: 0.001,
                ground: 0.04,
                ground_sigma: 0.001,
                walks: 65536,
                seconds: 0.01,
                couplings: [],
                charges: [],
            };
        });

        const container = makeContainer();
        const widget = new FieldSolver2DWidget(container, app);
        await widget.loadItfFromPath('sky130.itf');

        // Default wires snapped to met1, target still centered at x = 0.
        assert.equal(widget.wires[0].layer, 'met1');
        assert.equal(widget.wires[0].x, 0);
        assert.equal(widget.wires[0].z, 0.8);
        assert.ok(Math.abs(widget.wires[0].height - 0.36) < 1e-12);

        // Snap preserves high-precision ITF z/height without 4-decimal rounding
        // so conformal dielectrics match within 1e-9 um.
        const preciseLayer = { name: 'met_hi', bottom: 0.60991234, top: 0.96996789 };
        const probe = { x: 0, z: 0.4, width: 0.2, height: 0.2 };
        snapWireToLayer(probe, preciseLayer);
        assert.equal(probe.z, 0.60991234);
        assert.equal(probe.height, preciseLayer.top - preciseLayer.bottom);

        // Z and height inputs are locked to the ITF layer; width remains editable.
        assert.equal(widget._propInputs.z.disabled, true);
        assert.equal(widget._propInputs.height.disabled, true);
        assert.equal(widget._propInputs.width.disabled, false);

        // Switching selected wire to met2 via dropdown snaps z and height.
        widget._layerSelectEl.value = 'met2';
        widget._layerSelectEl.dispatchEvent(new window.Event('change'));
        assert.equal(widget.wires[0].layer, 'met2');
        assert.equal(widget.wires[0].z, 1.5);
        assert.equal(widget.wires[0].x, 0);

        // Adding a wire snaps to an ITF layer too.
        const added = widget.addWire();
        assert.equal(added.layer, 'met2');
        assert.equal(added.z, 1.5);

        // Solve payload carries use_itf: true and wire layer names.
        const solveCalls = app.calls.filter(c => c.type === 'fsv_solve_2d');
        assert.ok(solveCalls.length >= 1);
        const lastSolve = solveCalls[solveCalls.length - 1];
        assert.equal(lastSolve.use_itf, true);
        assert.equal(lastSolve.wires[0].layer, 'met2');

        // Clearing ITF unlocks z and height.
        await widget.clearItf();
        assert.equal(widget.itfStack, null);
        assert.equal(widget._propInputs.z.disabled, false);

        widget.destroy();
    });

    it('enforces ITF min_width and min_spacing', () => {
        const itf = {
            conductors: [
                { name: 'met1', bottom: 0.8, top: 1.16, thickness: 0.36, min_width: 0.14, min_spacing: 0.14 },
            ],
        };
        const a = { id: 0, name: 'target', x: 0.0, z: 0.8, width: 0.14, height: 0.36, layer: 'met1', isTarget: true };
        const b = { id: 1, name: 'w1', x: 0.28, z: 0.8, width: 0.14, height: 0.36, layer: 'met1', isTarget: false };
        assert.equal(minWidthForWire(a, itf), 0.14);
        // Exact min_spacing (gap = 0.21 - 0.07 = 0.14): valid.
        assert.equal(hasSpacingViolation([a, b], itf), false);
        // Slightly closer than min_spacing (gap = 0.12 < 0.14): violation.
        b.x = 0.26;
        assert.equal(hasSpacingViolation([a, b], itf), true);
    });
});
