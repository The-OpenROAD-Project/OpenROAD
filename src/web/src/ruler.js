// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Ruler tool — interactive measurement with snapping.
//
// State machine: IDLE → FIRST_POINT → MEASURING → IDLE
//   'k'      : toggle between IDLE ↔ FIRST_POINT
//   click    : in FIRST_POINT → capture pt0, enter MEASURING
//              in MEASURING  → capture pt1, create ruler, enter IDLE
//   Escape   : cancel → IDLE
//   Shift+K  : clear all rulers

import { dbuToWorld, worldToDbu } from './coordinates.js';
import { DRAW_ORDER, toRgba } from './layout-view.js';
import { beginSelection, onSelectionReset } from './ui-utils.js';

const RULER_COLOR = '#00ffff';
const RULER_SELECTED_COLOR = '#ffff00';
const SNAP_INDICATOR_COLOR = '#ffffff';

// Screen distance within which a click picks a ruler line.
const kRulerHitPx = 4;

const IDLE = 0;
const FIRST_POINT = 1;
const MEASURING = 2;

export class RulerManager {
    constructor(app, visibility, updateInspector, focusComponent) {
        this._app = app;
        this._visibility = visibility;
        this._updateInspector = updateInspector;
        this._focusComponent = focusComponent;

        this._state = IDLE;
        this._rulers = [];
        this._nextId = 0;
        this._pt0 = null;           // {x, y} in DBU
        this._selectedRulerId = null;
        // Another object taking the selection unmarks the ruler.
        onSelectionReset(app, () => {
            if (this._selectedRulerId !== null) {
                this._selectedRulerId = null;
                this._renderAll();
            }
        });

        // Modifier key state (updated on mousemove/click)
        this._ctrlHeld = false;
        this._shiftHeld = false;

        // Debounce state
        this._snapTimer = null;
        this._lastSnapResult = null;
        this._snapShapes = null;
        this._preview = null;      // { shapes, label: { world, text } }

        // Tick lengths are in screen pixels, so a zoom redraws the rulers;
        // the labels are world-anchored markers and follow on their own.
        app.view?.on('zoom', () => this._renderRulerShapes());
    }

    isActive() {
        return this._state !== IDLE;
    }

    updateVisibility() {
        this._renderAll();
        this._drawTransient();
    }

    _shown() {
        return this._visibility.rulers !== false;
    }

    toggleRulerMode() {
        if (this._state === IDLE) {
            this._enterFirstPoint();
        } else {
            this.cancelRulerBuild();
        }
    }

    cancelRulerBuild() {
        this._state = IDLE;
        this._pt0 = null;
        this._lastSnapResult = null;
        this._clearTransient();
        this._setCursor(false);
    }

    clearAllRulers() {
        this._rulers = [];
        this._selectedRulerId = null;
        this._renderAll();
        this._clearTransient();
        if (this._state !== IDLE) {
            this.cancelRulerBuild();
        }
    }

    // ─── State transitions ───────────────────────────────────────────

    _enterFirstPoint() {
        this._state = FIRST_POINT;
        this._pt0 = null;
        this._lastSnapResult = null;
        this._setCursor(true);
    }

    _enterMeasuring(pt0) {
        this._state = MEASURING;
        this._pt0 = pt0;
    }

    // ─── Pointer input (from view-input via main.js) ─────────────────

    _toDbu(world) {
        return worldToDbu(world[0], world[1], this._app.designScale,
                          this._app.designOriginX, this._app.designOriginY);
    }

    _toWorld(x, y) {
        return dbuToWorld(x, y, this._app.designScale,
                          this._app.designOriginX, this._app.designOriginY);
    }

    handleMouseMove(world, e) {
        if (this._state === IDLE) return;

        this._ctrlHeld = e.ctrlKey || e.metaKey;
        this._shiftHeld = e.shiftKey;

        const dbu = this._toDbu(world);

        // Debounce snap requests at ~50ms
        if (this._snapTimer) clearTimeout(this._snapTimer);
        this._snapTimer = setTimeout(() => {
            this._requestSnap(dbu.dbuX, dbu.dbuY);
        }, 50);
    }

    handleMapClick(world, e) {
        if (this._state === IDLE) return;

        this._ctrlHeld = e.ctrlKey || e.metaKey;
        this._shiftHeld = e.shiftKey;

        const dbu = this._toDbu(world);

        // Use the last snap result if available, otherwise raw click
        const snapped = this._getSnappedPoint(dbu.dbuX, dbu.dbuY);

        if (this._state === FIRST_POINT) {
            this._enterMeasuring(snapped);
        } else if (this._state === MEASURING) {
            this._createRuler(this._pt0, snapped);
            // Stay in FIRST_POINT mode for chaining rulers (match GUI behavior
            // where you can keep placing rulers until pressing 'k' or Escape)
            this._state = FIRST_POINT;
            this._pt0 = null;
            this._clearTransient();
            this._lastSnapResult = null;
        }
    }

    // ─── Snap ────────────────────────────────────────────────────────

    async _requestSnap(dbuX, dbuY) {
        const app = this._app;
        if (!app.websocketManager || !app.designScale) return;

        // Ctrl held: skip snap entirely, use raw cursor position
        if (this._ctrlHeld) {
            this._lastSnapResult = null;
            this._snapShapes = null;
            this._updatePreview(dbuX, dbuY);
            return;
        }

        // Compute snap parameters
        const dbuPerMicron = app.techData?.dbu_per_micron || 1000;
        const radius = dbuPerMicron;  // 1 micron search radius

        // Point snap threshold: max(10 pixels in DBU, 10 DBU)
        const zoom = app.view.getZoom();
        const numTiles = Math.pow(2, Math.max(0, zoom));
        // designScale is CSS px per DBU at zoom 0, so it already carries the
        // tile size the map was built with -- which is not a constant.
        const dbuPerPixel = 1 / (app.designScale * numTiles);
        const pointThreshold = Math.max(Math.round(10 * dbuPerPixel), 10);

        // Direction constraint when measuring
        let horizontal = true;
        let vertical = true;
        if (this._shiftHeld) {
            // Shift: snap to all edges (no direction constraint)
            horizontal = true;
            vertical = true;
        } else if (this._state === MEASURING && this._pt0) {
            // Default: constrain to perpendicular edges based on ruler direction
            const dx = Math.abs(dbuX - this._pt0.x);
            const dy = Math.abs(dbuY - this._pt0.y);
            if (dx > dy) {
                // Mostly horizontal ruler → snap to vertical edges
                horizontal = false;
                vertical = true;
            } else if (dy > dx) {
                // Mostly vertical ruler → snap to horizontal edges
                horizontal = true;
                vertical = false;
            }
        }

        const vf = {};
        for (const [k, v] of Object.entries(this._visibility)) {
            vf[k] = !!v;
        }

        try {
            const result = await app.websocketManager.request({
                type: 'snap',
                dbu_x: dbuX,
                dbu_y: dbuY,
                radius,
                point_threshold: pointThreshold,
                horizontal: !!horizontal,
                vertical: !!vertical,
                visible_layers: [...app.visibleLayerNames],
                ...vf,
            });

            this._lastSnapResult = result;
            this._updateSnapIndicator(result);
            this._updatePreview(dbuX, dbuY);
        } catch (err) {
            // Snap failed — just clear indicator
            this._lastSnapResult = null;
            this._clearSnap();
        }
    }

    _getSnappedPoint(rawX, rawY) {
        // Ctrl: no snapping at all, use raw cursor position
        if (this._ctrlHeld) {
            return { x: rawX, y: rawY };
        }

        const snap = this._lastSnapResult;
        let result;

        if (snap?.found && snap.is_point) {
            result = { x: snap.edge[0][0], y: snap.edge[0][1] };
        } else if (snap?.found) {
            // Edge snap: snap the constrained coordinate
            const edge = snap.edge;
            const isHorizontalEdge = edge[0][1] === edge[1][1];
            if (isHorizontalEdge) {
                result = { x: rawX, y: edge[0][1] };
            } else {
                result = { x: edge[0][0], y: rawY };
            }
        } else {
            result = { x: rawX, y: rawY };
        }

        // Default (no Shift): apply axis lock to force perfectly H/V ruler
        if (!this._shiftHeld && this._state === MEASURING && this._pt0) {
            const dx = Math.abs(result.x - this._pt0.x);
            const dy = Math.abs(result.y - this._pt0.y);
            if (dx < dy) {
                // Mostly vertical → lock X to pt0
                result.x = this._pt0.x;
            } else {
                // Mostly horizontal → lock Y to pt0
                result.y = this._pt0.y;
            }
        }

        return result;
    }

    _updateSnapIndicator(snap) {
        this._snapShapes = null;
        if (snap?.found) {
            const white = toRgba(SNAP_INDICATOR_COLOR);
            if (snap.is_point) {
                this._snapShapes = [{
                    kind: 'circle',
                    center: this._toWorld(snap.edge[0][0], snap.edge[0][1]),
                    radius: 5, stroke: white, width: 1,
                    fill: toRgba(SNAP_INDICATOR_COLOR, 0.5),
                }];
            } else {
                this._snapShapes = [{
                    kind: 'path',
                    points: [this._toWorld(snap.edge[0][0], snap.edge[0][1]),
                             this._toWorld(snap.edge[1][0], snap.edge[1][1])],
                    stroke: white, width: 1,
                }];
            }
        }
        this._drawTransient();
    }

    _updatePreview(rawX, rawY) {
        this._preview = null;
        if (this._state === MEASURING && this._pt0) {
            const snapped = this._getSnappedPoint(rawX, rawY);
            const p0 = this._toWorld(this._pt0.x, this._pt0.y);
            const p1 = this._toWorld(snapped.x, snapped.y);
            const dist = this._computeDistance(this._pt0, snapped);
            this._preview = {
                shapes: [{ kind: 'path', points: [p0, p1],
                           stroke: toRgba(RULER_COLOR), width: 2,
                           dash: [6, 4] }],
                label: { world: [(p0[0] + p1[0]) / 2, (p0[1] + p1[1]) / 2],
                         text: this._formatDistance(dist) },
            };
        }
        this._drawTransient();
    }

    // The snap indicator and the rubber-band preview.
    _drawTransient() {
        const view = this._app.view;
        if (!view) return;
        const shown = this._shown();
        view.setOverlay('ruler-snap', shown ? this._snapShapes : null,
                        { order: DRAW_ORDER.ruler });
        view.setOverlay('ruler-preview', shown && this._preview
            ? this._preview.shapes : null, { order: DRAW_ORDER.ruler });
        view.clearMarkers('ruler-preview');
        if (shown && this._preview) {
            view.addMarker('ruler-preview', 'label', this._preview.label.world,
                           this._labelElement(this._preview.label.text));
        }
    }

    _labelElement(text) {
        const el = document.createElement('div');
        el.className = 'ruler-label';
        el.textContent = text;
        return el;
    }

    // ─── Ruler creation and rendering ────────────────────────────────

    _createRuler(pt0, pt1) {
        const id = this._nextId++;
        const ruler = {
            id,
            pt0: { ...pt0 },
            pt1: { ...pt1 },
            name: `ruler${id}`,
            label: '',
            // Global default style for new rulers (2.12); per-ruler editable.
            euclidian: this._app.rulerStyle !== 'manhattan',
        };
        this._rulers.push(ruler);
        this._selectRuler(id);
        return ruler;
    }

    // World points of a ruler's path: straight, or the Manhattan L.
    _rulerPath(ruler) {
        const joinPt = ruler.euclidian
            ? null : this._getManhattanJoinPt(ruler.pt0, ruler.pt1);
        const pts = joinPt ? [ruler.pt0, joinPt, ruler.pt1]
            : [ruler.pt0, ruler.pt1];
        return { joinPt, points: pts.map((p) => this._toWorld(p.x, p.y)) };
    }

    _rulerShapes(ruler) {
        const isSelected = ruler.id === this._selectedRulerId;
        const color = toRgba(isSelected ? RULER_SELECTED_COLOR : RULER_COLOR);
        const { joinPt, points } = this._rulerPath(ruler);
        const shapes = [{ kind: 'path', points, stroke: color, width: 2 }];

        // Endcap ticks, 8 px either side, perpendicular to each segment.
        const zoom = this._app.view.getZoom();
        const numTiles = Math.pow(2, Math.max(0, zoom));
        const dbuPerPixel = 1 / (this._app.designScale * numTiles);
        const tickLen = 8 * dbuPerPixel;
        const addTick = (pt, segDx, segDy) => {
            const segLen = Math.sqrt(segDx * segDx + segDy * segDy);
            if (segLen === 0) return;
            const px = -segDy / segLen;
            const py = segDx / segLen;
            shapes.push({ kind: 'path', stroke: color, width: 2, points: [
                this._toWorld(pt.x + px * tickLen, pt.y + py * tickLen),
                this._toWorld(pt.x - px * tickLen, pt.y - py * tickLen)] });
        };
        if (joinPt) {
            addTick(ruler.pt0, joinPt.x - ruler.pt0.x, joinPt.y - ruler.pt0.y);
            addTick(ruler.pt1, ruler.pt1.x - joinPt.x, ruler.pt1.y - joinPt.y);
        } else {
            const dx = ruler.pt1.x - ruler.pt0.x;
            const dy = ruler.pt1.y - ruler.pt0.y;
            addTick(ruler.pt0, dx, dy);
            addTick(ruler.pt1, dx, dy);
        }
        return shapes;
    }

    _renderAll() {
        this._renderRulerShapes();
        this._renderRulerLabels();
    }

    _renderRulerShapes() {
        const view = this._app.view;
        if (!view || !this._app.designScale) return;
        view.setOverlay('rulers', this._shown()
            ? this._rulers.flatMap((r) => this._rulerShapes(r)) : null,
                        { order: DRAW_ORDER.ruler });
    }

    _renderRulerLabels() {
        const view = this._app.view;
        if (!view || !this._app.designScale) return;
        view.clearMarkers('ruler-labels');
        if (!this._shown()) return;
        // Distance label at the midpoint, or at the join for Manhattan.
        for (const ruler of this._rulers) {
            const { joinPt, points } = this._rulerPath(ruler);
            const at = joinPt ? points[1]
                : [(points[0][0] + points[1][0]) / 2,
                   (points[0][1] + points[1][1]) / 2];
            const dist = this._computeDistance(ruler.pt0, ruler.pt1,
                                               ruler.euclidian);
            view.addMarker('ruler-labels', ruler.id, at,
                           this._labelElement(this._formatDistance(dist)));
        }
    }

    // Select the ruler whose line passes within a few pixels of `world`.
    // Returns true when one did, so the click is not also a selection.
    handleRulerClick(world) {
        const view = this._app.view;
        if (!view || !this._shown() || this._rulers.length === 0) {
            return false;
        }
        const p = view.project(world);
        for (const ruler of this._rulers) {
            const pts = this._rulerPath(ruler).points.map((w) => view.project(w));
            for (let i = 0; i + 1 < pts.length; i++) {
                if (segmentDistance(p, pts[i], pts[i + 1]) <= kRulerHitPx) {
                    this._selectRuler(ruler.id);
                    return true;
                }
            }
        }
        return false;
    }

    // ─── Selection and inspector ─────────────────────────────────────

    _selectRuler(rulerId) {
        // A client-side object that still takes the selection from the other
        // panels; ours is dropped first, so their reset does not redraw it.
        this._selectedRulerId = null;
        beginSelection(this._app);
        this._selectedRulerId = rulerId;
        this._renderAll();
        this._inspectRuler(rulerId, { focus: true });
    }

    // Shows the selected ruler in the Inspector again without taking the
    // selection.
    refreshInspector() {
        if (this._selectedRulerId !== null) {
            this._inspectRuler(this._selectedRulerId, { focus: false });
        }
    }

    _inspectRuler(rulerId, { focus }) {
        const ruler = this._rulers.find(r => r.id === rulerId);
        if (!ruler) return;

        const dx = Math.abs(ruler.pt1.x - ruler.pt0.x);
        const dy = Math.abs(ruler.pt1.y - ruler.pt0.y);
        const length = ruler.euclidian
            ? Math.sqrt(dx * dx + dy * dy)
            : dx + dy;

        const fmt = (dbu) => this._app.formatDbu(dbu, true);
        const parseDbu = (str) => this._app.parseDbu(str);

        const data = {
            type: 'Ruler',
            name: ruler.name,
            bbox: [
                Math.min(ruler.pt0.x, ruler.pt1.x),
                Math.min(ruler.pt0.y, ruler.pt1.y),
                Math.max(ruler.pt0.x, ruler.pt1.x),
                Math.max(ruler.pt0.y, ruler.pt1.y),
            ],
            properties: [
                { name: 'Name', value: ruler.name, editable: true },
                { name: 'Label', value: ruler.label || '', editable: true },
                { name: 'Point 0 - x', value: fmt(ruler.pt0.x), editable: true },
                { name: 'Point 0 - y', value: fmt(ruler.pt0.y), editable: true },
                { name: 'Point 1 - x', value: fmt(ruler.pt1.x), editable: true },
                { name: 'Point 1 - y', value: fmt(ruler.pt1.y), editable: true },
                { name: 'Delta x', value: fmt(dx) },
                { name: 'Delta y', value: fmt(dy) },
                { name: 'Length', value: fmt(length) },
                // 'True'/'False' to match the bool editor's option labels, so
                // the select opens on the current value instead of showing it
                // as an unrecognized placeholder.
                { name: 'Euclidian', value: ruler.euclidian ? 'True' : 'False',
                  editable: true, editor: { type: 'bool' } },
            ],
            onPropertyChange: (propName, newValue) => {
                switch (propName) {
                    case 'Name': ruler.name = newValue; break;
                    case 'Label': ruler.label = newValue; break;
                    case 'Point 0 - x': {
                        const v = parseDbu(newValue);
                        if (v !== null) ruler.pt0.x = v;
                        break;
                    }
                    case 'Point 0 - y': {
                        const v = parseDbu(newValue);
                        if (v !== null) ruler.pt0.y = v;
                        break;
                    }
                    case 'Point 1 - x': {
                        const v = parseDbu(newValue);
                        if (v !== null) ruler.pt1.x = v;
                        break;
                    }
                    case 'Point 1 - y': {
                        const v = parseDbu(newValue);
                        if (v !== null) ruler.pt1.y = v;
                        break;
                    }
                    case 'Euclidian':
                        ruler.euclidian = newValue.trim().toLowerCase() === 'true';
                        break;
                }
                this._renderAll();
                this._selectRuler(ruler.id);
            },
        };

        this._updateInspector(data);
        if (focus) {
            this._focusComponent('Inspector');
        }

        // Add delete button after inspector renders
        requestAnimationFrame(() => {
            const toolbar = this._app.inspectorEl?.querySelector('.inspector-toolbar');
            if (!toolbar) return;

            const deleteBtn = document.createElement('button');
            deleteBtn.className = 'or-btn or-btn-icon';
            deleteBtn.title = 'Delete ruler';
            deleteBtn.innerHTML =
                '<svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor">' +
                '<path d="M6 19c0 1.1.9 2 2 2h8c1.1 0 2-.9 2-2V7H6v12zM19 4h-3.5l-1-1h-5l-1 1H5v2h14V4z"/>' +
                '</svg>';
            deleteBtn.addEventListener('click', () => {
                this.deleteRuler(rulerId);
            });
            toolbar.appendChild(deleteBtn);
        });
    }

    deleteRuler(rulerId) {
        const idx = this._rulers.findIndex(r => r.id === rulerId);
        if (idx === -1) return;

        this._rulers.splice(idx, 1);
        this._renderAll();

        if (this._selectedRulerId === rulerId) {
            this._selectedRulerId = null;
            this._updateInspector(null);
        }
    }

    // ─── Helpers ─────────────────────────────────────────────────────

    // Manhattan join point: corner of the L-shaped path.
    // Matches Qt GUI's Ruler::getManhattanJoinPt().
    _getManhattanJoinPt(pt0, pt1) {
        return { x: pt1.x, y: pt0.y };
    }

    _computeDistance(pt0, pt1, euclidian = true) {
        const dx = Math.abs(pt1.x - pt0.x);
        const dy = Math.abs(pt1.y - pt0.y);
        if (euclidian) {
            return Math.sqrt(dx * dx + dy * dy);
        }
        return dx + dy;
    }

    _formatDistance(dbuLength) {
        return this._app.formatDistance(dbuLength);
    }

    _setCursor(crosshair) {
        this._app.view?.setCursorMode('ruler', crosshair);
    }

    _clearTransient() {
        this._preview = null;
        this._snapShapes = null;
        this._drawTransient();
    }

    _clearSnap() {
        this._snapShapes = null;
        this._drawTransient();
    }
}

// Distance in px from point p to segment ab.
function segmentDistance(p, a, b) {
    const dx = b[0] - a[0];
    const dy = b[1] - a[1];
    const len2 = dx * dx + dy * dy;
    let t = len2 > 0 ? ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / len2 : 0;
    t = Math.max(0, Math.min(1, t));
    return Math.hypot(p[0] - (a[0] + t * dx), p[1] - (a[1] + t * dy));
}
