// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { getThemeColors } from './theme.js';
import { isStaticMode } from './ui-utils.js';

export const MIN_WIRE_WIDTH = 0.02;
export const MIN_WIRE_HEIGHT = 0.02;
export const MIN_WIRE_Z = 0.02;
export const DEFAULT_EPS = 3.9;
export const DEFAULT_BINS = 16;

const HANDLE_HIT_PX = 7;
const SOLVE_DEBOUNCE_MS = 45;

/**
 * Returns a fresh default 3-wire cross-section: a centered target wire at x=0
 * flanked by two symmetric grounded neighbor wires.
 */
export function createDefaultWires() {
    return [
        { id: 0, name: 'target', x: 0.0, z: 0.4, width: 0.2, height: 0.2, isTarget: true },
        { id: 1, name: 'w1', x: -0.4, z: 0.4, width: 0.2, height: 0.2, isTarget: false },
        { id: 2, name: 'w2', x: 0.4, z: 0.4, width: 0.2, height: 0.2, isTarget: false },
    ];
}

/**
 * Returns true if two wire rectangles overlap or touch within `margin` µm.
 */
export function wiresOverlap(a, b, margin = 1e-6) {
    const ax0 = a.x - a.width / 2;
    const ax1 = a.x + a.width / 2;
    const az0 = a.z;
    const az1 = a.z + a.height;
    const bx0 = b.x - b.width / 2;
    const bx1 = b.x + b.width / 2;
    const bz0 = b.z;
    const bz1 = b.z + b.height;
    return ax0 < bx1 + margin && ax1 > bx0 - margin
        && az0 < bz1 + margin && az1 > bz0 - margin;
}

/**
 * Returns true if any pair of wires in the list overlaps or touches.
 */
export function hasAnyOverlap(wires, margin = 1e-6) {
    for (let i = 0; i < wires.length; i++) {
        for (let j = i + 1; j < wires.length; j++) {
            if (wiresOverlap(wires[i], wires[j], margin)) {
                return true;
            }
        }
    }
    return false;
}

/**
 * Returns the minimum allowed width for `wire`, respecting the ITF layer's
 * `min_width` when an ITF stack is loaded.
 */
export function minWidthForWire(wire, itf) {
    if (itf && wire && wire.layer && Array.isArray(itf.conductors)) {
        const layer = itf.conductors.find(c => c.name === wire.layer);
        if (layer && layer.min_width > 0) {
            return Math.max(MIN_WIRE_WIDTH, roundCoord(layer.min_width));
        }
    }
    return MIN_WIRE_WIDTH;
}

/**
 * Returns the minimum horizontal spacing between two wires `a` and `b`.
 * When both sit on the same ITF layer with `min_spacing > 0`, returns that
 * spacing; otherwise returns a tiny positive clearance (non-overlap).
 */
export function minSpacingBetween(a, b, itf) {
    if (itf && a.layer && a.layer === b.layer && Array.isArray(itf.conductors)) {
        const layer = itf.conductors.find(c => c.name === a.layer);
        if (layer && layer.min_spacing > 0) {
            return layer.min_spacing;
        }
    }
    return 1e-6;
}

/**
 * Returns true if any pair of wires overlaps or violates same-layer ITF
 * `min_spacing`.
 */
export function hasSpacingViolation(wires, itf) {
    for (let i = 0; i < wires.length; i++) {
        for (let j = i + 1; j < wires.length; j++) {
            const a = wires[i];
            const b = wires[j];
            if (wiresOverlap(a, b, 1e-6)) {
                return true;
            }
            const spacing = minSpacingBetween(a, b, itf);
            if (spacing > 1e-6) {
                const az0 = a.z;
                const az1 = a.z + a.height;
                const bz0 = b.z;
                const bz1 = b.z + b.height;
                const zOverlap = az0 < bz1 && az1 > bz0;
                if (zOverlap) {
                    const gapX = Math.max(
                        (b.x - b.width / 2) - (a.x + a.width / 2),
                        (a.x - a.width / 2) - (b.x + b.width / 2));
                    if (gapX < spacing - 1e-6) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

/**
 * Resizes a wire from its left or right edge.
 * For the target wire, resizing is always symmetric around x = 0 so it stays
 * centered.
 */
export function resizeWireEdge(wire, edge, worldX, minWidth = MIN_WIRE_WIDTH) {
    if (wire.isTarget) {
        const half = Math.max(minWidth / 2, Math.abs(worldX));
        wire.width = roundCoord(2 * half);
        wire.x = 0.0;
        return;
    }
    const x0 = wire.x - wire.width / 2;
    const x1 = wire.x + wire.width / 2;
    if (edge === 'left') {
        const newX0 = Math.min(x1 - minWidth, worldX);
        wire.width = roundCoord(x1 - newX0);
        wire.x = roundCoord((newX0 + x1) / 2);
    } else {
        const newX1 = Math.max(x0 + minWidth, worldX);
        wire.width = roundCoord(newX1 - x0);
        wire.x = roundCoord((x0 + newX1) / 2);
    }
}

/**
 * Finds a non-overlapping placement for a newly added neighbor wire.
 */
export function findNewWirePlacement(wires, width = 0.2, height = 0.2, baseZ = 0.4) {
    const stepX = width + 0.2;
    const candidates = [];
    for (let k = 1; k <= 12; k++) {
        candidates.push({ x: roundCoord(k * stepX), z: baseZ });
        candidates.push({ x: roundCoord(-k * stepX), z: baseZ });
    }
    for (let row = 1; row <= 6; row++) {
        const z = roundCoord(baseZ + row * (height + 0.2));
        candidates.push({ x: 0.0, z });
        for (let k = 1; k <= 6; k++) {
            candidates.push({ x: roundCoord(k * stepX), z });
            candidates.push({ x: roundCoord(-k * stepX), z });
        }
    }
    for (const pos of candidates) {
        const probe = { x: pos.x, z: pos.z, width, height };
        if (!wires.some(w => wiresOverlap(w, probe, 0.01))) {
            return pos;
        }
    }
    return { x: roundCoord((wires.length + 1) * stepX), z: baseZ };
}

function lerpStops(stops, t) {
    const clamped = Math.max(0, Math.min(1, t));
    for (let i = 0; i < stops.length - 1; i++) {
        const a = stops[i];
        const b = stops[i + 1];
        if (clamped <= b[0]) {
            const u = (clamped - a[0]) / Math.max(1e-9, b[0] - a[0]);
            return [
                Math.round(a[1] + u * (b[1] - a[1])),
                Math.round(a[2] + u * (b[2] - a[2])),
                Math.round(a[3] + u * (b[3] - a[3])),
            ];
        }
    }
    const last = stops[stops.length - 1];
    return [last[1], last[2], last[3]];
}

const POS_CHARGE_STOPS = [
    [0.00, 82, 98, 112],    // muted slate at 0
    [0.22, 185, 58, 20],    // deep rust-orange
    [0.52, 255, 118, 0],    // vivid orange
    [0.80, 255, 212, 24],   // bright gold-yellow
    [1.00, 255, 250, 205],  // hot yellow-white at peak corners
];

const NEG_CHARGE_STOPS = [
    [0.00, 82, 98, 112],    // muted slate at 0
    [0.22, 26, 78, 165],    // deep cobalt
    [0.52, 24, 156, 255],   // vivid electric blue
    [0.80, 72, 238, 255],   // bright cyan
    [1.00, 218, 252, 255],  // ice cyan-white at peak corners
];

/**
 * Maps a signed surface charge density `val` to an RGB color string given the
 * maximum magnitude `maxAbs`. Positive charges ramp through deep orange,
 * vivid orange, gold, and hot yellow-white; negative induced charges ramp
 * through cobalt, electric blue, bright cyan, and ice-white.
 */
export function chargeToColor(val, maxAbs, alpha = 1.0) {
    if (!Number.isFinite(val) || !Number.isFinite(maxAbs) || maxAbs <= 0) {
        return `rgba(82, 98, 112, ${alpha})`;
    }
    const t = Math.min(1, Math.abs(val) / maxAbs);
    const [r, g, b] = lerpStops(val >= 0 ? POS_CHARGE_STOPS : NEG_CHARGE_STOPS, t);
    return alpha >= 1 ? `rgb(${r}, ${g}, ${b})` : `rgba(${r}, ${g}, ${b}, ${alpha})`;
}

export function roundCoord(v) {
    return Math.round(v * 10000) / 10000;
}

export function formatCap(val, sigma) {
    if (!Number.isFinite(val)) return '—';
    const main = val.toFixed(4);
    if (Number.isFinite(sigma) && sigma > 0) {
        return `${main} ± ${sigma.toFixed(4)}`;
    }
    return main;
}

/**
 * Cleans Monte Carlo bin noise on one straight wire face while preserving
 * physical corner singularities:
 *  - Enforces the electrostatic maximum principle (target >= 0, neighbors <= 0).
 *  - Suppresses isolated single-bin high-weight FRW spikes in the interior.
 *  - Applies a light [0.2, 0.6, 0.2] kernel on interior bins only.
 */
export function smoothChargeFace(vals, isTarget) {
    if (!Array.isArray(vals) || vals.length === 0) return [];
    const n = vals.length;
    const clamped = new Array(n);
    for (let i = 0; i < n; i++) {
        const v = Number.isFinite(vals[i]) ? vals[i] : 0;
        clamped[i] = isTarget ? Math.max(0, v) : Math.min(0, v);
    }
    if (n < 3) return clamped;

    // Despike isolated single-bin FRW outliers.
    const despiked = clamped.slice();
    for (let i = 1; i < n - 1; i++) {
        const nbAvg = 0.5 * (clamped[i - 1] + clamped[i + 1]);
        if (Math.abs(clamped[i]) > 1.8 * Math.abs(nbAvg) + 1e-6) {
            despiked[i] = nbAvg;
        }
    }
    // Guard against an extreme single-bin outlier on a corner bin while still
    // allowing strong physical corner peaking (up to 4x its reference scale).
    let interiorMean = 0;
    for (let i = 1; i < n - 1; i++) interiorMean += Math.abs(despiked[i]);
    interiorMean /= Math.max(1, n - 2);
    const leftRef = Math.max(Math.abs(despiked[1]), interiorMean);
    const rightRef = Math.max(Math.abs(despiked[n - 2]), interiorMean);
    if (leftRef > 0 && Math.abs(despiked[0]) > 6.0 * leftRef + 1e-6) {
        despiked[0] = Math.sign(despiked[0]) * 6.0 * leftRef;
    }
    if (rightRef > 0 && Math.abs(despiked[n - 1]) > 6.0 * rightRef + 1e-6) {
        despiked[n - 1] = Math.sign(despiked[n - 1]) * 6.0 * rightRef;
    }

    // Light interior smoothing; endpoints stay crisp.
    const out = despiked.slice();
    for (let i = 1; i < n - 1; i++) {
        out[i] = 0.2 * despiked[i - 1] + 0.6 * despiked[i] + 0.2 * despiked[i + 1];
    }
    return out;
}

/**
 * Picks a sensible default conductor layer from an ITF stack (prefers the
 * lowest metal layer such as met1 / M1, otherwise the first conductor layer).
 */
export function pickDefaultItfLayer(conductors) {
    if (!Array.isArray(conductors) || conductors.length === 0) return null;
    const metal = conductors.find(c => /^m(et(al)?)?\d/i.test(c.name));
    return metal || conductors[0];
}

/**
 * Finds the ITF conductor layer whose vertical midpoint is closest to `zMid`.
 */
export function nearestItfLayer(conductors, zMid) {
    if (!Array.isArray(conductors) || conductors.length === 0) return null;
    let best = conductors[0];
    let bestDist = Infinity;
    for (const c of conductors) {
        const mid = 0.5 * (c.bottom + c.top);
        const d = Math.abs(zMid - mid);
        if (d < bestDist) {
            bestDist = d;
            best = c;
        }
    }
    return best;
}

/**
 * Snaps a wire's z and height to an ITF conductor layer without rounding,
 * preserving exact stack coordinates so conformal dielectrics match.
 */
export function snapWireToLayer(wire, layer) {
    if (!layer) return;
    wire.layer = layer.name;
    wire.z = layer.bottom;
    wire.height = layer.top - layer.bottom;
}

/**
 * Maps a dielectric relative permittivity to a subtle translucent band fill
 * color: low-k cool teal/blue, SiO2 (~3.9) soft indigo, high-k (~7+) warm violet.
 */
export function dielectricColor(eps, isDark, alpha = 0.16) {
    const t = Math.max(0, Math.min(1, (eps - 2.0) / 6.0));
    const r = Math.round(isDark ? (55 + t * 135) : (70 + t * 130));
    const g = Math.round(isDark ? (145 - t * 65) : (155 - t * 65));
    const b = Math.round(isDark ? (175 + t * 35) : (190 + t * 30));
    return `rgba(${r}, ${g}, ${b}, ${alpha})`;
}

/**
 * Interactive 2D cross-section field solver widget backed by fsv::Solver.
 */
export class FieldSolver2DWidget {
    constructor(container, app) {
        this._app = app;
        this._container = container;
        this._element = document.createElement('div');
        this._element.className = 'fsv-widget';
        this._element.tabIndex = 0;
        container.element.appendChild(this._element);

        if (isStaticMode(app)) {
            const msg = document.createElement('div');
            msg.className = 'fsv-static-msg';
            msg.textContent = 'The interactive 2D field solver requires a live OpenROAD server.';
            this._element.appendChild(msg);
            this._destroyed = true;
            return;
        }

        this._wires = createDefaultWires();
        this._nextId = 3;
        this._selectedId = 0;
        this._eps = DEFAULT_EPS;
        this._bins = DEFAULT_BINS;
        this._itf = null;
        this._itfRevision = null;
        this._showDielectrics = true;
        this._showProfiles = true;
        this._showNeighborCharges = true;
        this._result = null;
        this._error = null;
        this._solving = false;
        this._solveSeq = 0;
        this._solveTimer = null;
        this._hasSolvedOnce = false;
        this._destroyed = false;

        // Viewport in world µm: centerX, centerZ, pxPerUm
        this._view = { centerX: 0, centerZ: 0.45, scale: 320 };
        this._drag = null;
        this._hover = null;
        this._hoverBin = null;

        this._buildDOM();
        this._bindEvents();
        this._updateSidebar();
        this._fitView();
        this._draw();

        // Trigger initial sync/solve when the tab becomes visible, and re-sync
        // on subsequent tab switches in case Tcl read_itf updated the stack.
        if (container && typeof container.on === 'function') {
            container.on('show', () => {
                this._resizeCanvas();
                if (!this._hasSolvedOnce) {
                    this._fitView();
                    this._initFromServer();
                } else {
                    this._syncItfFromServer();
                    this._draw();
                }
            });
            container.on('resize', () => {
                this._resizeCanvas();
                this._draw();
            });
            container.on('destroy', () => this.destroy());
        }
    }

    get wires() {
        return this._wires;
    }

    get selectedWire() {
        if (this._selectedId === null) return null;
        return this._wires.find(w => w.id === this._selectedId) || null;
    }

    _buildDOM() {
        // ── Top toolbar ──
        const toolbar = document.createElement('div');
        toolbar.className = 'fsv-toolbar';

        const addBtn = document.createElement('button');
        addBtn.type = 'button';
        addBtn.className = 'fsv-btn fsv-btn-primary';
        addBtn.textContent = '+ Add Wire';
        addBtn.title = 'Add a grounded neighbor wire (or double-click empty space)';
        addBtn.addEventListener('click', () => this.addWire());
        toolbar.appendChild(addBtn);
        this._addBtn = addBtn;

        const removeBtn = document.createElement('button');
        removeBtn.type = 'button';
        removeBtn.className = 'fsv-btn';
        removeBtn.textContent = 'Remove';
        removeBtn.title = 'Remove selected wire (Delete/Backspace)';
        removeBtn.addEventListener('click', () => this.removeSelectedWire());
        toolbar.appendChild(removeBtn);
        this._removeBtn = removeBtn;

        const resetBtn = document.createElement('button');
        resetBtn.type = 'button';
        resetBtn.className = 'fsv-btn';
        resetBtn.textContent = 'Reset';
        resetBtn.title = 'Reset to default 3-wire cross section';
        resetBtn.addEventListener('click', () => this.resetScene());
        toolbar.appendChild(resetBtn);

        const fitBtn = document.createElement('button');
        fitBtn.type = 'button';
        fitBtn.className = 'fsv-btn';
        fitBtn.textContent = 'Fit';
        fitBtn.title = 'Fit cross section to view';
        fitBtn.addEventListener('click', () => {
            this._fitView();
            this._draw();
        });
        toolbar.appendChild(fitBtn);

        const sep1 = document.createElement('span');
        sep1.className = 'fsv-toolbar-sep';
        toolbar.appendChild(sep1);

        const loadItfBtn = document.createElement('button');
        loadItfBtn.type = 'button';
        loadItfBtn.className = 'fsv-btn';
        loadItfBtn.textContent = 'Load ITF…';
        loadItfBtn.title = 'Load an ITF process stack file (wires snap to ITF layers)';
        loadItfBtn.addEventListener('click', () => this._openItfDialog());
        toolbar.appendChild(loadItfBtn);
        this._loadItfBtn = loadItfBtn;

        const itfPill = document.createElement('span');
        itfPill.className = 'fsv-itf-pill hidden';
        const itfPillText = document.createElement('span');
        itfPillText.className = 'fsv-itf-pill-text';
        const itfClearBtn = document.createElement('button');
        itfClearBtn.type = 'button';
        itfClearBtn.className = 'fsv-itf-pill-clear';
        itfClearBtn.title = 'Unload ITF stack and return to uniform permittivity';
        itfClearBtn.textContent = '×';
        itfClearBtn.addEventListener('click', () => this.clearItf());
        itfPill.appendChild(itfPillText);
        itfPill.appendChild(itfClearBtn);
        toolbar.appendChild(itfPill);
        this._itfPillEl = itfPill;
        this._itfPillTextEl = itfPillText;

        const epsLabel = document.createElement('label');
        epsLabel.className = 'fsv-toolbar-field';
        epsLabel.innerHTML = '<span>&epsilon;<sub>r</sub></span>';
        const epsInput = document.createElement('input');
        epsInput.type = 'number';
        epsInput.className = 'fsv-num-input fsv-eps-input';
        epsInput.step = '0.1';
        epsInput.min = '0.1';
        epsInput.max = '100';
        epsInput.value = String(this._eps);
        epsInput.addEventListener('change', () => {
            const v = parseFloat(epsInput.value);
            if (Number.isFinite(v) && v > 0) {
                this._eps = v;
                this._scheduleSolve(0);
            } else {
                epsInput.value = String(this._eps);
            }
        });
        epsLabel.appendChild(epsInput);
        toolbar.appendChild(epsLabel);
        this._epsLabel = epsLabel;
        this._epsInput = epsInput;

        const dielLabel = document.createElement('label');
        dielLabel.className = 'fsv-toolbar-check';
        const dielCb = document.createElement('input');
        dielCb.type = 'checkbox';
        dielCb.checked = this._showDielectrics;
        dielCb.addEventListener('change', () => {
            this._showDielectrics = dielCb.checked;
            this._draw();
        });
        dielLabel.appendChild(dielCb);
        dielLabel.appendChild(document.createTextNode('Dielectrics'));
        toolbar.appendChild(dielLabel);
        this._dielCb = dielCb;
        this._dielLabel = dielLabel;

        const profLabel = document.createElement('label');
        profLabel.className = 'fsv-toolbar-check';
        const profCb = document.createElement('input');
        profCb.type = 'checkbox';
        profCb.checked = this._showProfiles;
        profCb.addEventListener('change', () => {
            this._showProfiles = profCb.checked;
            this._draw();
        });
        profLabel.appendChild(profCb);
        profLabel.appendChild(document.createTextNode('Charge profiles'));
        toolbar.appendChild(profLabel);

        const nbLabel = document.createElement('label');
        nbLabel.className = 'fsv-toolbar-check';
        const nbCb = document.createElement('input');
        nbCb.type = 'checkbox';
        nbCb.checked = this._showNeighborCharges;
        nbCb.addEventListener('change', () => {
            this._showNeighborCharges = nbCb.checked;
            this._draw();
        });
        nbLabel.appendChild(nbCb);
        nbLabel.appendChild(document.createTextNode('Neighbor charges'));
        toolbar.appendChild(nbLabel);

        const status = document.createElement('span');
        status.className = 'fsv-status';
        status.textContent = 'Ready';
        this._statusEl = status;

        this._element.appendChild(toolbar);

        // ── Main body: canvas + sidebar ──
        const body = document.createElement('div');
        body.className = 'fsv-body';

        const canvasWrap = document.createElement('div');
        canvasWrap.className = 'fsv-canvas-wrap';

        const canvas = document.createElement('canvas');
        canvas.className = 'fsv-canvas';
        canvasWrap.appendChild(canvas);
        this._canvas = canvas;
        this._canvasWrap = canvasWrap;

        canvasWrap.appendChild(status);

        const hint = document.createElement('div');
        hint.className = 'fsv-canvas-hint';
        hint.textContent = 'Drag wires to move • Drag left/right edges to resize width • Double-click empty space to add wire • Scroll to zoom';
        canvasWrap.appendChild(hint);

        const tooltip = document.createElement('div');
        tooltip.className = 'fsv-tooltip hidden';
        canvasWrap.appendChild(tooltip);
        this._tooltipEl = tooltip;

        body.appendChild(canvasWrap);

        // ── Right sidebar ──
        const sidebar = document.createElement('div');
        sidebar.className = 'fsv-sidebar';

        // Selected Wire section
        const propSec = document.createElement('div');
        propSec.className = 'fsv-section';
        const propHeader = document.createElement('div');
        propHeader.className = 'fsv-section-title';
        propHeader.textContent = 'Selected Wire';
        propSec.appendChild(propHeader);

        const propBadge = document.createElement('div');
        propBadge.className = 'fsv-wire-badge';
        propSec.appendChild(propBadge);
        this._propBadgeEl = propBadge;

        const propGrid = document.createElement('div');
        propGrid.className = 'fsv-prop-grid';

        // ITF layer selector (visible only when an ITF stack is loaded).
        const layerLbl = document.createElement('label');
        layerLbl.className = 'fsv-prop-label hidden';
        layerLbl.textContent = 'Layer';
        const layerSel = document.createElement('select');
        layerSel.className = 'fsv-select hidden';
        layerSel.addEventListener('change', () => this._onLayerSelectChange(layerSel.value));
        propGrid.appendChild(layerLbl);
        propGrid.appendChild(layerSel);
        this._layerLabelEl = layerLbl;
        this._layerSelectEl = layerSel;

        this._propInputs = {};
        const fields = [
            { key: 'x', label: 'Center X (µm)', step: '0.02' },
            { key: 'z', label: 'Bottom Z (µm)', step: '0.02', min: String(MIN_WIRE_Z) },
            { key: 'width', label: 'Width (µm)', step: '0.02', min: String(MIN_WIRE_WIDTH) },
            { key: 'height', label: 'Height (µm)', step: '0.02', min: String(MIN_WIRE_HEIGHT) },
        ];
        for (const f of fields) {
            const lbl = document.createElement('label');
            lbl.className = 'fsv-prop-label';
            lbl.textContent = f.label;
            const inp = document.createElement('input');
            inp.type = 'number';
            inp.className = 'fsv-num-input';
            inp.step = f.step;
            if (f.min !== undefined) inp.min = f.min;
            inp.addEventListener('change', () => this._onPropInputChange(f.key, inp));
            propGrid.appendChild(lbl);
            propGrid.appendChild(inp);
            this._propInputs[f.key] = inp;
        }
        propSec.appendChild(propGrid);
        sidebar.appendChild(propSec);

        // Capacitance section
        const capSec = document.createElement('div');
        capSec.className = 'fsv-section';
        const capHeader = document.createElement('div');
        capHeader.className = 'fsv-section-title';
        capHeader.innerHTML = 'Capacitance <span class="fsv-unit">(fF/µm)</span>';
        capSec.appendChild(capHeader);

        const capBody = document.createElement('div');
        capBody.className = 'fsv-cap-list';
        capSec.appendChild(capBody);
        this._capListEl = capBody;
        sidebar.appendChild(capSec);

        // Wires list section
        const listSec = document.createElement('div');
        listSec.className = 'fsv-section fsv-section-grow';
        const listHeader = document.createElement('div');
        listHeader.className = 'fsv-section-title';
        listHeader.textContent = 'Wires';
        listSec.appendChild(listHeader);

        const wireList = document.createElement('div');
        wireList.className = 'fsv-wire-list';
        listSec.appendChild(wireList);
        this._wireListEl = wireList;
        sidebar.appendChild(listSec);

        body.appendChild(sidebar);
        this._element.appendChild(body);
    }

    _bindEvents() {
        const canvas = this._canvas;
        canvas.addEventListener('pointerdown', e => this._onPointerDown(e));
        canvas.addEventListener('pointermove', e => this._onPointerMove(e));
        canvas.addEventListener('pointerup', e => this._onPointerUp(e));
        canvas.addEventListener('pointerleave', () => this._onPointerLeave());
        canvas.addEventListener('dblclick', e => this._onDoubleClick(e));
        canvas.addEventListener('wheel', e => this._onWheel(e), { passive: false });

        this._element.addEventListener('keydown', e => {
            if (e.target && (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA')) {
                return;
            }
            if (e.key === 'Delete' || e.key === 'Backspace') {
                e.preventDefault();
                this.removeSelectedWire();
            } else if (e.key === 'Escape') {
                this.selectWire(null);
            }
        });

        if (typeof ResizeObserver !== 'undefined') {
            this._resizeObserver = new ResizeObserver(() => {
                this._resizeCanvas();
                this._draw();
            });
            this._resizeObserver.observe(this._canvasWrap);
        }

        // Redraw when light/dark theme changes.
        this._onThemeChange = () => this._draw();
        if (typeof window !== 'undefined') {
            window.addEventListener('or-theme-change', this._onThemeChange);
        }
    }

    destroy() {
        this._destroyed = true;
        if (this._solveTimer) {
            clearTimeout(this._solveTimer);
            this._solveTimer = null;
        }
        if (this._resizeObserver) {
            this._resizeObserver.disconnect();
            this._resizeObserver = null;
        }
        if (typeof window !== 'undefined' && this._onThemeChange) {
            window.removeEventListener('or-theme-change', this._onThemeChange);
        }
    }

    // ─── ITF Stack Support ──────────────────────────────────────────────────

    get itfStack() {
        return this._itf;
    }

    async _initFromServer() {
        const ws = this._app?.websocketManager;
        if (!ws) {
            this.solveNow();
            return;
        }
        try {
            const resp = await ws.request({ type: 'fsv_get_itf' });
            if (this._destroyed) return;
            if (resp && resp.revision !== undefined) {
                this._itfRevision = resp.revision;
            }
            if (resp && resp.loaded && Array.isArray(resp.conductors) && resp.conductors.length > 0) {
                this.applyItfStack(resp);
                return;
            }
        } catch (_) {
            // Fall through to default uniform solve.
        }
        if (!this._destroyed) {
            this.solveNow();
        }
    }

    async _syncItfFromServer() {
        const ws = this._app?.websocketManager;
        if (!ws) return;
        try {
            const resp = await ws.request({ type: 'fsv_get_itf' });
            if (this._destroyed || !resp) return;
            if (resp.revision !== undefined && resp.revision === this._itfRevision) {
                return;
            }
            if (resp.revision !== undefined) {
                this._itfRevision = resp.revision;
            }
            if (resp.loaded && Array.isArray(resp.conductors) && resp.conductors.length > 0) {
                this.applyItfStack(resp);
            } else if (this._itf) {
                this._itf = null;
                for (const w of this._wires) delete w.layer;
                this._syncItfControls();
                this._updateSidebar();
                this._draw();
                this._scheduleSolve(0);
            }
        } catch (_) {
            // Ignore transient sync errors.
        }
    }

    async loadItfFromPath(path) {
        const ws = this._app?.websocketManager;
        if (!ws) return { error: 'No server connection.' };
        const resp = await ws.request({ type: 'fsv_load_itf', path });
        if (resp && !resp.error) {
            this.applyItfStack(resp);
        }
        return resp;
    }

    async loadItfFromContent(name, content) {
        const ws = this._app?.websocketManager;
        if (!ws) return { error: 'No server connection.' };
        const resp = await ws.request({ type: 'fsv_load_itf', name, content });
        if (resp && !resp.error) {
            this.applyItfStack(resp);
        }
        return resp;
    }

    applyItfStack(stack) {
        this._itf = stack;
        if (stack && stack.revision !== undefined) {
            this._itfRevision = stack.revision;
        }
        const defLayer = pickDefaultItfLayer(stack.conductors);
        if (defLayer) {
            const w = Math.max(MIN_WIRE_WIDTH, roundCoord(defLayer.min_width || 0.2));
            const s = Math.max(0.05, roundCoord(defLayer.min_spacing || w));
            const pitch = roundCoord(w + s);
            const h = defLayer.top - defLayer.bottom;
            const z = defLayer.bottom;
            this._wires = [
                { id: 0, name: 'target', x: 0.0, z, width: w, height: h, layer: defLayer.name, isTarget: true },
                { id: 1, name: 'w1', x: -pitch, z, width: w, height: h, layer: defLayer.name, isTarget: false },
                { id: 2, name: 'w2', x: pitch, z, width: w, height: h, layer: defLayer.name, isTarget: false },
            ];
            this._nextId = 3;
            this._selectedId = 0;
        }
        this._showDielectrics = true;
        if (this._dielCb) this._dielCb.checked = true;
        this._syncItfControls();
        this._updateSidebar();
        this._fitView();
        this._draw();
        this._scheduleSolve(0);
    }

    async clearItf() {
        const ws = this._app?.websocketManager;
        this._itf = null;
        for (const w of this._wires) {
            delete w.layer;
        }
        if (ws) {
            try {
                const resp = await ws.request({ type: 'fsv_clear_itf' });
                if (resp && resp.revision !== undefined) {
                    this._itfRevision = resp.revision;
                }
            } catch (_) { /* ignore */ }
        }
        this._syncItfControls();
        this._updateSidebar();
        this._draw();
        this._scheduleSolve(0);
    }

    _syncItfControls() {
        const hasItf = !!this._itf;
        if (this._itfPillEl && this._itfPillTextEl) {
            this._itfPillEl.classList.toggle('hidden', !hasItf);
            if (hasItf) {
                const label = this._itf.technology || this._itf.source || 'ITF';
                this._itfPillTextEl.textContent = `ITF: ${label}`;
                this._itfPillEl.title = this._itf.source || label;
            }
        }
        if (this._epsLabel) {
            this._epsLabel.classList.toggle('hidden', hasItf);
        }
        if (this._layerLabelEl && this._layerSelectEl) {
            this._layerLabelEl.classList.toggle('hidden', !hasItf);
            this._layerSelectEl.classList.toggle('hidden', !hasItf);
            if (hasItf) {
                this._layerSelectEl.innerHTML = '';
                for (const c of this._itf.conductors) {
                    const opt = document.createElement('option');
                    opt.value = c.name;
                    opt.textContent = `${c.name} (${c.bottom.toFixed(2)}–${c.top.toFixed(2)} µm)`;
                    this._layerSelectEl.appendChild(opt);
                }
            }
        }
    }

    _findItfLayer(name) {
        if (!this._itf || !name) return null;
        return this._itf.conductors.find(c => c.name === name) || null;
    }

    _onLayerSelectChange(layerName) {
        const wire = this.selectedWire;
        const layer = this._findItfLayer(layerName);
        if (!wire || !layer) return;
        const prev = {
            z: wire.z,
            height: wire.height,
            width: wire.width,
            layer: wire.layer,
        };
        snapWireToLayer(wire, layer);
        if (layer.min_width > 0 && wire.width < layer.min_width) {
            wire.width = roundCoord(layer.min_width);
        }
        if (hasSpacingViolation(this._wires, this._itf)) {
            Object.assign(wire, prev);
            this._setStatus(`Cannot snap to ${layer.name} — violates min spacing`, true);
            this._updateSidebar();
            return;
        }
        this._updateSidebar();
        this._draw();
        this._scheduleSolve(0);
    }

    _openItfDialog() {
        const overlay = document.createElement('div');
        overlay.className = 'modal-overlay';
        overlay.innerHTML = `
            <div class="modal-dialog file-browser-dialog fsv-itf-dialog">
                <div class="modal-header">
                    <span>Load ITF Process Stack</span>
                    <button type="button" class="modal-close" aria-label="Close">×</button>
                </div>
                <div class="modal-body">
                    <div class="fsv-itf-local-row">
                        <span class="fsv-itf-local-label">Browse server files below, or upload from your computer:</span>
                        <button type="button" class="fsv-btn fsv-itf-upload-btn">Upload Local .itf…</button>
                        <input type="file" class="fsv-itf-file-input" accept=".itf,.ITF,.txt" style="display:none">
                    </div>
                    <div class="fb-breadcrumb"></div>
                    <div class="fb-list"></div>
                    <div class="fb-path-row">
                        <label>Path:</label>
                        <input type="text" class="fb-path-input" placeholder="/path/to/stack.itf">
                    </div>
                    <div class="fb-error" style="display:none"></div>
                </div>
                <div class="modal-footer">
                    <button type="button" class="fsv-btn fsv-itf-cancel">Cancel</button>
                    <button type="button" class="fsv-btn fsv-btn-primary fsv-itf-ok" disabled>Load</button>
                </div>
            </div>
        `;
        document.body.appendChild(overlay);

        let closed = false;
        let currentPath = '';
        const close = () => {
            if (closed) return;
            closed = true;
            overlay.remove();
        };

        const breadcrumb = overlay.querySelector('.fb-breadcrumb');
        const listEl = overlay.querySelector('.fb-list');
        const pathInput = overlay.querySelector('.fb-path-input');
        const errEl = overlay.querySelector('.fb-error');
        const okBtn = overlay.querySelector('.fsv-itf-ok');
        const uploadBtn = overlay.querySelector('.fsv-itf-upload-btn');
        const fileInput = overlay.querySelector('.fsv-itf-file-input');

        const showError = msg => {
            errEl.textContent = msg;
            errEl.style.display = '';
        };
        const updateOk = () => {
            okBtn.disabled = !pathInput.value.trim();
        };

        overlay.querySelector('.modal-close').addEventListener('click', close);
        overlay.querySelector('.fsv-itf-cancel').addEventListener('click', close);
        overlay.addEventListener('click', e => {
            if (e.target === overlay) close();
        });
        pathInput.addEventListener('input', updateOk);

        uploadBtn.addEventListener('click', () => fileInput.click());
        fileInput.addEventListener('change', async () => {
            const file = fileInput.files && fileInput.files[0];
            if (!file) return;
            errEl.style.display = 'none';
            try {
                const text = await file.text();
                const resp = await this.loadItfFromContent(file.name, text);
                if (closed) return;
                if (resp && resp.error) {
                    showError(resp.error);
                } else {
                    close();
                }
            } catch (err) {
                if (!closed) showError(err?.message || String(err));
            }
        });

        const submitPath = async () => {
            const p = pathInput.value.trim();
            if (!p) return;
            okBtn.disabled = true;
            okBtn.textContent = 'Loading…';
            errEl.style.display = 'none';
            try {
                const resp = await this.loadItfFromPath(p);
                if (closed) return;
                if (resp && resp.error) {
                    showError(resp.error);
                    okBtn.disabled = false;
                    okBtn.textContent = 'Load';
                } else {
                    close();
                }
            } catch (err) {
                if (!closed) {
                    showError(err?.message || String(err));
                    okBtn.disabled = false;
                    okBtn.textContent = 'Load';
                }
            }
        };

        okBtn.addEventListener('click', submitPath);
        pathInput.addEventListener('keydown', e => {
            if (e.key === 'Enter') submitPath();
            if (e.key === 'Escape') close();
        });

        const navigate = async dirPath => {
            errEl.style.display = 'none';
            listEl.innerHTML = '<div class="fb-loading">Loading…</div>';
            try {
                const resp = await this._app.websocketManager.request({
                    type: 'list_dir',
                    path: dirPath || '',
                });
                if (closed) return;
                currentPath = resp.path || '';
                breadcrumb.textContent = currentPath;
                listEl.innerHTML = '';

                if (currentPath && currentPath !== '/') {
                    const up = document.createElement('div');
                    up.className = 'fb-entry fb-dir';
                    up.textContent = '📁 ..';
                    up.addEventListener('click', () => {
                        const parent = currentPath.replace(/\/[^/]+\/?$/, '') || '/';
                        navigate(parent);
                    });
                    listEl.appendChild(up);
                }

                const entries = (resp.entries || []).filter(
                    ent => ent.is_dir || /\.itf$/i.test(ent.name));
                entries.sort((a, b) => (b.is_dir - a.is_dir) || a.name.localeCompare(b.name));

                if (entries.length === 0) {
                    const empty = document.createElement('div');
                    empty.className = 'fb-loading';
                    empty.textContent = 'No .itf files in this directory';
                    listEl.appendChild(empty);
                }
                for (const ent of entries) {
                    const row = document.createElement('div');
                    row.className = 'fb-entry ' + (ent.is_dir ? 'fb-dir' : 'fb-file');
                    row.textContent = (ent.is_dir ? '📁 ' : '📄 ') + ent.name;
                    const full = currentPath.endsWith('/')
                        ? currentPath + ent.name
                        : `${currentPath}/${ent.name}`;
                    if (ent.is_dir) {
                        row.addEventListener('click', () => navigate(full));
                    } else {
                        row.addEventListener('click', () => {
                            for (const el of listEl.querySelectorAll('.fb-entry.selected')) {
                                el.classList.remove('selected');
                            }
                            row.classList.add('selected');
                            pathInput.value = full;
                            updateOk();
                        });
                        row.addEventListener('dblclick', () => {
                            pathInput.value = full;
                            updateOk();
                            submitPath();
                        });
                    }
                    listEl.appendChild(row);
                }
            } catch (err) {
                if (!closed) {
                    listEl.innerHTML = '';
                    showError(err?.message || String(err));
                }
            }
        };

        navigate('');
    }

    // ─── Wire Operations ────────────────────────────────────────────────────

    addWire(atPos = null) {
        let layer = null;
        if (this._itf) {
            if (atPos) {
                layer = nearestItfLayer(this._itf.conductors, atPos.z);
            } else {
                layer = this._findItfLayer(this.selectedWire?.layer)
                    || pickDefaultItfLayer(this._itf.conductors);
            }
        }

        const width = layer
            ? Math.max(MIN_WIRE_WIDTH, roundCoord(layer.min_width || 0.2))
            : 0.2;
        const height = layer
            ? (layer.top - layer.bottom)
            : 0.2;
        const baseZ = layer ? layer.bottom : 0.4;
        const spacing = (layer && layer.min_spacing > 0) ? layer.min_spacing : 0.01;

        let pos = atPos;
        if (!pos) {
            const stepX = width + Math.max(spacing, 0.2);
            pos = null;
            for (let k = 1; k <= 12 && !pos; k++) {
                for (const sign of [1, -1]) {
                    const candidate = {
                        x: roundCoord(sign * k * stepX),
                        z: baseZ,
                        width,
                        height,
                        layer: layer ? layer.name : undefined,
                    };
                    if (!hasSpacingViolation([...this._wires, candidate], this._itf)) {
                        pos = { x: candidate.x, z: candidate.z };
                        break;
                    }
                }
            }
            if (!pos) {
                pos = findNewWirePlacement(this._wires, width, height, baseZ);
                if (layer && Math.abs(pos.z - baseZ) > 1e-6) {
                    layer = nearestItfLayer(this._itf.conductors, pos.z + height / 2);
                    pos.z = layer.bottom;
                }
            }
        } else {
            const probe = {
                x: roundCoord(pos.x),
                z: layer ? baseZ : Math.max(MIN_WIRE_Z, roundCoord(pos.z - height / 2)),
                width,
                height,
                layer: layer ? layer.name : undefined,
            };
            if (hasSpacingViolation([...this._wires, probe], this._itf)) {
                return null;
            }
            pos = { x: probe.x, z: probe.z };
        }
        const id = this._nextId++;
        const wire = {
            id,
            name: `w${id}`,
            x: pos.x,
            z: pos.z,
            width,
            height,
            isTarget: false,
        };
        if (layer) {
            wire.layer = layer.name;
        }
        this._wires.push(wire);
        this._selectedId = wire.id;
        this._updateSidebar();
        this._draw();
        this._scheduleSolve(0);
        return wire;
    }

    removeWire(id) {
        if (id === null || id === undefined) return false;
        const idx = this._wires.findIndex(w => w.id === id);
        if (idx < 0 || this._wires[idx].isTarget) {
            return false;
        }
        this._wires.splice(idx, 1);
        if (this._selectedId === id) {
            this._selectedId = null;
        }
        this._updateSidebar();
        this._draw();
        this._scheduleSolve(0);
        return true;
    }

    removeSelectedWire() {
        return this.removeWire(this._selectedId);
    }

    selectWire(id) {
        if (id === null || id === undefined) {
            this._selectedId = null;
        } else if (this._wires.some(w => w.id === id)) {
            this._selectedId = id;
        } else {
            return;
        }
        this._updateSidebar();
        this._draw();
    }

    resetScene() {
        if (this._itf) {
            this.applyItfStack(this._itf);
            return;
        }
        this._wires = createDefaultWires();
        this._nextId = 3;
        this._selectedId = 0;
        this._eps = DEFAULT_EPS;
        if (this._epsInput) this._epsInput.value = String(this._eps);
        this._updateSidebar();
        this._fitView();
        this._draw();
        this._scheduleSolve(0);
    }

    _onPropInputChange(key, inp) {
        const wire = this.selectedWire;
        if (!wire) return;
        const val = parseFloat(inp.value);
        if (!Number.isFinite(val)) {
            this._updateSidebar();
            return;
        }
        const prev = { x: wire.x, z: wire.z, width: wire.width, height: wire.height };
        if (key === 'x') {
            if (wire.isTarget) {
                wire.x = 0.0;
            } else {
                wire.x = roundCoord(val);
            }
        } else if (key === 'z') {
            wire.z = Math.max(MIN_WIRE_Z, roundCoord(val));
        } else if (key === 'width') {
            const minW = minWidthForWire(wire, this._itf);
            wire.width = Math.max(minW, roundCoord(val));
            if (wire.isTarget) {
                wire.x = 0.0;
            }
        } else if (key === 'height') {
            wire.height = Math.max(MIN_WIRE_HEIGHT, roundCoord(val));
        }
        if (hasSpacingViolation(this._wires, this._itf)) {
            Object.assign(wire, prev);
            this._setStatus('Violates minimum spacing or overlaps another wire', true);
            this._updateSidebar();
            return;
        }
        this._updateSidebar();
        this._draw();
        this._scheduleSolve(0);
    }

    // ─── Solver Communication ───────────────────────────────────────────────

    _scheduleSolve(delayMs = SOLVE_DEBOUNCE_MS) {
        if (this._solveTimer) {
            clearTimeout(this._solveTimer);
            this._solveTimer = null;
        }
        if (delayMs <= 0) {
            this.solveNow();
        } else {
            this._solveTimer = setTimeout(() => {
                this._solveTimer = null;
                this.solveNow();
            }, delayMs);
        }
    }

    async solveNow() {
        if (this._destroyed) return null;
        const ws = this._app?.websocketManager;
        if (!ws) return null;

        if (hasAnyOverlap(this._wires)) {
            this._error = 'Wires overlap — separate them to solve.';
            this._setStatus(this._error, true);
            this._draw();
            return null;
        }

        const seq = ++this._solveSeq;
        this._solving = true;
        this._error = null;
        this._hasSolvedOnce = true;
        this._setStatus('Solving…', false);

        const targetIndex = this._wires.findIndex(w => w.isTarget);
        let maxAbsX = 0.6;
        let maxZ = 0.6;
        for (const w of this._wires) {
            maxAbsX = Math.max(maxAbsX, Math.abs(w.x) + w.width / 2);
            maxZ = Math.max(maxZ, w.z + w.height);
        }
        const groundHalfSpan = roundCoord(Math.max(1.2, maxAbsX + 1.8 * maxZ));
        const payload = {
            type: 'fsv_solve_2d',
            use_itf: !!this._itf,
            eps: this._eps,
            target_index: targetIndex >= 0 ? targetIndex : 0,
            bins: this._bins,
            ground_bins: 48,
            ground_x_lo: -groundHalfSpan,
            ground_x_hi: groundHalfSpan,
            tol_total: 0.005,
            min_walks: 131072,
            max_walks: 1000000,
            wires: this._wires.map(w => ({
                name: w.name,
                x: w.isTarget ? 0.0 : w.x,
                z: w.z,
                width: w.width,
                height: w.height,
                ...(w.layer ? { layer: w.layer } : {}),
            })),
        };

        try {
            const resp = await ws.request(payload);
            if (this._destroyed || seq !== this._solveSeq) {
                return null;
            }
            this._solving = false;
            if (resp && resp.error) {
                this._error = resp.error;
                this._setStatus(resp.error, true);
            } else {
                if (resp && Array.isArray(resp.charges)) {
                    resp.charges = resp.charges.map((ch, idx) => {
                        const isTarget = !!(this._wires[idx] && this._wires[idx].isTarget);
                        return {
                            ...ch,
                            bottom: smoothChargeFace(ch.bottom, isTarget),
                            top: smoothChargeFace(ch.top, isTarget),
                            left: smoothChargeFace(ch.left, isTarget),
                            right: smoothChargeFace(ch.right, isTarget),
                        };
                    });
                }
                if (resp && resp.ground_charge && Array.isArray(resp.ground_charge.values)) {
                    resp.ground_charge = {
                        ...resp.ground_charge,
                        values: smoothChargeFace(resp.ground_charge.values, false),
                    };
                }
                this._result = resp;
                this._error = null;
                const ms = Math.round((resp.seconds || 0) * 1000);
                const walksK = Math.round((resp.walks || 0) / 1000);
                this._setStatus(`Solved in ${ms} ms (${walksK}k walks)`, false);
            }
            this._updateCapacitancePanel();
            this._draw();
            return resp;
        } catch (err) {
            if (this._destroyed || seq !== this._solveSeq) {
                return null;
            }
            this._solving = false;
            this._error = err?.message || String(err);
            this._setStatus(this._error, true);
            this._draw();
            return null;
        }
    }

    _setStatus(text, isError) {
        if (!this._statusEl) return;
        this._statusEl.textContent = text;
        this._statusEl.classList.toggle('fsv-status-error', !!isError);
    }

    // ─── Sidebar Updates ────────────────────────────────────────────────────

    _updateSelectedInputs() {
        const wire = this.selectedWire;
        if (!wire) {
            if (this._removeBtn) this._removeBtn.disabled = true;
            if (this._propBadgeEl) {
                this._propBadgeEl.textContent = 'No wire selected';
                this._propBadgeEl.classList.remove('is-target');
            }
            if (this._layerSelectEl) {
                this._layerSelectEl.disabled = true;
            }
            for (const key of ['x', 'z', 'width', 'height']) {
                const inp = this._propInputs[key];
                if (!inp) continue;
                inp.value = '';
                inp.disabled = true;
            }
            return;
        }

        if (this._removeBtn) {
            this._removeBtn.disabled = !!wire.isTarget;
        }
        if (this._propBadgeEl) {
            this._propBadgeEl.textContent = wire.isTarget
                ? `${wire.name} — Target (1V, centered at x = 0)`
                : `${wire.name} — Grounded neighbor (0V)`;
            this._propBadgeEl.classList.toggle('is-target', !!wire.isTarget);
        }

        const hasItf = !!this._itf;
        if (this._layerSelectEl) {
            this._layerSelectEl.disabled = !hasItf;
            if (hasItf && wire.layer) {
                this._layerSelectEl.value = wire.layer;
            }
        }

        const setVal = (key, val, disabled = false) => {
            const inp = this._propInputs[key];
            if (!inp) return;
            inp.value = Number(val).toFixed(3);
            inp.disabled = disabled;
        };
        setVal('x', wire.isTarget ? 0.0 : wire.x, !!wire.isTarget);
        setVal('z', wire.z, hasItf);
        setVal('width', wire.width, false);
        if (this._propInputs.width) {
            this._propInputs.width.min = String(minWidthForWire(wire, this._itf));
        }
        setVal('height', wire.height, hasItf);
    }

    _updateSidebar() {
        this._updateSelectedInputs();
        this._updateWireList();
        this._updateCapacitancePanel();
    }

    _updateWireList() {
        if (!this._wireListEl) return;
        this._wireListEl.innerHTML = '';
        for (const w of this._wires) {
            const row = document.createElement('div');
            row.className = 'fsv-wire-row'
                + (w.id === this._selectedId ? ' selected' : '')
                + (w.isTarget ? ' is-target' : '');
            row.addEventListener('click', () => this.selectWire(w.id));

            const dot = document.createElement('span');
            dot.className = 'fsv-wire-dot';
            row.appendChild(dot);

            const nameEl = document.createElement('span');
            nameEl.className = 'fsv-wire-name';
            nameEl.textContent = w.isTarget ? `${w.name} (target)` : w.name;
            row.appendChild(nameEl);

            const dims = document.createElement('span');
            dims.className = 'fsv-wire-dims';
            dims.textContent = w.layer
                ? `${w.layer} w=${w.width.toFixed(2)}`
                : `w=${w.width.toFixed(2)}`;
            row.appendChild(dims);

            if (!w.isTarget) {
                const del = document.createElement('button');
                del.type = 'button';
                del.className = 'fsv-wire-del';
                del.title = `Remove ${w.name}`;
                del.textContent = '×';
                del.addEventListener('click', e => {
                    e.stopPropagation();
                    this.removeWire(w.id);
                });
                row.appendChild(del);
            }
            this._wireListEl.appendChild(row);
        }
    }

    _updateCapacitancePanel() {
        if (!this._capListEl) return;
        this._capListEl.innerHTML = '';

        if (this._error) {
            const errEl = document.createElement('div');
            errEl.className = 'fsv-cap-error';
            errEl.textContent = this._error;
            this._capListEl.appendChild(errEl);
            return;
        }
        if (!this._result) {
            const empty = document.createElement('div');
            empty.className = 'fsv-cap-empty';
            empty.textContent = this._solving ? 'Solving…' : 'No solution yet.';
            this._capListEl.appendChild(empty);
            return;
        }

        const r = this._result;
        const total = r.total || 0;

        const addRow = (label, cap, sigma, opts = {}) => {
            const row = document.createElement('div');
            row.className = 'fsv-cap-row'
                + (opts.isTotal ? ' is-total' : '')
                + (opts.wireId !== undefined ? ' clickable' : '');
            if (opts.wireId !== undefined) {
                row.addEventListener('click', () => this.selectWire(opts.wireId));
            }
            const top = document.createElement('div');
            top.className = 'fsv-cap-row-top';
            const lbl = document.createElement('span');
            lbl.className = 'fsv-cap-label';
            lbl.textContent = label;
            const val = document.createElement('span');
            val.className = 'fsv-cap-val';
            val.textContent = formatCap(cap, sigma);
            top.appendChild(lbl);
            top.appendChild(val);
            row.appendChild(top);

            if (!opts.isTotal && total > 0) {
                const pct = Math.max(0, Math.min(100, (cap / total) * 100));
                const barBg = document.createElement('div');
                barBg.className = 'fsv-cap-bar-bg';
                const bar = document.createElement('div');
                bar.className = 'fsv-cap-bar' + (opts.isGround ? ' is-ground' : '');
                bar.style.width = `${pct.toFixed(1)}%`;
                barBg.appendChild(bar);
                row.appendChild(barBg);
            }
            this._capListEl.appendChild(row);
        };

        addRow('Total', r.total, r.total_sigma, { isTotal: true });
        addRow('Ground', r.ground, r.ground_sigma, { isGround: true });

        if (Array.isArray(r.couplings)) {
            for (const c of r.couplings) {
                const wire = this._wires[c.index];
                addRow(c.name, c.cap, c.sigma, { wireId: wire ? wire.id : undefined });
            }
        }
    }

    // ─── Coordinate Transforms & Viewport ───────────────────────────────────

    _resizeCanvas() {
        const wrap = this._canvasWrap;
        if (!wrap || !this._canvas) return;
        const rect = wrap.getBoundingClientRect();
        const w = Math.max(1, Math.round(rect.width));
        const h = Math.max(1, Math.round(rect.height));
        const dpr = (typeof window !== 'undefined' && window.devicePixelRatio) || 1;
        const bw = Math.round(w * dpr);
        const bh = Math.round(h * dpr);
        this._cssWidth = w;
        this._cssHeight = h;
        this._dpr = dpr;
        if (this._canvas.width !== bw) this._canvas.width = bw;
        if (this._canvas.height !== bh) this._canvas.height = bh;
    }

    _fitView() {
        this._resizeCanvas();
        const w = this._cssWidth || 600;
        const h = this._cssHeight || 420;
        let xMin = -0.6;
        let xMax = 0.6;
        let zMax = 0.9;
        for (const wire of this._wires) {
            xMin = Math.min(xMin, wire.x - wire.width / 2);
            xMax = Math.max(xMax, wire.x + wire.width / 2);
            zMax = Math.max(zMax, wire.z + wire.height);
        }
        const halfX = Math.max(Math.abs(xMin), Math.abs(xMax)) + 0.35;
        const spanX = 2 * halfX;
        const spanZ = zMax + 0.45;
        const scale = Math.min((w * 0.84) / spanX, (h * 0.78) / spanZ);
        this._view = {
            centerX: 0,
            centerZ: (zMax + 0.05) * 0.45,
            scale: Math.max(40, Math.min(2000, scale)),
        };
    }

    worldToScreen(wx, wz) {
        const w = this._cssWidth || 600;
        const h = this._cssHeight || 420;
        const sx = w / 2 + (wx - this._view.centerX) * this._view.scale;
        const sy = h / 2 - (wz - this._view.centerZ) * this._view.scale;
        return { x: sx, y: sy };
    }

    screenToWorld(sx, sy) {
        const w = this._cssWidth || 600;
        const h = this._cssHeight || 420;
        const wx = this._view.centerX + (sx - w / 2) / this._view.scale;
        const wz = this._view.centerZ - (sy - h / 2) / this._view.scale;
        return { x: wx, z: wz };
    }

    _wireScreenRect(wire) {
        const tl = this.worldToScreen(wire.x - wire.width / 2, wire.z + wire.height);
        const br = this.worldToScreen(wire.x + wire.width / 2, wire.z);
        return {
            x: tl.x,
            y: tl.y,
            w: br.x - tl.x,
            h: br.y - tl.y,
        };
    }

    // ─── Hit Testing & Pointer Handling ─────────────────────────────────────

    _hitTest(sx, sy) {
        // Check selected wire first, then others top-to-bottom.
        const ordered = [...this._wires].sort((a, b) =>
            (b.id === this._selectedId ? 1 : 0) - (a.id === this._selectedId ? 1 : 0));
        for (const wire of ordered) {
            const r = this._wireScreenRect(wire);
            const inY = sy >= r.y - 4 && sy <= r.y + r.h + 4;
            if (inY && Math.abs(sx - r.x) <= HANDLE_HIT_PX) {
                return { wire, mode: 'resize-left' };
            }
            if (inY && Math.abs(sx - (r.x + r.w)) <= HANDLE_HIT_PX) {
                return { wire, mode: 'resize-right' };
            }
            if (sx >= r.x && sx <= r.x + r.w && sy >= r.y && sy <= r.y + r.h) {
                return { wire, mode: 'move' };
            }
        }
        return null;
    }

    _hitTestChargeBin(sx, sy) {
        if (!this._result || !Array.isArray(this._result.charges)) return null;
        const threshold = 10;
        for (let i = 0; i < this._wires.length; i++) {
            const wire = this._wires[i];
            if (!wire.isTarget && !this._showNeighborCharges) continue;
            const ch = this._result.charges[i];
            if (!ch) continue;
            const r = this._wireScreenRect(wire);
            const checks = [
                { face: 'top', dist: Math.abs(sy - r.y), inSpan: sx >= r.x && sx <= r.x + r.w, t: (sx - r.x) / Math.max(1, r.w), arr: ch.top },
                { face: 'bottom', dist: Math.abs(sy - (r.y + r.h)), inSpan: sx >= r.x && sx <= r.x + r.w, t: (sx - r.x) / Math.max(1, r.w), arr: ch.bottom },
                { face: 'left', dist: Math.abs(sx - r.x), inSpan: sy >= r.y && sy <= r.y + r.h, t: (r.y + r.h - sy) / Math.max(1, r.h), arr: ch.left },
                { face: 'right', dist: Math.abs(sx - (r.x + r.w)), inSpan: sy >= r.y && sy <= r.y + r.h, t: (r.y + r.h - sy) / Math.max(1, r.h), arr: ch.right },
            ];
            let best = null;
            for (const c of checks) {
                if (c.inSpan && c.dist <= threshold && Array.isArray(c.arr) && c.arr.length > 0) {
                    if (!best || c.dist < best.dist) best = c;
                }
            }
            if (best) {
                const idx = Math.max(0, Math.min(best.arr.length - 1, Math.floor(best.t * best.arr.length)));
                return { wire, face: best.face, value: best.arr[idx], sx, sy };
            }
        }
        // Also check ground plane surface charge along z = 0.
        if (this._showNeighborCharges && this._result.ground_charge
            && Array.isArray(this._result.ground_charge.values)) {
            const gch = this._result.ground_charge;
            const gndY = this.worldToScreen(0, 0).y;
            if (Math.abs(sy - gndY) <= threshold) {
                const wx = this.screenToWorld(sx, sy).x;
                if (wx >= gch.x_lo && wx <= gch.x_hi) {
                    const t = (wx - gch.x_lo) / Math.max(1e-9, gch.x_hi - gch.x_lo);
                    const idx = Math.max(0, Math.min(gch.values.length - 1, Math.floor(t * gch.values.length)));
                    return { wire: { name: 'ground' }, face: 'z=0', value: gch.values[idx], sx, sy };
                }
            }
        }
        return null;
    }

    _canvasCoords(e) {
        const rect = this._canvas.getBoundingClientRect();
        return { x: e.clientX - rect.left, y: e.clientY - rect.top };
    }

    _onPointerDown(e) {
        if (e.button !== 0 && e.button !== 1) return;
        if (this._element.focus) {
            this._element.focus({ preventScroll: true });
        }
        const pt = this._canvasCoords(e);
        const world = this.screenToWorld(pt.x, pt.y);
        const hit = e.button === 0 ? this._hitTest(pt.x, pt.y) : null;

        if (hit) {
            this._selectedId = hit.wire.id;
            this._drag = {
                mode: hit.mode,
                wire: hit.wire,
                startScreen: pt,
                startWorld: world,
                origX: hit.wire.x,
                origZ: hit.wire.z,
                origWidth: hit.wire.width,
            };
            this._updateSidebar();
            this._draw();
        } else {
            this._drag = {
                mode: 'pan',
                startScreen: pt,
                origCenterX: this._view.centerX,
                origCenterZ: this._view.centerZ,
            };
        }
        if (this._canvas.setPointerCapture) {
            try { this._canvas.setPointerCapture(e.pointerId); } catch (_) { /* ignore */ }
        }
    }

    _onPointerMove(e) {
        const pt = this._canvasCoords(e);
        if (this._drag) {
            const d = this._drag;
            if (d.mode === 'pan') {
                const dx = (pt.x - d.startScreen.x) / this._view.scale;
                const dy = (pt.y - d.startScreen.y) / this._view.scale;
                this._view.centerX = d.origCenterX - dx;
                this._view.centerZ = d.origCenterZ + dy;
                this._draw();
                return;
            }
            const world = this.screenToWorld(pt.x, pt.y);
            const wire = d.wire;
            const prev = {
                x: wire.x,
                z: wire.z,
                width: wire.width,
                height: wire.height,
                layer: wire.layer,
            };

            if (d.mode === 'move') {
                const dx = world.x - d.startWorld.x;
                const dz = world.z - d.startWorld.z;
                // Target wire stays centered at x = 0; only z (or ITF layer) moves.
                wire.x = wire.isTarget ? 0.0 : roundCoord(d.origX + dx);
                if (this._itf) {
                    const targetMidZ = d.origZ + wire.height / 2 + dz;
                    const snapped = nearestItfLayer(this._itf.conductors, targetMidZ);
                    snapWireToLayer(wire, snapped);
                    if (snapped && snapped.min_width > 0 && wire.width < snapped.min_width) {
                        wire.width = roundCoord(snapped.min_width);
                    }
                } else {
                    wire.z = Math.max(MIN_WIRE_Z, roundCoord(d.origZ + dz));
                }
            } else if (d.mode === 'resize-left' || d.mode === 'resize-right') {
                const minW = minWidthForWire(wire, this._itf);
                resizeWireEdge(
                    wire,
                    d.mode === 'resize-left' ? 'left' : 'right',
                    world.x,
                    minW);
            }

            if (hasSpacingViolation(this._wires, this._itf)) {
                // Clamp smoothly against the nearest blocking wire's min spacing
                // when sliding or resizing horizontally, rather than freezing early.
                const clamped = this._clampHorizontalToSpacing(wire, prev, d.mode);
                if (!clamped || hasSpacingViolation(this._wires, this._itf)) {
                    wire.x = prev.x;
                    wire.z = prev.z;
                    wire.width = prev.width;
                    wire.height = prev.height;
                    wire.layer = prev.layer;
                    return;
                }
            }

            this._updateSelectedInputs();
            this._draw();
            this._scheduleSolve(SOLVE_DEBOUNCE_MS);
            return;
        }

        // Not dragging: update cursor & surface-charge tooltip.
        const hit = this._hitTest(pt.x, pt.y);
        this._hover = hit;
        if (hit) {
            if (hit.mode === 'resize-left' || hit.mode === 'resize-right') {
                this._canvas.style.cursor = 'ew-resize';
            } else {
                this._canvas.style.cursor = hit.wire.isTarget ? 'ns-resize' : 'grab';
            }
        } else {
            this._canvas.style.cursor = 'default';
        }

        const binHit = this._hitTestChargeBin(pt.x, pt.y);
        this._hoverBin = binHit;
        if (binHit && this._tooltipEl) {
            const sign = binHit.value >= 0 ? '+' : '';
            this._tooltipEl.textContent = `${binHit.wire.name} (${binHit.face}): σ = ${sign}${binHit.value.toFixed(3)} fF/µm²`;
            this._tooltipEl.style.left = `${Math.min(this._cssWidth - 180, pt.x + 12)}px`;
            this._tooltipEl.style.top = `${Math.max(8, pt.y - 28)}px`;
            this._tooltipEl.classList.remove('hidden');
        } else if (this._tooltipEl) {
            this._tooltipEl.classList.add('hidden');
        }
        this._draw();
    }

    _clampHorizontalToSpacing(wire, prev, mode) {
        if (wire.z !== prev.z || wire.height !== prev.height) {
            return false;
        }
        const z0 = wire.z;
        const z1 = wire.z + wire.height;
        let minLeftEdge = -Infinity;
        let maxRightEdge = Infinity;
        for (const other of this._wires) {
            if (other.id === wire.id) continue;
            if (!(z0 < other.z + other.height && z1 > other.z)) continue;
            const sp = minSpacingBetween(wire, other, this._itf);
            const ox0 = other.x - other.width / 2;
            const ox1 = other.x + other.width / 2;
            const prevX0 = prev.x - prev.width / 2;
            const prevX1 = prev.x + prev.width / 2;
            if (ox1 <= prevX0 + 1e-6) {
                minLeftEdge = Math.max(minLeftEdge, ox1 + sp);
            } else if (ox0 >= prevX1 - 1e-6) {
                maxRightEdge = Math.min(maxRightEdge, ox0 - sp);
            }
        }
        if (mode === 'move' && !wire.isTarget) {
            const half = wire.width / 2;
            const loX = minLeftEdge + half;
            const hiX = maxRightEdge - half;
            if (loX > hiX) return false;
            wire.x = roundCoord(Math.max(loX, Math.min(hiX, wire.x)));
            return true;
        }
        if (mode === 'resize-left' || mode === 'resize-right') {
            const minW = minWidthForWire(wire, this._itf);
            if (wire.isTarget) {
                const maxHalf = Math.min(
                    Number.isFinite(minLeftEdge) ? -minLeftEdge : Infinity,
                    Number.isFinite(maxRightEdge) ? maxRightEdge : Infinity);
                const half = Math.max(minW / 2, Math.min(maxHalf, wire.width / 2));
                if (2 * half < minW - 1e-6) return false;
                wire.width = roundCoord(2 * half);
                wire.x = 0.0;
                return true;
            }
            if (mode === 'resize-left') {
                const x1 = prev.x + prev.width / 2;
                const x0 = Math.max(minLeftEdge, Math.min(x1 - minW, wire.x - wire.width / 2));
                if (x1 - x0 < minW - 1e-6) return false;
                wire.width = roundCoord(x1 - x0);
                wire.x = roundCoord((x0 + x1) / 2);
                return true;
            }
            const x0 = prev.x - prev.width / 2;
            const x1 = Math.min(maxRightEdge, Math.max(x0 + minW, wire.x + wire.width / 2));
            if (x1 - x0 < minW - 1e-6) return false;
            wire.width = roundCoord(x1 - x0);
            wire.x = roundCoord((x0 + x1) / 2);
            return true;
        }
        return false;
    }

    _onPointerUp(e) {
        if (!this._drag) return;
        const drag = this._drag;
        const wasGeometryDrag = drag.mode !== 'pan';
        this._drag = null;
        if (this._canvas.releasePointerCapture) {
            try { this._canvas.releasePointerCapture(e.pointerId); } catch (_) { /* ignore */ }
        }
        if (wasGeometryDrag) {
            this._updateWireList();
            if (this._solveTimer) {
                this._scheduleSolve(0);
            }
        } else {
            const pt = this._canvasCoords(e);
            const moved = Math.hypot(
                pt.x - drag.startScreen.x, pt.y - drag.startScreen.y);
            if (moved < 4) {
                this.selectWire(null);
            }
        }
    }

    _onPointerLeave() {
        this._hover = null;
        this._hoverBin = null;
        if (this._tooltipEl) this._tooltipEl.classList.add('hidden');
        this._draw();
    }

    _onDoubleClick(e) {
        const pt = this._canvasCoords(e);
        if (this._hitTest(pt.x, pt.y)) return;
        const world = this.screenToWorld(pt.x, pt.y);
        if (world.z <= MIN_WIRE_Z) return;
        this.addWire(world);
    }

    _onWheel(e) {
        e.preventDefault();
        const pt = this._canvasCoords(e);
        const before = this.screenToWorld(pt.x, pt.y);
        const factor = e.deltaY < 0 ? 1.15 : 1 / 1.15;
        this._view.scale = Math.max(40, Math.min(3000, this._view.scale * factor));
        const after = this.screenToWorld(pt.x, pt.y);
        this._view.centerX += before.x - after.x;
        this._view.centerZ += before.z - after.z;
        this._draw();
    }

    // ─── Canvas Rendering ───────────────────────────────────────────────────

    _draw() {
        const canvas = this._canvas;
        if (!canvas) return;
        const ctx = canvas.getContext && canvas.getContext('2d');
        if (!ctx) return;

        const w = this._cssWidth || 600;
        const h = this._cssHeight || 420;
        const dpr = this._dpr || 1;

        ctx.save();
        ctx.scale(dpr, dpr);

        const theme = getThemeColors();
        const isDark = theme.isDark !== false;
        const bg = isDark ? '#0e141a' : '#f7f9fb';
        const gridColor = isDark ? 'rgba(255, 255, 255, 0.06)' : 'rgba(0, 0, 0, 0.07)';
        const axisColor = isDark ? 'rgba(255, 255, 255, 0.18)' : 'rgba(0, 0, 0, 0.22)';
        const textMuted = isDark ? '#8fa3b0' : '#556470';
        const textPrimary = isDark ? '#eef4f8' : '#16202a';

        ctx.fillStyle = bg;
        ctx.fillRect(0, 0, w, h);

        if (this._itf && this._showDielectrics) {
            this._drawPlanarDielectrics(ctx, w, h, isDark, textMuted);
        }
        this._drawGrid(ctx, w, h, gridColor, axisColor, textMuted);
        if (this._itf) {
            this._drawItfLayerGuides(ctx, w, h, isDark, textMuted);
        }
        this._drawGroundPlane(ctx, w, h, isDark, textMuted);

        if (this._itf && this._showDielectrics) {
            this._drawConformalDielectrics(ctx, isDark);
        }

        // Compute global max |charge| across displayed wires for consistent scaling.
        const maxAbsCharge = this._computeMaxAbsCharge();

        // Draw induced ground-plane charge along z = 0.
        if (this._showNeighborCharges && this._result?.ground_charge && maxAbsCharge > 0) {
            this._drawGroundCharge(ctx, maxAbsCharge);
        }

        // Draw outward normal charge density profiles behind wire edges.
        if (this._result && Array.isArray(this._result.charges) && maxAbsCharge > 0) {
            if (this._showProfiles) {
                this._drawChargeProfiles(ctx, maxAbsCharge);
            }
        }

        // Draw wire bodies and surface charge color bands.
        for (let i = 0; i < this._wires.length; i++) {
            this._drawWire(ctx, i, isDark, textPrimary, textMuted, maxAbsCharge);
        }

        // Draw colorbar legend when a charge distribution is present.
        if (maxAbsCharge > 0) {
            this._drawColorbar(ctx, w, h, maxAbsCharge, isDark, textMuted);
        }

        ctx.restore();
    }

    _computeMaxAbsCharge() {
        if (!this._result || !Array.isArray(this._result.charges)) return 0;
        let maxAbs = 0;
        for (let i = 0; i < this._wires.length && i < this._result.charges.length; i++) {
            const wire = this._wires[i];
            if (!wire.isTarget && !this._showNeighborCharges) continue;
            const ch = this._result.charges[i];
            for (const face of ['bottom', 'top', 'left', 'right']) {
                const arr = ch[face];
                if (!Array.isArray(arr)) continue;
                for (const v of arr) {
                    if (Number.isFinite(v)) {
                        maxAbs = Math.max(maxAbs, Math.abs(v));
                    }
                }
            }
        }
        if (this._showNeighborCharges && this._result.ground_charge
            && Array.isArray(this._result.ground_charge.values)) {
            for (const v of this._result.ground_charge.values) {
                if (Number.isFinite(v)) {
                    maxAbs = Math.max(maxAbs, Math.abs(v));
                }
            }
        }
        return maxAbs;
    }

    _drawGroundCharge(ctx, maxAbs) {
        const gch = this._result?.ground_charge;
        if (!gch || !Array.isArray(gch.values) || gch.values.length === 0) return;
        const vals = gch.values;
        const n = vals.length;
        const leftPt = this.worldToScreen(gch.x_lo, 0);
        const rightPt = this.worldToScreen(gch.x_hi, 0);
        const gndY = leftPt.y;
        const spanPx = rightPt.x - leftPt.x;
        if (spanPx <= 1) return;

        // Outward normal profile projecting upward from z = 0 into the dielectric
        // (matching the outward normal convention used on all wire faces).
        if (this._showProfiles) {
            const maxLobePx = 44;
            const pts = [];
            for (let k = 0; k < n; k++) {
                const t = (k + 0.5) / n;
                const mag = Math.min(1, Math.abs(vals[k]) / maxAbs) * maxLobePx;
                pts.push({ x: leftPt.x + t * spanPx, y: gndY - mag });
            }
            ctx.beginPath();
            ctx.moveTo(leftPt.x, gndY);
            ctx.lineTo(leftPt.x, pts[0].y);
            for (const p of pts) ctx.lineTo(p.x, p.y);
            ctx.lineTo(rightPt.x, pts[n - 1].y);
            ctx.lineTo(rightPt.x, gndY);
            ctx.closePath();
            ctx.fillStyle = 'rgba(41, 182, 246, 0.22)';
            ctx.fill();

            ctx.lineWidth = 1.75;
            for (let k = 0; k < n - 1; k++) {
                const midVal = 0.5 * (vals[k] + vals[k + 1]);
                ctx.beginPath();
                ctx.moveTo(pts[k].x, pts[k].y);
                ctx.lineTo(pts[k + 1].x, pts[k + 1].y);
                ctx.strokeStyle = chargeToColor(midVal, maxAbs, 0.95);
                ctx.stroke();
            }
        }

        // Surface charge color band right along the ground plane line z = 0.
        const band = 6;
        const segW = spanPx / n;
        for (let k = 0; k < n; k++) {
            ctx.fillStyle = chargeToColor(vals[k], maxAbs, 0.95);
            ctx.fillRect(leftPt.x + k * segW, gndY - band / 2, Math.ceil(segW), band);
        }
    }

    _drawPlanarDielectrics(ctx, w, h, isDark, textMuted) {
        const diels = this._itf?.dielectrics;
        if (!Array.isArray(diels) || diels.length === 0) return;

        for (const d of diels) {
            const yTop = this.worldToScreen(0, d.top).y;
            const yBot = this.worldToScreen(0, d.bottom).y;
            if (yBot < 0 || yTop > h) continue;
            const bandH = yBot - yTop;

            ctx.fillStyle = dielectricColor(d.eps, isDark, isDark ? 0.20 : 0.16);
            ctx.fillRect(0, yTop, w, bandH);

            // Interface boundary line at top of dielectric layer.
            ctx.save();
            ctx.strokeStyle = isDark ? 'rgba(200, 220, 235, 0.22)' : 'rgba(40, 60, 80, 0.22)';
            ctx.setLineDash([5, 4]);
            ctx.lineWidth = 1;
            ctx.beginPath();
            ctx.moveTo(0, Math.round(yTop) + 0.5);
            ctx.lineTo(w, Math.round(yTop) + 0.5);
            ctx.stroke();
            ctx.restore();

            // Label on right side when band is tall enough.
            if (bandH >= 14) {
                ctx.fillStyle = textMuted;
                ctx.font = '10px ui-monospace, Menlo, Consolas, monospace';
                ctx.textAlign = 'right';
                ctx.textBaseline = 'middle';
                ctx.fillText(
                    `${d.name} (ε=${d.eps})`,
                    w - 10,
                    yTop + bandH / 2);
            }
        }

        // Background dielectric above the top planar dielectric.
        const topZ = diels[diels.length - 1].top;
        const topY = this.worldToScreen(0, topZ).y;
        if (topY > 18) {
            ctx.fillStyle = textMuted;
            ctx.font = '10px ui-monospace, Menlo, Consolas, monospace';
            ctx.textAlign = 'right';
            ctx.textBaseline = 'bottom';
            ctx.fillText(
                `Background (ε=${this._itf.background_eps})`,
                w - 10,
                topY - 4);
        }
    }

    _drawConformalDielectrics(ctx, isDark) {
        for (const wire of this._wires) {
            const layer = this._findItfLayer(wire.layer);
            if (!layer || !Array.isArray(layer.conformals) || layer.conformals.length === 0) {
                continue;
            }
            // Cumulative thicknesses from innermost to outermost, drawn outer first.
            const shells = [];
            let sw = 0;
            let tw = 0;
            for (const c of layer.conformals) {
                sw += c.side_thickness || 0;
                tw += c.top_thickness || 0;
                shells.push({ sw, tw, eps: c.eps, name: c.name });
            }
            for (let k = shells.length - 1; k >= 0; k--) {
                const s = shells[k];
                const tl = this.worldToScreen(
                    wire.x - wire.width / 2 - s.sw,
                    wire.z + wire.height + s.tw);
                const br = this.worldToScreen(
                    wire.x + wire.width / 2 + s.sw,
                    wire.z);
                const rw = br.x - tl.x;
                const rh = br.y - tl.y;
                ctx.fillStyle = dielectricColor(s.eps, isDark, isDark ? 0.34 : 0.28);
                ctx.fillRect(tl.x, tl.y, rw, rh);
                ctx.save();
                ctx.strokeStyle = dielectricColor(s.eps, isDark, 0.75);
                ctx.setLineDash([3, 2]);
                ctx.lineWidth = 1;
                ctx.strokeRect(tl.x, tl.y, rw, rh);
                ctx.restore();
            }
        }
    }

    _drawItfLayerGuides(ctx, w, h, isDark, textMuted) {
        const conductors = this._itf?.conductors;
        if (!Array.isArray(conductors)) return;
        ctx.save();
        ctx.font = '10px ui-monospace, Menlo, Consolas, monospace';
        ctx.textAlign = 'left';
        ctx.textBaseline = 'middle';
        for (const c of conductors) {
            const yTop = this.worldToScreen(0, c.top).y;
            const yBot = this.worldToScreen(0, c.bottom).y;
            if (yBot < 0 || yTop > h) continue;
            const yMid = 0.5 * (yTop + yBot);
            // Subtle band marker along the left edge indicating snap layer.
            ctx.fillStyle = isDark ? 'rgba(70, 201, 224, 0.22)' : 'rgba(11, 109, 132, 0.18)';
            ctx.fillRect(0, yTop, 4, Math.max(2, yBot - yTop));
            if (yBot - yTop >= 10) {
                ctx.fillStyle = textMuted;
                ctx.fillText(c.name, 8, yMid);
            }
        }
        ctx.restore();
    }

    _drawGrid(ctx, w, h, gridColor, axisColor, textMuted) {
        // Choose a clean grid step in µm.
        const targetPx = 60;
        const rawStep = targetPx / this._view.scale;
        const pow10 = Math.pow(10, Math.floor(Math.log10(rawStep)));
        const frac = rawStep / pow10;
        const step = (frac < 1.5 ? 1 : frac < 3.5 ? 2 : frac < 7.5 ? 5 : 10) * pow10;

        const topLeft = this.screenToWorld(0, 0);
        const botRight = this.screenToWorld(w, h);

        ctx.strokeStyle = gridColor;
        ctx.lineWidth = 1;
        ctx.beginPath();
        const xStart = Math.floor(topLeft.x / step) * step;
        for (let x = xStart; x <= botRight.x + step; x += step) {
            const sx = Math.round(this.worldToScreen(x, 0).x) + 0.5;
            ctx.moveTo(sx, 0);
            ctx.lineTo(sx, h);
        }
        const zStart = Math.max(0, Math.floor(botRight.z / step) * step);
        for (let z = zStart; z <= topLeft.z + step; z += step) {
            const sy = Math.round(this.worldToScreen(0, z).y) + 0.5;
            ctx.moveTo(0, sy);
            ctx.lineTo(w, sy);
        }
        ctx.stroke();

        // Dashed vertical centerline at x = 0 (target wire symmetry axis).
        const x0 = Math.round(this.worldToScreen(0, 0).x) + 0.5;
        ctx.save();
        ctx.strokeStyle = axisColor;
        ctx.setLineDash([4, 4]);
        ctx.beginPath();
        ctx.moveTo(x0, 0);
        ctx.lineTo(x0, this.worldToScreen(0, 0).y);
        ctx.stroke();
        ctx.restore();

        // Axis labels along left edge (z) — omitted when ITF layer guides occupy
        // the left edge, keeping the margin uncluttered.
        ctx.fillStyle = textMuted;
        ctx.font = '10px ui-monospace, Menlo, Consolas, monospace';
        if (!this._itf) {
            ctx.textAlign = 'left';
            ctx.textBaseline = 'middle';
            for (let z = step; z <= topLeft.z; z += step) {
                const sy = this.worldToScreen(0, z).y;
                if (sy > 16 && sy < h - 24) {
                    ctx.fillText(`${z.toFixed(step < 0.1 ? 2 : 1)} µm`, 6, sy);
                }
            }
        }
        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';
        ctx.fillText('x = 0', x0, 6);
    }

    _drawGroundPlane(ctx, w, h, isDark, textMuted) {
        const gndY = this.worldToScreen(0, 0).y;
        if (gndY >= h) return;

        // Shaded substrate below z = 0.
        ctx.fillStyle = isDark ? 'rgba(30, 42, 52, 0.7)' : 'rgba(205, 215, 224, 0.7)';
        ctx.fillRect(0, gndY, w, h - gndY);

        // Diagonal hatch lines.
        ctx.save();
        ctx.beginPath();
        ctx.rect(0, gndY, w, Math.min(28, h - gndY));
        ctx.clip();
        ctx.strokeStyle = isDark ? 'rgba(140, 170, 190, 0.25)' : 'rgba(60, 80, 100, 0.25)';
        ctx.lineWidth = 1;
        ctx.beginPath();
        for (let x = -30; x < w + 30; x += 12) {
            ctx.moveTo(x, gndY);
            ctx.lineTo(x - 18, gndY + 24);
        }
        ctx.stroke();
        ctx.restore();

        // Ground line.
        ctx.strokeStyle = isDark ? '#78909c' : '#546e7a';
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.moveTo(0, gndY);
        ctx.lineTo(w, gndY);
        ctx.stroke();

        ctx.fillStyle = textMuted;
        ctx.font = '11px -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif';
        ctx.textAlign = 'left';
        ctx.textBaseline = 'top';
        ctx.fillText('Ground plane (z = 0 µm, 0 V)', 8, gndY + 6);
    }

    _drawChargeProfiles(ctx, maxAbs) {
        const maxLobePx = 44;
        for (let i = 0; i < this._wires.length && i < this._result.charges.length; i++) {
            const wire = this._wires[i];
            if (!wire.isTarget && !this._showNeighborCharges) continue;
            const ch = this._result.charges[i];
            const r = this._wireScreenRect(wire);
            this._drawFaceProfile(ctx, r, 'top', ch.top, maxAbs, maxLobePx);
            this._drawFaceProfile(ctx, r, 'bottom', ch.bottom, maxAbs, maxLobePx);
            this._drawFaceProfile(ctx, r, 'left', ch.left, maxAbs, maxLobePx);
            this._drawFaceProfile(ctx, r, 'right', ch.right, maxAbs, maxLobePx);
        }
    }

    _drawFaceProfile(ctx, r, face, vals, maxAbs, maxLobePx) {
        if (!Array.isArray(vals) || vals.length === 0) return;
        const n = vals.length;
        // Mean sign of the face determines fill tint.
        let sum = 0;
        for (const v of vals) sum += v;
        const isPos = sum >= 0;
        const fill = isPos ? 'rgba(255, 112, 67, 0.24)' : 'rgba(41, 182, 246, 0.22)';
        const stroke = isPos ? 'rgba(255, 138, 101, 0.9)' : 'rgba(79, 195, 247, 0.9)';

        const pts = [];
        for (let k = 0; k < n; k++) {
            const t = (k + 0.5) / n;
            const mag = Math.min(1, Math.abs(vals[k]) / maxAbs) * maxLobePx;
            if (face === 'top') {
                pts.push({ x: r.x + t * r.w, y: r.y - mag });
            } else if (face === 'bottom') {
                pts.push({ x: r.x + t * r.w, y: r.y + r.h + mag });
            } else if (face === 'left') {
                pts.push({ x: r.x - mag, y: r.y + (1 - t) * r.h });
            } else {
                pts.push({ x: r.x + r.w + mag, y: r.y + (1 - t) * r.h });
            }
        }

        ctx.beginPath();
        if (face === 'top') {
            ctx.moveTo(r.x, r.y);
            ctx.lineTo(r.x, pts[0].y);
            for (const p of pts) ctx.lineTo(p.x, p.y);
            ctx.lineTo(r.x + r.w, pts[n - 1].y);
            ctx.lineTo(r.x + r.w, r.y);
        } else if (face === 'bottom') {
            ctx.moveTo(r.x, r.y + r.h);
            ctx.lineTo(r.x, pts[0].y);
            for (const p of pts) ctx.lineTo(p.x, p.y);
            ctx.lineTo(r.x + r.w, pts[n - 1].y);
            ctx.lineTo(r.x + r.w, r.y + r.h);
        } else if (face === 'left') {
            ctx.moveTo(r.x, r.y + r.h);
            ctx.lineTo(pts[0].x, r.y + r.h);
            for (const p of pts) ctx.lineTo(p.x, p.y);
            ctx.lineTo(pts[n - 1].x, r.y);
            ctx.lineTo(r.x, r.y);
        } else {
            ctx.moveTo(r.x + r.w, r.y + r.h);
            ctx.lineTo(pts[0].x, r.y + r.h);
            for (const p of pts) ctx.lineTo(p.x, p.y);
            ctx.lineTo(pts[n - 1].x, r.y);
            ctx.lineTo(r.x + r.w, r.y);
        }
        ctx.closePath();
        ctx.fillStyle = fill;
        ctx.fill();

        ctx.lineWidth = 1.75;
        for (let k = 0; k < n - 1; k++) {
            const midVal = 0.5 * (vals[k] + vals[k + 1]);
            ctx.beginPath();
            ctx.moveTo(pts[k].x, pts[k].y);
            ctx.lineTo(pts[k + 1].x, pts[k + 1].y);
            ctx.strokeStyle = chargeToColor(midVal, maxAbs, 0.95);
            ctx.stroke();
        }
    }

    _drawWire(ctx, index, isDark, textPrimary, textMuted, maxAbsCharge) {
        const wire = this._wires[index];
        const r = this._wireScreenRect(wire);
        const isSelected = wire.id === this._selectedId;
        const isHovered = this._hover && this._hover.wire.id === wire.id;

        // Wire body fill (opaque enough that adjacent charge profiles stay
        // cleanly in the dielectric space and never obscure wire labels).
        if (wire.isTarget) {
            ctx.fillStyle = isDark ? '#3a2717' : '#fbe6d4';
        } else {
            ctx.fillStyle = isDark ? '#182b3a' : '#dcebf6';
        }
        ctx.fillRect(r.x, r.y, r.w, r.h);

        // Wire outline.
        ctx.strokeStyle = isSelected
            ? (wire.isTarget ? '#ffb74d' : '#4fc3f7')
            : (isDark ? 'rgba(180, 200, 215, 0.5)' : 'rgba(60, 80, 100, 0.5)');
        ctx.lineWidth = isSelected ? 2 : 1.25;
        ctx.strokeRect(r.x, r.y, r.w, r.h);

        // Surface charge color band right along the 4 wire edges.
        const ch = this._result && Array.isArray(this._result.charges)
            ? this._result.charges[index] : null;
        if (ch && maxAbsCharge > 0 && (wire.isTarget || this._showNeighborCharges)) {
            this._drawSurfaceChargeBand(ctx, r, ch, maxAbsCharge);
        }

        // Resize handle indicators on left/right edges when selected or hovered.
        if (isSelected || isHovered) {
            const handleH = Math.min(18, Math.max(8, r.h * 0.45));
            const midY = r.y + r.h / 2;
            ctx.fillStyle = wire.isTarget ? '#ffb74d' : '#4fc3f7';
            ctx.fillRect(r.x - 2.5, midY - handleH / 2, 5, handleH);
            ctx.fillRect(r.x + r.w - 2.5, midY - handleH / 2, 5, handleH);
        }

        // Labels inside wire.
        if (wire.isTarget) {
            const cy = r.y + r.h / 2;
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.font = 'bold 11px -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif';
            const showName = r.h >= 18 && (ctx.measureText(wire.name).width + 6 <= r.w);
            if (showName) {
                let sub = '1V';
                if (this._result && Number.isFinite(this._result.total)) {
                    sub = `${this._result.total.toFixed(3)} fF/µm`;
                }
                ctx.font = '10px ui-monospace, Menlo, Consolas, monospace';
                const showSub = r.h >= 34 && (ctx.measureText(sub).width + 6 <= r.w);

                ctx.fillStyle = textPrimary;
                ctx.font = 'bold 11px -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif';
                ctx.fillText(wire.name, r.x + r.w / 2, showSub ? cy - 7 : cy);

                if (showSub) {
                    ctx.fillStyle = textMuted;
                    ctx.font = '10px ui-monospace, Menlo, Consolas, monospace';
                    ctx.fillText(sub, r.x + r.w / 2, cy + 8);
                }
            }
        } else {
            const cy = r.y + r.h / 2;
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';

            let sub = '0V';
            if (this._result && Array.isArray(this._result.couplings)) {
                const c = this._result.couplings.find(e => e.index === index);
                if (c && Number.isFinite(c.cap)) {
                    sub = `${c.cap.toFixed(3)} fF/µm`;
                }
            }
            ctx.font = '10px ui-monospace, Menlo, Consolas, monospace';
            const showSub = r.h >= 34 && (ctx.measureText(sub).width + 6 <= r.w);

            ctx.fillStyle = textPrimary;
            ctx.font = (r.w >= 26 ? 'bold 11px ' : 'bold 10px ')
                + '-apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif';
            ctx.fillText(wire.name, r.x + r.w / 2, showSub ? cy - 7 : cy);

            if (showSub) {
                ctx.fillStyle = textMuted;
                ctx.font = '10px ui-monospace, Menlo, Consolas, monospace';
                ctx.fillText(sub, r.x + r.w / 2, cy + 8);
            }
        }
    }

    _drawSurfaceChargeBand(ctx, r, ch, maxAbs) {
        const band = 6;
        const drawHoriz = (arr, y) => {
            if (!Array.isArray(arr) || !arr.length) return;
            const segW = r.w / arr.length;
            for (let k = 0; k < arr.length; k++) {
                ctx.fillStyle = chargeToColor(arr[k], maxAbs, 0.95);
                ctx.fillRect(r.x + k * segW, y, Math.ceil(segW), band);
            }
        };
        const drawVert = (arr, x) => {
            if (!Array.isArray(arr) || !arr.length) return;
            const segH = r.h / arr.length;
            for (let k = 0; k < arr.length; k++) {
                // k = 0 is bottom (z = lo), k = n-1 is top (z = hi)
                const y = r.y + r.h - (k + 1) * segH;
                ctx.fillStyle = chargeToColor(arr[k], maxAbs, 0.95);
                ctx.fillRect(x, y, band, Math.ceil(segH));
            }
        };
        drawHoriz(ch.top, r.y - band / 2);
        drawHoriz(ch.bottom, r.y + r.h - band / 2);
        drawVert(ch.left, r.x - band / 2);
        drawVert(ch.right, r.x + r.w - band / 2);
    }

    _drawColorbar(ctx, w, h, maxAbs, isDark, textMuted) {
        const barW = 140;
        const barH = 10;
        const pad = 12;
        const x = w - barW - pad - 8;
        const y = h - barH - pad - 24;

        ctx.fillStyle = isDark ? 'rgba(12, 18, 24, 0.82)' : 'rgba(255, 255, 255, 0.88)';
        ctx.strokeStyle = isDark ? 'rgba(255, 255, 255, 0.12)' : 'rgba(0, 0, 0, 0.12)';
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.roundRect(x - 10, y - 18, barW + 20, barH + 34, 5);
        ctx.fill();
        ctx.stroke();

        ctx.fillStyle = textMuted;
        ctx.font = '10px -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'bottom';
        ctx.fillText('Surface Charge σ (fF/µm²)', x + barW / 2, y - 4);

        const steps = 40;
        const sw = barW / steps;
        for (let i = 0; i < steps; i++) {
            const frac = (i + 0.5) / steps;
            const val = this._showNeighborCharges
                ? (frac * 2 - 1) * maxAbs
                : frac * maxAbs;
            ctx.fillStyle = chargeToColor(val, maxAbs, 1.0);
            ctx.fillRect(x + i * sw, y, Math.ceil(sw), barH);
        }

        ctx.fillStyle = textMuted;
        ctx.font = '9px ui-monospace, Menlo, Consolas, monospace';
        ctx.textBaseline = 'top';
        ctx.textAlign = 'left';
        ctx.fillText(
            this._showNeighborCharges ? `-${maxAbs.toFixed(2)}` : '0',
            x,
            y + barH + 3);
        if (this._showNeighborCharges) {
            ctx.textAlign = 'center';
            ctx.fillText('0', x + barW / 2, y + barH + 3);
        }
        ctx.textAlign = 'right';
        ctx.fillText(`+${maxAbs.toFixed(2)}`, x + barW, y + barH + 3);
    }
}
