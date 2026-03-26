// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it, beforeEach } from 'node:test';
import assert from 'node:assert/strict';

// ─── Minimal Leaflet mock ────────────────────────────────────────────────────
// RulerManager uses L.layerGroup, L.polyline, L.circleMarker, L.marker,
// L.divIcon, and L.DomEvent.stopPropagation.  We provide just enough to let
// the constructor and state-machine logic run without a real browser.

const layers = [];

function makeFakeLayer() {
    return {
        addTo() { return this; },
        addLayer(l) { layers.push(l); },
        removeLayer() {},
        clearLayers() { layers.length = 0; },
        on() {},
        off() {},
    };
}

globalThis.L = {
    layerGroup() { return makeFakeLayer(); },
    polyline() { return makeFakeLayer(); },
    circleMarker() { return makeFakeLayer(); },
    marker() { return makeFakeLayer(); },
    divIcon() { return {}; },
    DomEvent: { stopPropagation() {} },
};

// Node.js doesn't have requestAnimationFrame.
globalThis.requestAnimationFrame = (cb) => setTimeout(cb, 0);
globalThis.document = { createElement() { return { className: '', innerHTML: '', addEventListener() {} }; } };

const { RulerManager } = await import('../../src/ruler.js');

// ─── Test helpers ────────────────────────────────────────────────────────────

function makeApp() {
    const classList = new Set();
    const container = {
        classList: {
            add(c) { classList.add(c); },
            remove(c) { classList.delete(c); },
            has(c) { return classList.has(c); },
        },
        addEventListener() {},
        removeEventListener() {},
        getBoundingClientRect() { return { left: 0, top: 0 }; },
    };
    const requests = [];
    return {
        map: {
            createPane() { return { style: {} }; },
            getContainer() { return container; },
            getZoom() { return 1; },
            on() {},
            off() {},
        },
        designScale: 1e-6,
        designMaxDXDY: 1000000,
        techData: { dbu_per_micron: 1000 },
        visibleLayers: new Set(['metal1']),
        websocketManager: {
            request(msg) {
                requests.push(msg);
                return Promise.resolve({ found: false });
            },
        },
        inspectorEl: null,
        _container: container,
        _classList: classList,
        _requests: requests,
    };
}

function makeManager(appOverrides = {}) {
    const app = makeApp();
    Object.assign(app, appOverrides);
    const inspectorData = [];
    const focusCalls = [];
    const visibility = { stdcells: true, macros: true, routing: true };
    const mgr = new RulerManager(
        app,
        visibility,
        (data) => inspectorData.push(data),
        (comp) => focusCalls.push(comp),
    );
    return { mgr, app, inspectorData, focusCalls };
}

// ─── State machine ───────────────────────────────────────────────────────────

describe('RulerManager state machine', () => {
    it('starts in IDLE', () => {
        const { mgr } = makeManager();
        assert.equal(mgr.isActive(), false);
    });

    it('toggleRulerMode enters FIRST_POINT', () => {
        const { mgr, app } = makeManager();
        mgr.toggleRulerMode();
        assert.equal(mgr.isActive(), true);
        assert.ok(app._classList.has('ruler-mode-cursor'));
    });

    it('toggleRulerMode again cancels back to IDLE', () => {
        const { mgr, app } = makeManager();
        mgr.toggleRulerMode();
        mgr.toggleRulerMode();
        assert.equal(mgr.isActive(), false);
        assert.ok(!app._classList.has('ruler-mode-cursor'));
    });

    it('cancelRulerBuild returns to IDLE', () => {
        const { mgr } = makeManager();
        mgr.toggleRulerMode();
        mgr.cancelRulerBuild();
        assert.equal(mgr.isActive(), false);
    });
});

// ─── Ruler creation and deletion ─────────────────────────────────────────────

describe('Ruler creation and deletion', () => {
    it('_createRuler adds a ruler', () => {
        const { mgr } = makeManager();
        mgr._createRuler({ x: 0, y: 0 }, { x: 1000, y: 2000 });
        assert.equal(mgr._rulers.length, 1);
        assert.deepEqual(mgr._rulers[0].pt0, { x: 0, y: 0 });
        assert.deepEqual(mgr._rulers[0].pt1, { x: 1000, y: 2000 });
    });

    it('rulers get incrementing IDs', () => {
        const { mgr } = makeManager();
        mgr._createRuler({ x: 0, y: 0 }, { x: 100, y: 100 });
        mgr._createRuler({ x: 200, y: 200 }, { x: 300, y: 300 });
        assert.equal(mgr._rulers[0].id, 0);
        assert.equal(mgr._rulers[1].id, 1);
    });

    it('deleteRuler removes the correct ruler', () => {
        const { mgr } = makeManager();
        mgr._createRuler({ x: 0, y: 0 }, { x: 100, y: 100 });
        mgr._createRuler({ x: 200, y: 200 }, { x: 300, y: 300 });
        mgr.deleteRuler(0);
        assert.equal(mgr._rulers.length, 1);
        assert.equal(mgr._rulers[0].id, 1);
    });

    it('deleteRuler clears selection if deleted ruler was selected', () => {
        const { mgr, inspectorData } = makeManager();
        mgr._createRuler({ x: 0, y: 0 }, { x: 100, y: 100 });
        mgr._selectedRulerId = 0;
        mgr.deleteRuler(0);
        assert.equal(mgr._selectedRulerId, null);
        assert.equal(inspectorData[inspectorData.length - 1], null);
    });

    it('deleteRuler with non-existent ID is a no-op', () => {
        const { mgr } = makeManager();
        mgr._createRuler({ x: 0, y: 0 }, { x: 100, y: 100 });
        mgr.deleteRuler(999);
        assert.equal(mgr._rulers.length, 1);
    });

    it('clearAllRulers removes all rulers', () => {
        const { mgr } = makeManager();
        mgr._createRuler({ x: 0, y: 0 }, { x: 100, y: 100 });
        mgr._createRuler({ x: 200, y: 200 }, { x: 300, y: 300 });
        mgr.clearAllRulers();
        assert.equal(mgr._rulers.length, 0);
        assert.equal(mgr._selectedRulerId, null);
    });

    it('clearAllRulers cancels active build', () => {
        const { mgr } = makeManager();
        mgr.toggleRulerMode();
        assert.equal(mgr.isActive(), true);
        mgr.clearAllRulers();
        assert.equal(mgr.isActive(), false);
    });
});

// ─── _getSnappedPoint ────────────────────────────────────────────────────────

describe('_getSnappedPoint', () => {
    it('returns raw point when no snap result', () => {
        const { mgr } = makeManager();
        mgr._lastSnapResult = null;
        const pt = mgr._getSnappedPoint(100, 200);
        assert.deepEqual(pt, { x: 100, y: 200 });
    });

    it('returns edge point for point snap', () => {
        const { mgr } = makeManager();
        mgr._lastSnapResult = {
            found: true,
            is_point: true,
            edge: [[500, 600], [500, 600]],
        };
        const pt = mgr._getSnappedPoint(490, 590);
        assert.deepEqual(pt, { x: 500, y: 600 });
    });

    it('snaps Y for horizontal edge', () => {
        const { mgr } = makeManager();
        mgr._lastSnapResult = {
            found: true,
            is_point: false,
            edge: [[0, 500], [1000, 500]],
        };
        const pt = mgr._getSnappedPoint(300, 490);
        assert.equal(pt.y, 500);
        assert.equal(pt.x, 300);
    });

    it('snaps X for vertical edge', () => {
        const { mgr } = makeManager();
        mgr._lastSnapResult = {
            found: true,
            is_point: false,
            edge: [[500, 0], [500, 1000]],
        };
        const pt = mgr._getSnappedPoint(510, 300);
        assert.equal(pt.x, 500);
        assert.equal(pt.y, 300);
    });

    it('returns raw point when snap.found is false', () => {
        const { mgr } = makeManager();
        mgr._lastSnapResult = { found: false };
        const pt = mgr._getSnappedPoint(42, 99);
        assert.deepEqual(pt, { x: 42, y: 99 });
    });
});

// ─── Modifier key behavior ───────────────────────────────────────────────────

describe('Modifier keys — _getSnappedPoint', () => {
    it('Ctrl bypasses snap entirely', () => {
        const { mgr } = makeManager();
        mgr._ctrlHeld = true;
        mgr._lastSnapResult = {
            found: true,
            is_point: true,
            edge: [[500, 600], [500, 600]],
        };
        const pt = mgr._getSnappedPoint(100, 200);
        assert.deepEqual(pt, { x: 100, y: 200 });
    });

    it('default (no modifiers) applies axis lock — horizontal', () => {
        const { mgr } = makeManager();
        mgr._state = 2;  // MEASURING
        mgr._pt0 = { x: 100, y: 100 };
        mgr._ctrlHeld = false;
        mgr._shiftHeld = false;
        mgr._lastSnapResult = null;
        // dx=400 > dy=50 → horizontal → lock Y to pt0.y
        const pt = mgr._getSnappedPoint(500, 150);
        assert.equal(pt.y, 100);  // locked to pt0.y
        assert.equal(pt.x, 500);  // x unchanged
    });

    it('default (no modifiers) applies axis lock — vertical', () => {
        const { mgr } = makeManager();
        mgr._state = 2;  // MEASURING
        mgr._pt0 = { x: 100, y: 100 };
        mgr._ctrlHeld = false;
        mgr._shiftHeld = false;
        mgr._lastSnapResult = null;
        // dx=50 < dy=400 → vertical → lock X to pt0.x
        const pt = mgr._getSnappedPoint(150, 500);
        assert.equal(pt.x, 100);  // locked to pt0.x
        assert.equal(pt.y, 500);  // y unchanged
    });

    it('Shift disables axis lock', () => {
        const { mgr } = makeManager();
        mgr._state = 2;  // MEASURING
        mgr._pt0 = { x: 100, y: 100 };
        mgr._ctrlHeld = false;
        mgr._shiftHeld = true;
        mgr._lastSnapResult = null;
        const pt = mgr._getSnappedPoint(500, 150);
        // With Shift, no axis lock — both coords preserved
        assert.equal(pt.x, 500);
        assert.equal(pt.y, 150);
    });

    it('axis lock not applied in FIRST_POINT state', () => {
        const { mgr } = makeManager();
        mgr._state = 1;  // FIRST_POINT
        mgr._pt0 = null;
        mgr._ctrlHeld = false;
        mgr._shiftHeld = false;
        mgr._lastSnapResult = null;
        const pt = mgr._getSnappedPoint(500, 150);
        assert.equal(pt.x, 500);
        assert.equal(pt.y, 150);
    });

    it('axis lock combines with edge snap', () => {
        const { mgr } = makeManager();
        mgr._state = 2;  // MEASURING
        mgr._pt0 = { x: 100, y: 100 };
        mgr._ctrlHeld = false;
        mgr._shiftHeld = false;
        // Vertical edge snap: X snaps to 500
        mgr._lastSnapResult = {
            found: true,
            is_point: false,
            edge: [[500, 0], [500, 1000]],
        };
        // dx=400 > dy=50 → horizontal → lock Y to pt0.y=100
        const pt = mgr._getSnappedPoint(510, 150);
        assert.equal(pt.x, 500);  // from edge snap
        assert.equal(pt.y, 100);  // from axis lock
    });
});

// ─── _computeDistance ────────────────────────────────────────────────────────

describe('_computeDistance', () => {
    it('computes euclidian distance', () => {
        const { mgr } = makeManager();
        const d = mgr._computeDistance({ x: 0, y: 0 }, { x: 3000, y: 4000 });
        assert.equal(d, 5000);
    });

    it('computes manhattan distance when euclidian=false', () => {
        const { mgr } = makeManager();
        const d = mgr._computeDistance(
            { x: 0, y: 0 }, { x: 3000, y: 4000 }, false);
        assert.equal(d, 7000);
    });

    it('handles zero distance', () => {
        const { mgr } = makeManager();
        assert.equal(mgr._computeDistance({ x: 5, y: 5 }, { x: 5, y: 5 }), 0);
    });
});

// ─── _formatDistance ─────────────────────────────────────────────────────────

describe('_formatDistance', () => {
    it('formats millimeters', () => {
        const { mgr } = makeManager();
        // 1000 um = 1 mm; dbu_per_micron=1000 → 1000000 dbu = 1 mm
        assert.equal(mgr._formatDistance(1000000), '1.000 mm');
    });

    it('formats micrometers', () => {
        const { mgr } = makeManager();
        // 5000 dbu / 1000 = 5 um
        assert.equal(mgr._formatDistance(5000), '5.000 um');
    });

    it('formats nanometers', () => {
        const { mgr } = makeManager();
        // 500 dbu / 1000 = 0.5 um → 500 nm
        assert.equal(mgr._formatDistance(500), '500.0 nm');
    });

    it('formats zero', () => {
        const { mgr } = makeManager();
        assert.equal(mgr._formatDistance(0), '0.0 nm');
    });
});

// ─── Inspector data ──────────────────────────────────────────────────────────

describe('Ruler inspector data', () => {
    it('_selectRuler produces correct inspector properties', () => {
        const { mgr, inspectorData, focusCalls } = makeManager();
        mgr._createRuler({ x: 1000, y: 2000 }, { x: 4000, y: 6000 });
        mgr._selectRuler(0);

        assert.equal(focusCalls[0], 'Inspector');

        const data = inspectorData[0];
        assert.equal(data.type, 'Ruler');
        assert.equal(data.name, 'ruler0');

        // bbox
        assert.deepEqual(data.bbox, [1000, 2000, 4000, 6000]);

        // Check key properties
        const propMap = {};
        for (const p of data.properties) {
            propMap[p.name] = p.value;
        }
        assert.equal(propMap['Name'], 'ruler0');
        assert.equal(propMap['Point 0 - x'], '1.000 um');
        assert.equal(propMap['Point 0 - y'], '2.000 um');
        assert.equal(propMap['Point 1 - x'], '4.000 um');
        assert.equal(propMap['Point 1 - y'], '6.000 um');
        assert.equal(propMap['Delta x'], '3.000 um');
        assert.equal(propMap['Delta y'], '4.000 um');
        assert.equal(propMap['Length'], '5.000 um');
        assert.equal(propMap['Euclidian'], 'true');
    });

    it('selection changes selectedRulerId', () => {
        const { mgr } = makeManager();
        mgr._createRuler({ x: 0, y: 0 }, { x: 100, y: 100 });
        mgr._createRuler({ x: 200, y: 200 }, { x: 300, y: 300 });
        mgr._selectRuler(1);
        assert.equal(mgr._selectedRulerId, 1);
    });
});

// ─── Ruler naming ────────────────────────────────────────────────────────────

describe('Ruler naming', () => {
    it('rulers get sequential names', () => {
        const { mgr } = makeManager();
        mgr._createRuler({ x: 0, y: 0 }, { x: 10, y: 10 });
        mgr._createRuler({ x: 0, y: 0 }, { x: 20, y: 20 });
        assert.equal(mgr._rulers[0].name, 'ruler0');
        assert.equal(mgr._rulers[1].name, 'ruler1');
    });

    it('ruler defaults to euclidian', () => {
        const { mgr } = makeManager();
        mgr._createRuler({ x: 0, y: 0 }, { x: 10, y: 10 });
        assert.equal(mgr._rulers[0].euclidian, true);
    });
});
