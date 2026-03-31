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

import { dbuToLatLng, latLngToDbu } from './coordinates.js';

const RULER_COLOR = '#00ffff';
const RULER_SELECTED_COLOR = '#ffff00';
const SNAP_INDICATOR_COLOR = '#ffffff';

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

        // Modifier key state (updated on mousemove/click)
        this._ctrlHeld = false;
        this._shiftHeld = false;

        // Leaflet layers
        this._rulerPane = null;
        this._rulerLayerGroup = null;
        this._previewGroup = null;  // rubber-band during MEASURING
        this._snapGroup = null;     // snap indicator

        // Debounce state
        this._snapTimer = null;
        this._lastSnapResult = null;

        // Map event handlers (bound references for cleanup)
        this._onMouseMove = this._handleMouseMove.bind(this);
        this._onMapClick = this._handleMapClick.bind(this);

        this._initLayers();
    }

    _initLayers() {
        const map = this._app.map;
        if (!map) return;

        this._rulerPane = map.createPane('ruler-pane');
        this._rulerPane.style.zIndex = '700';
        this._rulerPane.style.pointerEvents = 'auto';

        this._rulerLayerGroup = L.layerGroup([], { pane: 'ruler-pane' }).addTo(map);
        this._previewGroup = L.layerGroup([], { pane: 'ruler-pane' }).addTo(map);
        this._snapGroup = L.layerGroup([], { pane: 'ruler-pane' }).addTo(map);
    }

    isActive() {
        return this._state !== IDLE;
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
        this._clearPreview();
        this._clearSnap();
        this._setCursor(false);
        this._removeMapListeners();
    }

    clearAllRulers() {
        this._rulers = [];
        this._selectedRulerId = null;
        this._rulerLayerGroup.clearLayers();
        this._clearPreview();
        this._clearSnap();
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
        this._addMapListeners();
    }

    _enterMeasuring(pt0) {
        this._state = MEASURING;
        this._pt0 = pt0;
    }

    // ─── Map event wiring ────────────────────────────────────────────

    _addMapListeners() {
        const map = this._app.map;
        if (!map) return;
        map.getContainer().addEventListener('mousemove', this._onMouseMove);
        map.on('click', this._onMapClick);
    }

    _removeMapListeners() {
        const map = this._app.map;
        if (!map) return;
        map.getContainer().removeEventListener('mousemove', this._onMouseMove);
        map.off('click', this._onMapClick);
    }

    _handleMouseMove(e) {
        if (this._state === IDLE) return;

        this._ctrlHeld = e.ctrlKey || e.metaKey;
        this._shiftHeld = e.shiftKey;

        const map = this._app.map;
        const rect = map.getContainer().getBoundingClientRect();
        const pt = map.containerPointToLatLng([
            e.clientX - rect.left, e.clientY - rect.top
        ]);
        const dbu = latLngToDbu(pt.lat, pt.lng,
            this._app.designScale, this._app.designMaxDXDY,
            this._app.designOriginX, this._app.designOriginY);

        // Debounce snap requests at ~50ms
        if (this._snapTimer) clearTimeout(this._snapTimer);
        this._snapTimer = setTimeout(() => {
            this._requestSnap(dbu.dbuX, dbu.dbuY);
        }, 50);
    }

    _handleMapClick(e) {
        if (this._state === IDLE) return;

        L.DomEvent.stopPropagation(e);

        // Read modifier keys from the native DOM event
        const orig = e.originalEvent;
        if (orig) {
            this._ctrlHeld = orig.ctrlKey || orig.metaKey;
            this._shiftHeld = orig.shiftKey;
        }

        const dbu = latLngToDbu(e.latlng.lat, e.latlng.lng,
            this._app.designScale, this._app.designMaxDXDY,
            this._app.designOriginX, this._app.designOriginY);

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
            this._clearPreview();
            this._clearSnap();
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
            this._clearSnap();
            this._updatePreview(dbuX, dbuY);
            return;
        }

        // Compute snap parameters
        const dbuPerMicron = app.techData?.dbu_per_micron || 1000;
        const radius = dbuPerMicron;  // 1 micron search radius

        // Point snap threshold: max(10 pixels in DBU, 10 DBU)
        const zoom = app.map.getZoom();
        const numTiles = Math.pow(2, Math.max(0, zoom));
        const dbuPerPixel = app.designMaxDXDY / (256 * numTiles);
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
            vf[k] = v ? 1 : 0;
        }

        try {
            const result = await app.websocketManager.request({
                type: 'snap',
                dbu_x: dbuX,
                dbu_y: dbuY,
                radius,
                point_threshold: pointThreshold,
                horizontal: horizontal ? 1 : 0,
                vertical: vertical ? 1 : 0,
                visible_layers: [...app.visibleLayers],
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
        this._clearSnap();
        if (!snap?.found) return;

        const scale = this._app.designScale;
        const maxDXDY = this._app.designMaxDXDY;
        const originX = this._app.designOriginX;
        const originY = this._app.designOriginY;

        if (snap.is_point) {
            const ll = dbuToLatLng(snap.edge[0][0], snap.edge[0][1], scale, maxDXDY, originX, originY);
            const marker = L.circleMarker(ll, {
                radius: 5,
                color: SNAP_INDICATOR_COLOR,
                fillColor: SNAP_INDICATOR_COLOR,
                fillOpacity: 0.5,
                weight: 1,
                pane: 'ruler-pane',
                interactive: false,
            });
            this._snapGroup.addLayer(marker);
        } else {
            const p1 = dbuToLatLng(snap.edge[0][0], snap.edge[0][1], scale, maxDXDY, originX, originY);
            const p2 = dbuToLatLng(snap.edge[1][0], snap.edge[1][1], scale, maxDXDY, originX, originY);
            const line = L.polyline([p1, p2], {
                color: SNAP_INDICATOR_COLOR,
                weight: 1,
                pane: 'ruler-pane',
                interactive: false,
            });
            this._snapGroup.addLayer(line);
        }
    }

    _updatePreview(rawX, rawY) {
        this._clearPreview();
        if (this._state !== MEASURING || !this._pt0) return;

        const scale = this._app.designScale;
        const maxDXDY = this._app.designMaxDXDY;
        const originX = this._app.designOriginX;
        const originY = this._app.designOriginY;
        const snapped = this._getSnappedPoint(rawX, rawY);

        const p0 = dbuToLatLng(this._pt0.x, this._pt0.y, scale, maxDXDY, originX, originY);
        const p1 = dbuToLatLng(snapped.x, snapped.y, scale, maxDXDY, originX, originY);

        const line = L.polyline([p0, p1], {
            color: RULER_COLOR,
            weight: 2,
            dashArray: '6,4',
            pane: 'ruler-pane',
            interactive: false,
        });
        this._previewGroup.addLayer(line);

        // Show distance label
        const dist = this._computeDistance(this._pt0, snapped);
        const midLat = (p0[0] + p1[0]) / 2;
        const midLng = (p0[1] + p1[1]) / 2;
        const label = L.marker([midLat, midLng], {
            icon: L.divIcon({
                className: 'ruler-label',
                html: this._formatDistance(dist),
                iconSize: null,
            }),
            pane: 'ruler-pane',
            interactive: false,
        });
        this._previewGroup.addLayer(label);
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
            euclidian: true,
        };
        this._rulers.push(ruler);
        this._renderRuler(ruler);
        return ruler;
    }

    _renderRuler(ruler) {
        const scale = this._app.designScale;
        const maxDXDY = this._app.designMaxDXDY;
        const originX = this._app.designOriginX;
        const originY = this._app.designOriginY;

        const p0 = dbuToLatLng(ruler.pt0.x, ruler.pt0.y, scale, maxDXDY, originX, originY);
        const p1 = dbuToLatLng(ruler.pt1.x, ruler.pt1.y, scale, maxDXDY, originX, originY);
        const isSelected = ruler.id === this._selectedRulerId;
        const color = isSelected ? RULER_SELECTED_COLOR : RULER_COLOR;

        const group = L.layerGroup([], { pane: 'ruler-pane' });

        // Main ruler line
        const mainLine = L.polyline([p0, p1], {
            color,
            weight: 2,
            pane: 'ruler-pane',
            interactive: true,
        });
        mainLine.on('click', (e) => {
            L.DomEvent.stopPropagation(e);
            this._selectRuler(ruler.id);
        });
        group.addLayer(mainLine);

        // Endcap ticks — short perpendicular lines
        const dx = ruler.pt1.x - ruler.pt0.x;
        const dy = ruler.pt1.y - ruler.pt0.y;
        const len = Math.sqrt(dx * dx + dy * dy);
        if (len > 0) {
            // Tick length in DBU: scale to ~8 pixels at current zoom
            const zoom = this._app.map.getZoom();
            const numTiles = Math.pow(2, Math.max(0, zoom));
            const dbuPerPixel = maxDXDY / (256 * numTiles);
            const tickLen = 8 * dbuPerPixel;

            // Unit perpendicular
            const px = -dy / len;
            const py = dx / len;

            for (const pt of [ruler.pt0, ruler.pt1]) {
                const t0 = dbuToLatLng(pt.x + px * tickLen, pt.y + py * tickLen, scale, maxDXDY, originX, originY);
                const t1 = dbuToLatLng(pt.x - px * tickLen, pt.y - py * tickLen, scale, maxDXDY, originX, originY);
                group.addLayer(L.polyline([t0, t1], {
                    color,
                    weight: 2,
                    pane: 'ruler-pane',
                    interactive: false,
                }));
            }
        }

        // Distance label at midpoint
        const dist = this._computeDistance(ruler.pt0, ruler.pt1, ruler.euclidian);
        const midLat = (p0[0] + p1[0]) / 2;
        const midLng = (p0[1] + p1[1]) / 2;
        const label = L.marker([midLat, midLng], {
            icon: L.divIcon({
                className: 'ruler-label',
                html: this._formatDistance(dist),
                iconSize: null,
            }),
            pane: 'ruler-pane',
            interactive: false,
        });
        group.addLayer(label);

        ruler._layerGroup = group;
        this._rulerLayerGroup.addLayer(group);
    }

    _rerenderAll() {
        this._rulerLayerGroup.clearLayers();
        for (const ruler of this._rulers) {
            this._renderRuler(ruler);
        }
    }

    // ─── Selection and inspector ─────────────────────────────────────

    _selectRuler(rulerId) {
        this._selectedRulerId = rulerId;
        this._rerenderAll();

        const ruler = this._rulers.find(r => r.id === rulerId);
        if (!ruler) return;

        const dbuPerUm = this._app.techData?.dbu_per_micron || 1000;
        const dx = Math.abs(ruler.pt1.x - ruler.pt0.x);
        const dy = Math.abs(ruler.pt1.y - ruler.pt0.y);
        const length = ruler.euclidian
            ? Math.sqrt(dx * dx + dy * dy)
            : dx + dy;

        const fmt = (dbu) => {
            const um = dbu / dbuPerUm;
            return um.toFixed(3) + ' um';
        };

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
                { name: 'Name', value: ruler.name },
                { name: 'Label', value: ruler.label || '' },
                { name: 'Point 0 - x', value: fmt(ruler.pt0.x) },
                { name: 'Point 0 - y', value: fmt(ruler.pt0.y) },
                { name: 'Point 1 - x', value: fmt(ruler.pt1.x) },
                { name: 'Point 1 - y', value: fmt(ruler.pt1.y) },
                { name: 'Delta x', value: fmt(dx) },
                { name: 'Delta y', value: fmt(dy) },
                { name: 'Length', value: fmt(length) },
                { name: 'Euclidian', value: ruler.euclidian ? 'true' : 'false' },
            ],
        };

        this._updateInspector(data);
        this._focusComponent('Inspector');

        // Add delete button after inspector renders
        requestAnimationFrame(() => {
            const toolbar = this._app.inspectorEl?.querySelector('.inspector-toolbar');
            if (!toolbar) return;

            const deleteBtn = document.createElement('button');
            deleteBtn.className = 'inspector-btn';
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

        const ruler = this._rulers[idx];
        if (ruler._layerGroup) {
            this._rulerLayerGroup.removeLayer(ruler._layerGroup);
        }
        this._rulers.splice(idx, 1);

        if (this._selectedRulerId === rulerId) {
            this._selectedRulerId = null;
            this._updateInspector(null);
        }
    }

    // ─── Helpers ─────────────────────────────────────────────────────

    _computeDistance(pt0, pt1, euclidian = true) {
        const dx = Math.abs(pt1.x - pt0.x);
        const dy = Math.abs(pt1.y - pt0.y);
        if (euclidian) {
            return Math.sqrt(dx * dx + dy * dy);
        }
        return dx + dy;
    }

    _formatDistance(dbuLength) {
        const dbuPerUm = this._app.techData?.dbu_per_micron || 1000;
        const um = dbuLength / dbuPerUm;
        if (um >= 1000) return (um / 1000).toFixed(3) + ' mm';
        if (um >= 1) return um.toFixed(3) + ' um';
        return (um * 1000).toFixed(1) + ' nm';
    }

    _setCursor(crosshair) {
        const container = this._app.map?.getContainer();
        if (!container) return;
        if (crosshair) {
            container.classList.add('ruler-mode-cursor');
        } else {
            container.classList.remove('ruler-mode-cursor');
        }
    }

    _clearPreview() {
        this._previewGroup?.clearLayers();
    }

    _clearSnap() {
        this._snapGroup?.clearLayers();
    }
}
