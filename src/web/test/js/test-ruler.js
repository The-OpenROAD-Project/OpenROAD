// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it, beforeEach } from 'node:test';
import assert from 'node:assert/strict';
import { formatDbu, formatDistance, parseDbu } from '../../src/ui-utils.js';

import './setup-dom.js';
import { makeFakeView } from './fake-view.js';

// Node.js doesn't have requestAnimationFrame.
globalThis.requestAnimationFrame = (cb) => setTimeout(cb, 0);

const { RulerManager } = await import('../../src/ruler.js');

// ─── Test helpers ────────────────────────────────────────────────────────────

function makeApp() {
    const requests = [];
    const app = {
        view: makeFakeView(),
        designScale: 1e-6,
        techData: { dbu_per_micron: 1000 },
        showDbu: false,
        visibleLayers: new Set(['metal1']),
        websocketManager: {
            request(msg) {
                requests.push(msg);
                return Promise.resolve({ found: false });
            },
        },
        inspectorEl: null,
        // Delegated to the real formatters, not reimplemented, so a change
        // to the display-unit rules cannot pass here and fail in the app.
        getDbuPerMicron() {
            return this.techData?.dbu_per_micron || 1000;
        },
        unitOpts() {
            return { showDbu: this.showDbu,
                     dbuPerMicron: this.getDbuPerMicron() };
        },
        formatDbu(value, addUnits = false) {
            return formatDbu(value, this.unitOpts(), addUnits);
        },
        parseDbu(str) {
            return parseDbu(str, this.unitOpts());
        },
        formatDistance(dbuLength) {
            return formatDistance(dbuLength, this.unitOpts());
        },
        _requests: requests,
    };
    return app;
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

describe('RulerManager default style (2.12)', () => {
    it('new rulers are euclidian by default', () => {
        const { mgr } = makeManager();
        const r = mgr._createRuler({ x: 0, y: 0 }, { x: 100, y: 200 });
        assert.equal(r.euclidian, true);
    });

    it('honors app.rulerStyle = manhattan for new rulers', () => {
        const { mgr } = makeManager({ rulerStyle: 'manhattan' });
        const r = mgr._createRuler({ x: 0, y: 0 }, { x: 100, y: 200 });
        assert.equal(r.euclidian, false);
    });
});

describe('RulerManager state machine', () => {
    it('starts in IDLE', () => {
        const { mgr } = makeManager();
        assert.equal(mgr.isActive(), false);
    });

    it('toggleRulerMode enters FIRST_POINT', () => {
        const { mgr, app } = makeManager();
        mgr.toggleRulerMode();
        assert.equal(mgr.isActive(), true);
        assert.ok(app.view.cursorModes.has('ruler'));
        assert.ok(app.view.getContainer().classList.contains('ruler-mode-cursor'));
    });

    it('toggleRulerMode again cancels back to IDLE', () => {
        const { mgr, app } = makeManager();
        mgr.toggleRulerMode();
        mgr.toggleRulerMode();
        assert.equal(mgr.isActive(), false);
        assert.ok(!app.view.cursorModes.has('ruler'));
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
        assert.equal(propMap['Point 0 - x'], '1.000 \u00b5m');
        assert.equal(propMap['Point 0 - y'], '2.000 \u00b5m');
        assert.equal(propMap['Point 1 - x'], '4.000 \u00b5m');
        assert.equal(propMap['Point 1 - y'], '6.000 \u00b5m');
        assert.equal(propMap['Delta x'], '3.000 \u00b5m');
        assert.equal(propMap['Delta y'], '4.000 \u00b5m');
        assert.equal(propMap['Length'], '5.000 \u00b5m');
        assert.equal(propMap['Euclidian'], 'True');
    });

    it('_selectRuler produces DBU values when showDbu is true', () => {
        const { mgr, app, inspectorData } = makeManager();
        app.showDbu = true;
        mgr._createRuler({ x: 1000, y: 2000 }, { x: 4000, y: 6000 });
        mgr._selectRuler(0);

        const data = inspectorData[0];
        const propMap = {};
        for (const p of data.properties) {
            propMap[p.name] = p.value;
        }
        assert.equal(propMap['Point 0 - x'], '1000');
        assert.equal(propMap['Point 0 - y'], '2000');
        assert.equal(propMap['Delta x'], '3000');
        assert.equal(propMap['Length'], '5000');
    });

    it('re-selecting ruler after toggling showDbu updates values', () => {
        const { mgr, app, inspectorData } = makeManager();
        mgr._createRuler({ x: 1000, y: 2000 }, { x: 4000, y: 6000 });
        mgr._selectRuler(0);

        // Initially micron mode
        const umProps = inspectorData.at(-1).properties;
        const umMap = {};
        for (const p of umProps) umMap[p.name] = p.value;
        assert.equal(umMap['Point 0 - x'], '1.000 \u00b5m');

        // Toggle to DBU and re-select
        app.showDbu = true;
        mgr._selectRuler(0);

        const dbuProps = inspectorData.at(-1).properties;
        const dbuMap = {};
        for (const p of dbuProps) dbuMap[p.name] = p.value;
        assert.equal(dbuMap['Point 0 - x'], '1000');
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

// ─── Drawing on the view ─────────────────────────────────────────────────────

// 256 world units over 1e6 DBU, so at zoom 0 the 1000x800 fake view shows the
// whole square and 1 DBU is 1/3906 px.
function makeDrawingManager(overrides = {}) {
    return makeManager({ designScale: 256 / 1e6, designOriginX: 0,
                         designOriginY: 0, ...overrides });
}

describe('Ruler drawing', () => {
    it('draws each ruler as a line plus two end ticks', () => {
        const { mgr, app } = makeDrawingManager();
        mgr._createRuler({ x: 0, y: 0 }, { x: 500000, y: 0 });
        const shapes = app.view.shapes('rulers');
        assert.equal(shapes.length, 3);
        assert.deepEqual(shapes[0].points, [[0, 0], [128, 0]]);
        assert.equal(shapes[0].width, 2);
    });

    it('colours the selected ruler yellow and the others cyan', () => {
        const { mgr, app } = makeDrawingManager();
        mgr._createRuler({ x: 0, y: 0 }, { x: 100000, y: 0 });
        mgr._createRuler({ x: 0, y: 200000 }, { x: 100000, y: 200000 });
        const lines = app.view.shapes('rulers').filter((s) => s.points.length >= 2
                                                       && s.points[0][1] === s.points[1][1]
                                                       && s.points[1][0] > 20);
        assert.deepEqual(lines[0].stroke.slice(0, 3), [0, 255, 255]);
        assert.deepEqual(lines[1].stroke.slice(0, 3), [255, 255, 0]);
    });

    it('draws a Manhattan ruler as an L through the join point', () => {
        const { mgr, app } = makeDrawingManager({ rulerStyle: 'manhattan' });
        mgr._createRuler({ x: 0, y: 0 }, { x: 500000, y: 250000 });
        const line = app.view.shapes('rulers')[0];
        assert.deepEqual(line.points, [[0, 0], [128, 0], [128, 64]]);
    });

    it('pins a distance label at the midpoint', () => {
        const { mgr, app } = makeDrawingManager();
        mgr._createRuler({ x: 0, y: 0 }, { x: 500000, y: 0 });
        const labels = app.view.markersIn('ruler-labels');
        assert.equal(labels.length, 1);
        assert.deepEqual(labels[0].world, [64, 0]);
        assert.ok(labels[0].el.classList.contains('ruler-label'));
        assert.equal(labels[0].el.textContent, app.formatDistance(500000));
    });

    it('keeps the end ticks 8 px long across a zoom', () => {
        const { mgr, app } = makeDrawingManager();
        mgr._createRuler({ x: 0, y: 0 }, { x: 500000, y: 0 });
        const tickPx = () => {
            const t = app.view.shapes('rulers')[1].points;
            const a = app.view.project(t[0]);
            const b = app.view.project(t[1]);
            return Math.hypot(a[0] - b[0], a[1] - b[1]);
        };
        assert.ok(Math.abs(tickPx() - 16) < 1e-6);
        app.view.setZoomForTest(3);
        assert.ok(Math.abs(tickPx() - 16) < 1e-6);
    });

    // The labels are world-anchored markers: a zoom frame redraws the ticks
    // only.
    it('leaves the label markers alone on a zoom', () => {
        const { mgr, app } = makeDrawingManager();
        mgr._createRuler({ x: 0, y: 0 }, { x: 500000, y: 0 });
        const label = app.view.markersIn('ruler-labels')[0].el;
        let markerCalls = 0;
        for (const fn of ['addMarker', 'clearMarkers']) {
            const orig = app.view[fn];
            app.view[fn] = (...args) => {
                markerCalls++;
                return orig(...args);
            };
        }
        app.view.setZoomForTest(3);
        assert.equal(markerCalls, 0);
        assert.equal(app.view.markersIn('ruler-labels')[0].el, label);
    });

    it('removes the shapes and labels of a deleted ruler', () => {
        const { mgr, app } = makeDrawingManager();
        mgr._createRuler({ x: 0, y: 0 }, { x: 500000, y: 0 });
        mgr.deleteRuler(0);
        assert.equal(app.view.hasOverlay('rulers'), false);
        assert.equal(app.view.markersIn('ruler-labels').length, 0);
    });

    it('hides everything when the Rulers visibility is off', () => {
        const { mgr, app } = makeDrawingManager();
        mgr._createRuler({ x: 0, y: 0 }, { x: 500000, y: 0 });
        mgr._visibility.rulers = false;
        mgr.updateVisibility();
        assert.equal(app.view.hasOverlay('rulers'), false);
        assert.equal(app.view.markersIn('ruler-labels').length, 0);
        mgr._visibility.rulers = true;
        mgr.updateVisibility();
        assert.equal(app.view.shapes('rulers').length, 3);
    });
});

describe('Ruler placement from view clicks', () => {
    it('two clicks in ruler mode create a ruler and stay in ruler mode', () => {
        const { mgr, app } = makeDrawingManager();
        mgr.toggleRulerMode();
        mgr._ctrlHeld = true;
        const e = { ctrlKey: true, metaKey: false, shiftKey: false };
        mgr.handleMapClick([0, 0], e);
        mgr.handleMapClick([128, 0], e);
        assert.equal(mgr._rulers.length, 1);
        assert.deepEqual(mgr._rulers[0].pt1, { x: 500000, y: 0 });
        assert.equal(mgr.isActive(), true);
        assert.equal(app.view.hasOverlay('ruler-preview'), false);
    });

    it('a click outside ruler mode is ignored', () => {
        const { mgr } = makeDrawingManager();
        mgr.handleMapClick([0, 0], { ctrlKey: false, shiftKey: false });
        assert.equal(mgr._pt0, null);
    });

    it('draws a dashed preview with a live distance label while measuring', () => {
        const { mgr, app } = makeDrawingManager();
        mgr.toggleRulerMode();
        const e = { ctrlKey: true, metaKey: false, shiftKey: false };
        mgr.handleMapClick([0, 0], e);
        mgr._updatePreview(250000, 0);
        const preview = app.view.shapes('ruler-preview');
        assert.equal(preview.length, 1);
        assert.deepEqual(preview[0].dash, [6, 4]);
        const label = app.view.markersIn('ruler-preview')[0];
        assert.equal(label.el.textContent, app.formatDistance(250000));
        mgr.cancelRulerBuild();
        assert.equal(app.view.hasOverlay('ruler-preview'), false);
        assert.equal(app.view.markersIn('ruler-preview').length, 0);
    });

    it('draws the snap indicator as a circle for a point snap', () => {
        const { mgr, app } = makeDrawingManager();
        mgr._updateSnapIndicator({ found: true, is_point: true,
                                   edge: [[500000, 0], [500000, 0]] });
        const snap = app.view.shapes('ruler-snap');
        assert.equal(snap[0].kind, 'circle');
        assert.deepEqual(snap[0].center, [128, 0]);
        mgr._clearSnap();
        assert.equal(app.view.hasOverlay('ruler-snap'), false);
    });
});

describe('Ruler hit test', () => {
    it('selects a ruler clicked within a few pixels of its line', () => {
        const { mgr, app, inspectorData } = makeDrawingManager();
        mgr._createRuler({ x: 0, y: 0 }, { x: 500000, y: 0 });
        mgr._createRuler({ x: 0, y: 500000 }, { x: 500000, y: 500000 });
        assert.equal(mgr._selectedRulerId, 1);
        const before = inspectorData.length;
        // 3 px above the first ruler's line.
        const p = app.view.project([64, 0]);
        const world = app.view.unproject([p[0], p[1] - 3]);
        assert.equal(mgr.handleRulerClick(world), true);
        assert.equal(mgr._selectedRulerId, 0);
        assert.equal(inspectorData.length, before + 1);
    });

    it('misses a click further away', () => {
        const { mgr, app } = makeDrawingManager();
        mgr._createRuler({ x: 0, y: 0 }, { x: 500000, y: 0 });
        const p = app.view.project([64, 0]);
        assert.equal(mgr.handleRulerClick(app.view.unproject([p[0], p[1] - 8])),
                     false);
    });

    it('does not pick hidden rulers', () => {
        const { mgr, app } = makeDrawingManager();
        mgr._createRuler({ x: 0, y: 0 }, { x: 500000, y: 0 });
        mgr._visibility.rulers = false;
        assert.equal(mgr.handleRulerClick([64, 0]), false);
    });
});
