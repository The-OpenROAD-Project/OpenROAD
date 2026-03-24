// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Canvas-based clock tree viewer widget.

import { getThemeColors } from './theme.js';

export const kNodeSpacing = 24;    // pixels between adjacent leaf bins
export const kNodeSize = 10;       // base node shape size in pixels
export const kTopMargin = 40;      // pixels above the first node
export const kBottomMargin = 40;   // pixels below the last node
export const kLeftMargin = 70;     // room for time-axis labels
export const kRightMargin = 20;

// Node type → fill colour
const kTypeColors = {
    root:       '#ff4444',
    buffer:     '#4488ff',
    inverter:   '#00b0b0',
    clock_gate: '#cc44cc',
    register:   '#ff4444',
    macro:      '#00b0b0',
    unknown:    '#888888',
};

// Pure layout computation — extracted for testability.
export function computeClockTreeLayout(clockData) {
    const nodes = clockData.nodes;
    if (!nodes || nodes.length === 0) {
        return {
            layout: [],
            layoutWidth: 0,
            layoutHeight: 0,
            timeMin: 0,
            timeMax: 0,
            timeUnit: '',
            sceneHeight: 0,
            layoutById: new Map(),
        };
    }

    // Build children map
    const childrenMap = new Map();
    for (const n of nodes) {
        childrenMap.set(n.id, []);
    }
    for (const n of nodes) {
        if (n.parent_id >= 0 && childrenMap.has(n.parent_id)) {
            childrenMap.get(n.parent_id).push(n.id);
        }
    }
    const nodeById = new Map();
    for (const n of nodes) {
        nodeById.set(n.id, n);
    }

    // Compute total leaf-equivalent width for each subtree
    const widthOf = new Map();
    const computeWidth = (id) => {
        const kids = childrenMap.get(id) || [];
        if (kids.length === 0) {
            widthOf.set(id, 1);
            return 1;
        }
        let w = 0;
        for (const kid of kids) {
            w += computeWidth(kid);
        }
        widthOf.set(id, w);
        return w;
    };

    // Find root nodes (parent_id == -1)
    const roots = nodes.filter(n => n.parent_id === -1);
    let totalWidth = 0;
    for (const r of roots) {
        totalWidth += computeWidth(r.id);
    }

    // Time range
    const minArr = clockData.min_arrival;
    const maxArr = clockData.max_arrival;
    const timeRange = maxArr - minArr || 1;
    const sceneHeight = Math.max(200, totalWidth * kNodeSpacing * 0.6);

    // Assign x positions: each node gets the centre of its allocated bins
    const layout = [];
    const assignPositions = (id, binStart) => {
        const n = nodeById.get(id);
        const w = widthOf.get(id);
        const x = kLeftMargin + (binStart + w / 2) * kNodeSpacing;
        const y = kTopMargin +
            ((n.arrival - minArr) / timeRange) * sceneHeight;

        layout.push({
            id: n.id,
            x,
            y,
            type: n.type,
            name: n.name,
            pin_name: n.pin_name,
            arrival: n.arrival,
            delay: n.delay,
            fanout: n.fanout,
            level: n.level,
            parent_id: n.parent_id,
        });

        let offset = binStart;
        const kids = childrenMap.get(id) || [];
        for (const kid of kids) {
            assignPositions(kid, offset);
            offset += widthOf.get(kid);
        }
    };

    let rootOffset = 0;
    for (const r of roots) {
        assignPositions(r.id, rootOffset);
        rootOffset += widthOf.get(r.id);
    }

    const layoutWidth = kLeftMargin + totalWidth * kNodeSpacing + kRightMargin;
    const layoutHeight = kTopMargin + sceneHeight + kBottomMargin;

    const layoutById = new Map();
    for (const item of layout) {
        layoutById.set(item.id, item);
    }

    return {
        layout,
        layoutWidth,
        layoutHeight,
        timeMin: minArr,
        timeMax: maxArr,
        timeUnit: clockData.time_unit || '',
        sceneHeight,
        layoutById,
    };
}

export class ClockTreeWidget {
    constructor(container, app, redrawAllLayers) {
        this._app = app;
        this._redrawAllLayers = redrawAllLayers;
        this._clockData = [];
        this._selectedClockIdx = 0;
        this._selectedNodeId = -1;

        // Canvas transform (pan / zoom)
        this._tx = 0;
        this._ty = 0;
        this._scale = 1;

        // Computed layout: array of {id, x, y, type, name, ...}
        this._layout = [];

        // Drag state
        this._dragging = false;
        this._dragStartX = 0;
        this._dragStartY = 0;
        this._dragStartTx = 0;
        this._dragStartTy = 0;

        this._build(container);
    }

    // ---- DOM construction ----

    _build(container) {
        const el = document.createElement('div');
        el.className = 'clock-tree-widget';

        // Toolbar
        const toolbar = document.createElement('div');
        toolbar.className = 'clock-tree-toolbar';
        this._updateBtn = document.createElement('button');
        this._updateBtn.className = 'timing-btn';
        this._updateBtn.textContent = 'Update';
        this._fitBtn = document.createElement('button');
        this._fitBtn.className = 'timing-btn';
        this._fitBtn.textContent = 'Fit';
        this._statusLabel = document.createElement('span');
        this._statusLabel.className = 'timing-path-count';
        toolbar.appendChild(this._updateBtn);
        toolbar.appendChild(this._fitBtn);
        toolbar.appendChild(this._statusLabel);
        el.appendChild(toolbar);

        // Tab bar (one tab per clock)
        this._tabBar = document.createElement('div');
        this._tabBar.className = 'timing-tab-bar';
        el.appendChild(this._tabBar);

        // Canvas
        this._canvas = document.createElement('canvas');
        this._canvas.className = 'clock-tree-canvas';
        el.appendChild(this._canvas);

        // Tooltip
        this._tooltip = document.createElement('div');
        this._tooltip.className = 'clock-tree-tooltip';
        this._tooltip.style.display = 'none';
        el.appendChild(this._tooltip);

        container.element.appendChild(el);
        this._el = el;

        this._ctx = this._canvas.getContext('2d');
        this._bindEvents();
    }

    _fit() {
        this._sizeCanvas();
        this._fitToView();
        this._render();
    }

    _bindEvents() {
        this._updateBtn.addEventListener('click', () => this.update());
        this._fitBtn.addEventListener('click', () => this._fit());

        this._canvas.addEventListener('keydown', (e) => {
            if (e.key === 'f') {
                this._fit();
            }
        });
        this._canvas.tabIndex = 0;  // make canvas focusable for key events

        this._canvas.addEventListener('wheel', (e) => {
            e.preventDefault();
            const rect = this._canvas.getBoundingClientRect();
            const mx = e.clientX - rect.left;
            const my = e.clientY - rect.top;
            const factor = e.deltaY < 0 ? 1.15 : 1 / 1.15;
            // Zoom centred on cursor
            this._tx = mx - (mx - this._tx) * factor;
            this._ty = my - (my - this._ty) * factor;
            this._scale *= factor;
            this._render();
        });

        this._canvas.addEventListener('mousedown', (e) => {
            this._dragging = true;
            this._dragStartX = e.clientX;
            this._dragStartY = e.clientY;
            this._dragStartTx = this._tx;
            this._dragStartTy = this._ty;
            this._canvas.style.cursor = 'grabbing';
        });

        window.addEventListener('mousemove', (e) => {
            if (this._dragging) {
                this._tx = this._dragStartTx + (e.clientX - this._dragStartX);
                this._ty = this._dragStartTy + (e.clientY - this._dragStartY);
                this._render();
            } else {
                this._handleHover(e);
            }
        });

        window.addEventListener('mouseup', () => {
            if (this._dragging) {
                this._dragging = false;
                this._canvas.style.cursor = 'grab';
            }
        });

        this._canvas.addEventListener('click', (e) => this._handleClick(e));

        // Resize observer to keep canvas resolution matched to layout
        this._resizeObserver = new ResizeObserver(() => {
            this._sizeCanvas();
            this._render();
        });
        this._resizeObserver.observe(this._canvas);
    }

    _sizeCanvas() {
        const rect = this._canvas.getBoundingClientRect();
        const dpr = window.devicePixelRatio || 1;
        this._canvas.width = rect.width * dpr;
        this._canvas.height = rect.height * dpr;
        this._ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    }

    // ---- Data fetching ----

    async update() {
        this._statusLabel.textContent = 'Loading...';
        try {
            const data = await this._app.websocketManager.request(
                { type: 'clock_tree' });
            this._clockData = data.clocks || [];
            this._buildTabs();
            this._selectClock(0);
            this._statusLabel.textContent =
                this._clockData.length + ' clock' +
                (this._clockData.length !== 1 ? 's' : '');
        } catch (err) {
            console.error('Clock tree error:', err);
            this._statusLabel.textContent = 'Error: ' + err.message;
        }
    }

    _buildTabs() {
        this._tabBar.innerHTML = '';
        this._clockData.forEach((clk, idx) => {
            const btn = document.createElement('button');
            btn.className = 'timing-tab' +
                (idx === this._selectedClockIdx ? ' active' : '');
            btn.textContent = clk.name;
            btn.addEventListener('click', () => this._selectClock(idx));
            this._tabBar.appendChild(btn);
        });
    }

    _selectClock(idx) {
        if (idx < 0 || idx >= this._clockData.length) return;
        this._selectedClockIdx = idx;
        this._selectedNodeId = -1;

        // Update tab highlights
        const tabs = this._tabBar.querySelectorAll('.timing-tab');
        tabs.forEach((t, i) => {
            t.classList.toggle('active', i === idx);
        });

        this._computeLayout(this._clockData[idx]);
        // Defer fit+render to ensure canvas has been laid out
        requestAnimationFrame(() => {
            this._sizeCanvas();
            this._fitToView();
            this._render();
        });
    }

    // ---- Layout computation ----

    _computeLayout(clockData) {
        const result = computeClockTreeLayout(clockData);
        this._layout = result.layout;
        this._layoutWidth = result.layoutWidth;
        this._layoutHeight = result.layoutHeight;
        this._timeMin = result.timeMin;
        this._timeMax = result.timeMax;
        this._timeUnit = result.timeUnit;
        this._sceneHeight = result.sceneHeight;
        this._layoutById = result.layoutById;
    }

    _fitToView() {
        if (!this._layout.length) return;
        const rect = this._canvas.getBoundingClientRect();
        const cw = rect.width;
        const ch = rect.height;
        const sx = cw / this._layoutWidth;
        const sy = ch / this._layoutHeight;
        this._scale = Math.min(sx, sy) * 0.9;
        this._tx = (cw - this._layoutWidth * this._scale) / 2;
        this._ty = (ch - this._layoutHeight * this._scale) / 2;
    }

    // ---- Rendering ----

    render() { this._render(); }

    _render() {
        const ctx = this._ctx;
        const rect = this._canvas.getBoundingClientRect();
        const w = rect.width;
        const h = rect.height;
        const tc = getThemeColors();
        ctx.clearRect(0, 0, w, h);

        // Background
        ctx.fillStyle = tc.canvasBg;
        ctx.fillRect(0, 0, w, h);

        if (!this._layout.length) {
            ctx.fillStyle = tc.canvasText;
            ctx.font = '14px monospace';
            ctx.textAlign = 'center';
            ctx.fillText('Click "Update" to load clock tree data',
                w / 2, h / 2);
            return;
        }

        ctx.save();
        ctx.translate(this._tx, this._ty);
        ctx.scale(this._scale, this._scale);

        // Draw connections first (behind nodes)
        this._drawConnections(ctx);

        // Draw nodes
        for (const item of this._layout) {
            this._drawNode(ctx, item, item.id === this._selectedNodeId);
        }

        ctx.restore();

        // Draw time scale (in screen coordinates, on the left)
        this._drawTimeScale(ctx, w, h, tc);
    }

    _drawConnections(ctx) {
        const lw = 1.5 / this._scale;
        const timeRange = this._timeMax - this._timeMin || 1;

        for (const item of this._layout) {
            if (item.parent_id < 0) continue;
            const parent = this._layoutById.get(item.parent_id);
            if (!parent) continue;

            const px = parent.x;
            const py = parent.y + kNodeSize / 2;  // bottom of parent node
            const cx = item.x;
            const cy = item.y - kNodeSize / 2;    // top of child node

            // Cell delay: vertical segment below parent node
            const cellDelayPx = (parent.delay || 0) / timeRange
                * this._sceneHeight;
            const splitY = py + cellDelayPx;

            // Cell delay segment (orange)
            if (cellDelayPx > 0) {
                ctx.strokeStyle = '#886644';
                ctx.lineWidth = lw;
                ctx.beginPath();
                ctx.moveTo(px, py);
                ctx.lineTo(px, splitY);
                ctx.stroke();
            }

            // Wire delay segment (green Bezier from splitY to child)
            ctx.strokeStyle = '#338833';
            ctx.lineWidth = lw;
            const midY = (splitY + cy) / 2;
            ctx.beginPath();
            ctx.moveTo(px, splitY);
            ctx.bezierCurveTo(px, midY, cx, midY, cx, cy);
            ctx.stroke();
        }
    }

    _drawNode(ctx, item, selected) {
        const x = item.x;
        const y = item.y;
        const s = kNodeSize;
        const color = kTypeColors[item.type] || kTypeColors.unknown;

        ctx.fillStyle = color;

        switch (item.type) {
            case 'root':
            case 'buffer':
                // Triangle pointing down
                ctx.beginPath();
                ctx.moveTo(x - s / 2, y - s / 2);
                ctx.lineTo(x + s / 2, y - s / 2);
                ctx.lineTo(x, y + s / 2);
                ctx.closePath();
                ctx.fill();
                break;
            case 'inverter':
                // Triangle with small circle at tip
                ctx.beginPath();
                ctx.moveTo(x - s / 2, y - s / 2);
                ctx.lineTo(x + s / 2, y - s / 2);
                ctx.lineTo(x, y + s / 2);
                ctx.closePath();
                ctx.fill();
                ctx.beginPath();
                ctx.arc(x, y + s / 2 + 2, 2, 0, Math.PI * 2);
                ctx.fill();
                break;
            case 'clock_gate':
                // Circle
                ctx.beginPath();
                ctx.arc(x, y, s / 2, 0, Math.PI * 2);
                ctx.fill();
                break;
            case 'register':
            case 'macro':
                // Rectangle
                ctx.fillRect(x - s / 2, y - s / 4, s, s / 2);
                break;
            default:
                // Diamond
                ctx.beginPath();
                ctx.moveTo(x, y - s / 2);
                ctx.lineTo(x + s / 2, y);
                ctx.lineTo(x, y + s / 2);
                ctx.lineTo(x - s / 2, y);
                ctx.closePath();
                ctx.fill();
                break;
        }

        if (selected) {
            ctx.strokeStyle = '#ffff00';
            ctx.lineWidth = 2 / this._scale;
            ctx.beginPath();
            ctx.arc(x, y, s / 2 + 3, 0, Math.PI * 2);
            ctx.stroke();
        }
    }

    _drawTimeScale(ctx, canvasW, canvasH, tc) {
        if (!this._sceneHeight) return;

        // Map layout Y to screen Y
        const yToScreen = (layoutY) => layoutY * this._scale + this._ty;

        const topY = yToScreen(kTopMargin);
        const botY = yToScreen(kTopMargin + this._sceneHeight);

        // Only draw if visible
        if (botY < 0 || topY > canvasH) return;

        ctx.save();
        ctx.fillStyle = tc.canvasLabel;
        ctx.strokeStyle = tc.canvasAxis;
        ctx.font = '11px monospace';
        ctx.textAlign = 'right';
        ctx.textBaseline = 'middle';

        const range = this._timeMax - this._timeMin;
        if (range <= 0) {
            ctx.restore();
            return;
        }

        // Choose nice tick interval
        const pixelRange = botY - topY;
        const targetTicks = Math.max(2, Math.floor(pixelRange / 40));
        const rawInterval = range / targetTicks;
        const mag = Math.pow(10, Math.floor(Math.log10(rawInterval)));
        let interval = mag;
        if (rawInterval / mag > 5) interval = 10 * mag;
        else if (rawInterval / mag > 2) interval = 5 * mag;
        else if (rawInterval / mag > 1) interval = 2 * mag;

        const startVal = Math.ceil(this._timeMin / interval) * interval;

        ctx.beginPath();
        for (let val = startVal; val <= this._timeMax; val += interval) {
            const frac = (val - this._timeMin) / range;
            const sy = topY + frac * (botY - topY);
            ctx.moveTo(0, sy);
            ctx.lineTo(canvasW, sy);

            const label = val.toFixed(
                interval < 0.01 ? 3 : interval < 0.1 ? 2 : 1)
                + ' ' + this._timeUnit;
            ctx.fillText(label, kLeftMargin - 4, sy);
        }
        ctx.stroke();
        ctx.restore();
    }

    // ---- Interaction ----

    _screenToLayout(screenX, screenY) {
        return {
            x: (screenX - this._tx) / this._scale,
            y: (screenY - this._ty) / this._scale,
        };
    }

    _hitTest(screenX, screenY) {
        const p = this._screenToLayout(screenX, screenY);
        const hitRadius = kNodeSize;
        for (const item of this._layout) {
            const dx = p.x - item.x;
            const dy = p.y - item.y;
            if (dx * dx + dy * dy < hitRadius * hitRadius) {
                return item;
            }
        }
        return null;
    }

    _handleClick(e) {
        const rect = this._canvas.getBoundingClientRect();
        const mx = e.clientX - rect.left;
        const my = e.clientY - rect.top;
        const hit = this._hitTest(mx, my);

        if (hit) {
            this._selectedNodeId = hit.id;
            this._render();
            // Highlight on the layout map
            this._app.websocketManager.request({
                type: 'clock_tree_highlight',
                inst_name: hit.name,
            }).then(() => this._redrawAllLayers());
        } else {
            if (this._selectedNodeId >= 0) {
                this._selectedNodeId = -1;
                this._render();
                this._app.websocketManager.request({
                    type: 'clock_tree_highlight',
                    inst_name: '',
                }).then(() => this._redrawAllLayers());
            }
        }
    }

    _handleHover(e) {
        const rect = this._canvas.getBoundingClientRect();
        const mx = e.clientX - rect.left;
        const my = e.clientY - rect.top;

        // Only handle if mouse is over our canvas
        if (mx < 0 || my < 0 ||
            mx > rect.width || my > rect.height) {
            this._tooltip.style.display = 'none';
            return;
        }

        const hit = this._hitTest(mx, my);
        if (hit) {
            const lines = [
                hit.name,
                'Type: ' + hit.type,
                'Arrival: ' + hit.arrival.toFixed(3) +
                    ' ' + (this._timeUnit || ''),
            ];
            if (hit.delay) {
                lines.push('Delay: ' + hit.delay.toFixed(3) +
                    ' ' + (this._timeUnit || ''));
            }
            if (hit.fanout) {
                lines.push('Fanout: ' + hit.fanout);
            }
            this._tooltip.textContent = lines.join('\n');
            this._tooltip.style.display = 'block';
            this._tooltip.style.left = (e.clientX + 12) + 'px';
            this._tooltip.style.top = (e.clientY + 12) + 'px';
        } else {
            this._tooltip.style.display = 'none';
        }
    }
}
