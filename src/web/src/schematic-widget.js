// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// NetlistsVG is used to render a Yosys-compatible JSON netlist into an SVG.
// It is loaded via <script> tags in index.html and exposed as window.netlistsvg.

export class SchematicWidget {
    constructor(container, appState) {
        this.container = container;
        this.appState = appState;

        this.element = document.createElement('div');
        this.element.className = 'schematic-widget';
        this.element.style.cssText = 'width:100%; height:100%; display:flex; flex-direction:column; background:var(--bg-panel); color:var(--fg-primary); overflow:hidden;';

        // ── Controls bar ────────────────────────────────────────────────────
        this.controls = document.createElement('div');
        this.controls.style.cssText =
            'padding:4px 8px; background:var(--bg-header); border-bottom:1px solid var(--border);' +
            'flex-shrink:0; display:flex; align-items:center; gap:8px; flex-wrap:wrap;';
        this.controls.innerHTML =
            '<label style="font-size:12px;">Fanin</label>' +
            '<input id="schematic-fanin-depth"  type="number" value="1" min="0" max="10" style="width:40px;">' +
            '<label style="font-size:12px;">Fanout</label>' +
            '<input id="schematic-fanout-depth" type="number" value="1" min="0" max="10" style="width:40px;">' +
            '<label for="schematic-view-style" style="font-size:12px;">View</label>' +
            '<select id="schematic-view-style" title="Choose standard logic symbols or original cell boxes">' +
            '<option value="symbols">Standard symbols</option>' +
            '<option value="boxes">Boxes</option>' +
            '</select>' +
            '<button id="schematic-refresh">Refresh</button>' +
            '<button id="schematic-fit">Fit</button>' +
            '<button id="schematic-zoom-in"  title="Zoom in">+</button>'  +
            '<button id="schematic-zoom-out" title="Zoom out">−</button>' +
            '<button id="schematic-select" title="Select mode" style="min-width:64px">Select</button>' +
            '<button id="schematic-zoom-to" title="Zoom to selected cell" disabled>Zoom To</button>' +
            '<span id="schematic-status" style="color:var(--fg-muted); flex:1;">Select an instance in the layout to view its schematic.</span>';
        this.element.appendChild(this.controls);

        // ── SVG viewport (overflow hidden; pan/zoom via CSS transform) ──────
        this.svgContainer = document.createElement('div');
        this.svgContainer.style.cssText =
            'flex:1; overflow:hidden; position:relative; cursor:grab; background:var(--bg-main);';
        this.element.appendChild(this.svgContainer);

        this.container.element.appendChild(this.element);

        // Button listeners
        this.controls.querySelector('#schematic-refresh').addEventListener('click', () => this.refresh());
        this.controls.querySelector('#schematic-view-style').addEventListener('change', () => {
            if (this._currentNetlist) {
                this.renderNetlist(this._currentNetlist);
            }
        });
        this.controls.querySelector('#schematic-fit').addEventListener('click', () => this.fitView());
        this.controls.querySelector('#schematic-zoom-in').addEventListener('click', () => this._zoomStep(1.5));
        this.controls.querySelector('#schematic-zoom-out').addEventListener('click', () => this._zoomStep(1 / 1.5));
        this.controls.querySelector('#schematic-select').addEventListener('click', () => this._toggleSelectMode());
        this.controls.querySelector('#schematic-zoom-to').addEventListener('click', () => this._zoomToSelected());

        this.appState.schematicWidget = this;

        this.netlistsvg = null;
        this.skin = null;
        this._svgEl = null;
        this._currentNetlist = null;

        // Pan/zoom state
        this._scale = 1;
        this._panX = 0;
        this._panY = 0;

        // Select mode state
        this._selectMode = false;
        this._selectedCell = null;
        this._lastDoubleClickTime = 0;
        this._svgCellHitRecords = [];

        // Map from SVG element id → ODB instance name.
        // netlistsvg prefixes instance names (e.g. "load2" → id="cell_load2"),
        // so we build this after each render to resolve the real name.
        this._svgIdToInstName = new Map();

        this._setupPanZoom();
        this._netlistsvgReady = false;
        this.initNetlistSVG();
    }

    // ── Pan / zoom ───────────────────────────────────────────────────────────

    _setupPanZoom() {
        const c = this.svgContainer;
        let dragging = false, startX, startY, startPanX, startPanY;

        c.addEventListener('wheel', (e) => {
            if (!this._svgEl) return;
            e.preventDefault();
            const factor = e.deltaY < 0 ? 1.2 : 1 / 1.2;
            const rect = c.getBoundingClientRect();
            const mx = e.clientX - rect.left;
            const my = e.clientY - rect.top;
            this._panX = mx + (this._panX - mx) * factor;
            this._panY = my + (this._panY - my) * factor;
            this._scale *= factor;
            this._applyTransform();
        }, { passive: false });

        c.addEventListener('dblclick', (e) => this._handleCellDoubleClick(e));

        c.addEventListener('mousedown', (e) => {
            if (!this._svgEl || e.button !== 0) return;
            if (e.detail >= 2 && this._handleCellDoubleClick(e)) {
                return;
            }
            if (this._selectMode) {
                // In select mode a click selects a cell; pan is suppressed.
                this._handleSelectClick(e);
                return;
            }
            dragging = true;
            startX = e.clientX; startY = e.clientY;
            startPanX = this._panX; startPanY = this._panY;
            c.style.cursor = 'grabbing';
            e.preventDefault();
        });

        window.addEventListener('mousemove', (e) => {
            if (!dragging) return;
            this._panX = startPanX + (e.clientX - startX);
            this._panY = startPanY + (e.clientY - startY);
            this._applyTransform();
        });

        window.addEventListener('mouseup', () => {
            if (dragging) {
                dragging = false;
                c.style.cursor = this._selectMode ? 'default' : 'grab';
            }
        });
    }

    _zoomStep(factor) {
        if (!this._svgEl) return;
        const cRect = this.svgContainer.getBoundingClientRect();
        const cx = cRect.width / 2;
        const cy = cRect.height / 2;
        this._panX = cx + (this._panX - cx) * factor;
        this._panY = cy + (this._panY - cy) * factor;
        this._scale *= factor;
        this._applyTransform();
    }

    _toggleSelectMode() {
        this._selectMode = !this._selectMode;
        const btn = this.controls.querySelector('#schematic-select');
        if (this._selectMode) {
            btn.style.background = 'var(--accent)';
            btn.style.color = 'var(--fg-white)';
            this.svgContainer.style.cursor = 'default';
        } else {
            btn.style.background = '';
            btn.style.color = '';
            this.svgContainer.style.cursor = 'grab';
            this._clearCellHighlight();
        }
    }

    _cellIdFromElement(el) {
        if (!el) {
            return null;
        }
        if (el.id
            && (this._svgIdToInstName.has(el.id)
                || el.id.startsWith('cell_'))) {
            return el.id;
        }

        const classAttr = el.getAttribute && el.getAttribute('class');
        if (!classAttr) {
            return null;
        }
        for (const token of classAttr.split(/\s+/)) {
            if (token
                && (this._svgIdToInstName.has(token)
                    || token.startsWith('cell_'))) {
                return token;
            }
        }
        return null;
    }

    _closestGroup(el) {
        while (el && el !== this._svgEl) {
            const tagName = el.tagName && el.tagName.toLowerCase();
            if (tagName === 'g') {
                return el;
            }
            el = el.parentElement;
        }
        return null;
    }

    _cellGroupForSvgId(cellId) {
        if (!this._svgEl || !cellId) {
            return null;
        }

        const byId = this._svgEl.querySelector(`#${CSS.escape(cellId)}`);
        if (byId) {
            return this._closestGroup(byId) || byId;
        }

        const byClass = this._svgEl.querySelector(`.${CSS.escape(cellId)}`);
        return byClass ? this._closestGroup(byClass) : null;
    }

    _cellHitFromEventTarget(target) {
        if (!this._svgEl) {
            return null;
        }

        let el = target;
        while (el && el !== this._svgEl) {
            const cellId = this._cellIdFromElement(el);
            if (cellId) {
                return {
                    cellId,
                    group: this._cellGroupForSvgId(cellId)
                        || this._closestGroup(el),
                };
            }
            el = el.parentElement;
        }
        return null;
    }

    _rectContainsPoint(rect, x, y) {
        return rect
            && rect.width >= 0
            && rect.height >= 0
            && x >= rect.left
            && x <= rect.right
            && y >= rect.top
            && y <= rect.bottom;
    }

    _cellHitFromPoint(clientX, clientY) {
        if (!Number.isFinite(clientX) || !Number.isFinite(clientY)) {
            return null;
        }

        let best = null;
        for (const record of this._svgCellHitRecords) {
            for (const element of record.hitElements) {
                const rect = element.getBoundingClientRect();
                if (!this._rectContainsPoint(rect, clientX, clientY)) {
                    continue;
                }

                const area = Math.max(rect.width, 1) * Math.max(rect.height, 1);
                if (!best || area < best.area) {
                    best = { ...record, area };
                }
            }
        }
        return best;
    }

    _cellHitFromEvent(e) {
        return this._cellHitFromEventTarget(e.target)
            || this._cellHitFromPoint(e.clientX, e.clientY);
    }

    _instanceNameForCellId(cellId) {
        if (!cellId) {
            return null;
        }
        if (this._svgIdToInstName.has(cellId)) {
            return this._svgIdToInstName.get(cellId);
        }

        const cells = this._currentNetlist && this._currentNetlist.modules
                    && this._currentNetlist.modules.top
                    && this._currentNetlist.modules.top.cells || {};
        if (cellId.startsWith('cell_')) {
            const instName = cellId.slice('cell_'.length);
            if (Object.prototype.hasOwnProperty.call(cells, instName)) {
                return instName;
            }
        }
        if (Object.prototype.hasOwnProperty.call(cells, cellId)) {
            return cellId;
        }
        return null;
    }

    _highlightCellGroup(cellGroup) {
        this._clearCellHighlight();
        if (!cellGroup) {
            return false;
        }

        try {
            const bb = cellGroup.getBBox();
            const pad = 3;
            const highlight = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
            highlight.setAttribute('x', bb.x - pad);
            highlight.setAttribute('y', bb.y - pad);
            highlight.setAttribute('width',  bb.width  + pad * 2);
            highlight.setAttribute('height', bb.height + pad * 2);
            highlight.setAttribute('fill', 'none');
            highlight.setAttribute('stroke', '#e05a00');
            highlight.setAttribute('stroke-width', '1.5');
            highlight.setAttribute('rx', '3');
            highlight.id = '_schematic_highlight';
            cellGroup.appendChild(highlight);
            this._selectedCell = cellGroup;
            this.controls.querySelector('#schematic-zoom-to').disabled = false;
            return true;
        } catch (_) {
            // getBBox can fail on hidden elements.
            return false;
        }
    }

    _handleSelectClick(e) {
        if (!this._svgEl) return;

        // netlistsvg renders each cell as <g id="cell_instName" transform="...">
        // Walk up from the click target to the first <g> whose id maps to a
        // known ODB instance (i.e. is present in _svgIdToInstName).
        // This ensures we never try to inspect port/net/wire groups, which
        // would either give "Instance not found" or an unexpected request type.
        const hit = this._cellHitFromEvent(e);
        if (!hit) return;

        const name = this._instanceNameForCellId(hit.cellId);
        if (!name) return;

        this._highlightCellGroup(hit.group);
        this.setStatus(`Selected: ${name}`);
        this._fetchInspect(name);
    }

    _handleCellDoubleClick(e) {
        if (!this._svgEl) {
            return false;
        }

        const hit = this._cellHitFromEvent(e);
        if (!hit) {
            return false;
        }

        const name = this._instanceNameForCellId(hit.cellId);
        if (!name) {
            return false;
        }

        const now = e.timeStamp || performance.now();
        if (this._lastDoubleClickTime
            && now - this._lastDoubleClickTime < 250) {
            return false;
        }
        this._lastDoubleClickTime = now;

        e.preventDefault();  //Stops the browser’s default double-click behavior
        e.stopPropagation();  //Stops the event from propagating further up the DOM tree
        this.appState.selectedInstanceName = name;  //Stores the clicked instance name as the currently selected instance
        this._highlightCellGroup(hit.group);  //Highlights the clicked cell group in the schematic
        this._fetchInspect(name);
        return this._expandFromInstance(name);
    }

    _fetchInspect(instName) {
        const wm = this.appState.websocketManager;
        if (!wm) return Promise.resolve(false);
        console.log('[Schematic] inspect:', instName);
        return wm.request({ type: 'schematic_inspect', inst_name: instName })
            .then(data => {
                if (this.appState.updateInspector) {
                    this.appState.updateInspector(data);
                }
                if (this.appState.focusComponent) {
                    this.appState.focusComponent('Inspector');
                }
                // Redraw layout tiles to show instance highlight
                if (this.appState.redrawAllLayers) {
                    this.appState.redrawAllLayers();
                }
            })
            .catch(err => {
                console.error('schematic_inspect failed:', err);
                this.setStatus(`Inspect error: ${err}`);
                return false;
            });
    }

    _clearCellHighlight() {
        if (this._svgEl) {
            const old = this._svgEl.querySelector('#_schematic_highlight');
            if (old) old.remove();
        }
        this._selectedCell = null;
        this.controls.querySelector('#schematic-zoom-to').disabled = true;
    }

    _zoomToSelected() {
        if (!this._selectedCell || !this._svgEl) return;
        const cRect = this.svgContainer.getBoundingClientRect();
        const cellRect = this._selectedCell.getBoundingClientRect();

        // Convert the cell's screen-space rect back to SVG coordinate space
        // by inverting the current translate(panX, panY) scale(scale) transform.
        const svgX = (cellRect.left - cRect.left - this._panX) / this._scale;
        const svgY = (cellRect.top  - cRect.top  - this._panY) / this._scale;
        const svgW = cellRect.width  / this._scale;
        const svgH = cellRect.height / this._scale;

        // Choose scale so the cell occupies 90 % of the smaller viewport dimension.
        const newScale = Math.min(cRect.width / svgW, cRect.height / svgH) * 0.9;

        // Center the cell in the viewport.
        this._scale = newScale;
        this._panX  = cRect.width  / 2 - (svgX + svgW / 2) * newScale;
        this._panY  = cRect.height / 2 - (svgY + svgH / 2) * newScale;
        this._applyTransform();
    }

    _applyTransform() {
        if (!this._svgEl) return;
        this._svgEl.style.transform =
            `translate(${this._panX}px, ${this._panY}px) scale(${this._scale})`;
        this._svgEl.style.transformOrigin = '0 0';
    }

    fitView() {
        if (!this._svgEl) return;
        const cRect = this.svgContainer.getBoundingClientRect();
        if (cRect.width === 0 || cRect.height === 0) return; // panel not visible yet
        // Prefer viewBox dimensions; fall back to rendered size
        const vb = this._svgEl.viewBox && this._svgEl.viewBox.baseVal;
        const svgW = (vb && vb.width)  || this._svgEl.clientWidth  || 1;
        const svgH = (vb && vb.height) || this._svgEl.clientHeight || 1;
        this._scale = Math.min(cRect.width / svgW, cRect.height / svgH) * 0.95;
        this._panX = (cRect.width  - svgW * this._scale) / 2;
        this._panY = (cRect.height - svgH * this._scale) / 2;
        this._applyTransform();
    }

    // ── NetlistSVG init ──────────────────────────────────────────────────────

    _addOpenRoadSkinSymbols(skin) {
        // Inject OpenROAD-owned templates so rendering is independent of the
        // NetlistSVG skin revision. The ports match the backend's normalized
        // connections.
        const closingSvg = skin.lastIndexOf('</svg>');
        if (closingSvg === -1) {
            throw new Error('NetlistSVG skin is not valid SVG');
        }

        const gateInputYs = {
            2: [7, 23],
            3: [6, 15, 24],
            4: [5, 11.5, 18.5, 25],
        };

        // - s:type: the cell type NetlistSVG matches, like openroad_nand2.
        // - s:alias: extra matching name.
        // - <path> / <circle>: the actual SVG shape.
        // - <g s:x=... s:y=... s:pid=...>: pin locations.
        // - <text s:attribute="ref">: instance name label.
        const nodeLabel = (x, label) =>
            `    <text x="${x}" y="-4" class="nodelabel $cell_id" s:attribute="ref">${label}</text>`;
        const pinLabel = (x, y, label, anchor = 'start', port = label, side = 'right') =>
            `    <text x="${x}" y="${y}" data-openroad-port="${port}" data-openroad-port-side="${side}" style="fill:#000;stroke:none;font-size:10px;font-family:&quot;Courier New&quot;,monospace;font-weight:bold;text-anchor:${anchor};pointer-events:none;">${label}</text>`;
        const portPin = (x, y, port, label, anchor, side, labelX, labelY = -4) => `
    <g transform="translate(${x}, ${y})" s:x="${x}" s:y="${y}" s:pid="${port}">
${pinLabel(labelX, labelY, label, anchor, port, side)}
    </g>`;
        const inputPins = (count, x = 0, labelY = -4) => gateInputYs[count]
            .map((y, i) => {
                const port = `A${i + 1}`;
                return portPin(x, y, port, port, 'end', 'left', -3, labelY);
            })
            .join('\n');
        const outputPin = (x, y, port = 'Y', label = port, labelX = 4) =>
            portPin(x, y, port, label, 'start', 'right', labelX);

        // NetlistSVG selects one of these drawings when a cell's "type"
        // matches s:type or s:alias. The <path> elements draw gate outlines;
        // the <circle> elements draw inversion bubbles.
        const andTemplate = (count) => `
  <g s:type="openroad_and${count}" transform="translate(0,0)" s:width="30" s:height="30">
    <s:alias val="openroad_and${count}"/>
${nodeLabel(15, `and${count}`)}
    <!-- AND body: flat input side with rounded output side. -->
    <path d="M0,0 L15,0 C23.284,0 30,6.716 30,15 C30,23.284 23.284,30 15,30 L0,30 Z" class="$cell_id"/>
${inputPins(count)}
${outputPin(30, 15, 'Y', 'Y', 0)}
  </g>
`;
        const nandTemplate = (count) => `
  <g s:type="openroad_nand${count}" transform="translate(0,0)" s:width="34" s:height="30">
    <s:alias val="openroad_nand${count}"/>
${nodeLabel(17, `nand${count}`)}
    <!-- NAND body: same AND outline plus an output inversion bubble. -->
    <path d="M0,0 L14,0 C21.732,0 28,6.716 28,15 C28,23.284 21.732,30 14,30 L0,30 Z" class="$cell_id"/>
    <circle cx="31" cy="15" r="3" class="$cell_id"/>
${inputPins(count, 0, count === 3 ? -2 : -4)}
${outputPin(34, 15, 'Y', 'Y', 0)}
  </g>
`;
        const orTemplate = (count) => `
  <g s:type="openroad_or${count}" transform="translate(0,0)" s:width="30" s:height="30">
    <s:alias val="openroad_or${count}"/>
${nodeLabel(15, `or${count}`)}
    <!-- OR body: two arcs form the curved input side and curved output side. -->
    <path d="M0,30 L15,30 A15 15 0 0 0 15,0 L0,0" class="$cell_id"/>
    <path d="M0,0 A30 30 0 0 1 0,30" class="$cell_id"/>
${inputPins(count, 3)}
${outputPin(30, 15, 'Y', 'Y', 0)}
  </g>
`;
        const norTemplate = (count) => `
  <g s:type="openroad_nor${count}" transform="translate(0,0)" s:width="37" s:height="30">
    <s:alias val="openroad_nor${count}"/>
${nodeLabel(18.5, `nor${count}`)}
    <!-- NOR body: same OR outline plus an output inversion bubble. -->
    <path d="M0,30 L15,30 A15 15 0 0 0 15,0 L0,0" class="$cell_id"/>
    <path d="M0,0 A30 30 0 0 1 0,30" class="$cell_id"/>
    <circle cx="34" cy="15" r="3" class="$cell_id"/>
${inputPins(count, 3)}
${outputPin(37, 15, 'Y', 'Y', 0)}
  </g>
`;
        const xorTemplate = (count) => `
  <g s:type="openroad_xor${count}" transform="translate(0,0)" s:width="33" s:height="30">
    <s:alias val="openroad_xor${count}"/>
${nodeLabel(16.5, `xor${count}`)}
    <!-- XOR body: OR outline plus the extra curved input stroke. -->
    <path d="M3,30 L18,30 A15 15 0 0 0 18,0 L3,0" class="$cell_id"/>
    <path d="M3,0 A30 30 0 0 1 3,30" class="$cell_id"/>
    <path d="M0,0 A30 30 0 0 1 0,30" class="$cell_id"/>
${inputPins(count)}
${outputPin(33, 15, 'Y', 'Y', 0)}
  </g>
`;
        const xnorTemplate = (count) => `
  <g s:type="openroad_xnor${count}" transform="translate(0,0)" s:width="40" s:height="30">
    <s:alias val="openroad_xnor${count}"/>
${nodeLabel(20, `xnor${count}`)}
    <!-- XNOR body: XOR outline plus an output inversion bubble. -->
    <path d="M3,30 L18,30 A15 15 0 0 0 18,0 L3,0" class="$cell_id"/>
    <path d="M3,0 A30 30 0 0 1 3,30" class="$cell_id"/>
    <path d="M0,0 A30 30 0 0 1 0,30" class="$cell_id"/>
    <circle cx="37" cy="15" r="3" class="$cell_id"/>
${inputPins(count)}
${outputPin(40, 15, 'Y', 'Y', 0)}
  </g>
`;
        const bufferTemplate = `
  <g s:type="openroad-buffer" transform="translate(0,0)" s:width="30" s:height="20">
    <s:alias val="openroad_buffer"/>
${nodeLabel(10, 'buffer')}
    <!-- Buffer body: triangle. -->
    <path d="M0,0 L0,20 L20,10 Z" class="$cell_id"/>
${portPin(0, 10, 'A', 'A', 'end', 'left', -3)}
${outputPin(20, 10, 'Y', 'Y', 0)}
  </g>
`;
        const inverterTemplate = `
  <g s:type="openroad-inverter" transform="translate(0,0)" s:width="25" s:height="20">
    <s:alias val="openroad_inverter"/>
${nodeLabel(12.5, 'inverter')}
    <!-- Inverter body: triangle plus an output inversion bubble. -->
    <path d="M0,0 L0,20 L20,10 Z" class="$cell_id"/>
    <circle cx="23" cy="10" r="3" class="$cell_id"/>
${portPin(0, 10, 'A', 'A', 'end', 'left', -3)}
${outputPin(25, 10, 'Y', 'Y', 0)}
  </g>
`;
        const dffTemplate = `
  <g s:type="openroad_dff" transform="translate(0,0)" s:width="46" s:height="44">
    <s:alias val="openroad_dff"/>
${nodeLabel(23, 'dff')}
    <!-- Register body plus a closed edge-triggered clock triangle on CK. -->
    <rect x="0" y="0" width="46" height="44" class="$cell_id"/>
    <path d="M0,30 L7,34 L0,38 Z" class="$cell_id" style="stroke-linejoin:miter;"/>
${portPin(0, 12, 'D', 'D', 'end', 'left', -3)}
${portPin(0, 34, 'CK', 'CK', 'end', 'left', -3)}
${outputPin(46, 12, 'Q')}
${outputPin(46, 32, 'QN')}
  </g>
`;
        const dffrTemplate = `
  <g s:type="openroad_dffr" transform="translate(0,0)" s:width="50" s:height="54">
    <s:alias val="openroad_dffr"/>
${nodeLabel(25, 'dffr')}
    <!-- Resettable register: DFF body plus active-low RN bubble. -->
    <rect x="0" y="0" width="50" height="54" class="$cell_id"/>
    <path d="M0,24 L7,28 L0,32 Z" class="$cell_id" style="stroke-linejoin:miter;"/>
    <circle cx="3" cy="44" r="3" class="$cell_id"/>
${portPin(0, 12, 'D', 'D', 'end', 'left', -3)}
${portPin(0, 28, 'CK', 'CK', 'end', 'left', -3)}
${portPin(0, 44, 'RN', 'RN', 'end', 'left', -3)}
${outputPin(50, 12, 'Q')}
${outputPin(50, 34, 'QN')}
  </g>
`;
        const dffsTemplate = `
  <g s:type="openroad_dffs" transform="translate(0,0)" s:width="50" s:height="54">
    <s:alias val="openroad_dffs"/>
${nodeLabel(25, 'dffs')}
    <!-- Settable register: DFF body plus active-low SN bubble. -->
    <rect x="0" y="0" width="50" height="54" class="$cell_id"/>
    <path d="M0,24 L7,28 L0,32 Z" class="$cell_id" style="stroke-linejoin:miter;"/>
    <circle cx="3" cy="44" r="3" class="$cell_id"/>
${portPin(0, 12, 'D', 'D', 'end', 'left', -3)}
${portPin(0, 28, 'CK', 'CK', 'end', 'left', -3)}
${portPin(0, 44, 'SN', 'SN', 'end', 'left', -3)}
${outputPin(50, 12, 'Q')}
${outputPin(50, 34, 'QN')}
  </g>
`;
        const gateTemplates = [2, 3, 4]
            .map((count) =>
                andTemplate(count)
                + nandTemplate(count)
                + orTemplate(count)
                + norTemplate(count)
                + xorTemplate(count)
                + xnorTemplate(count))
            .join('');
        return skin.slice(0, closingSvg)
             + bufferTemplate
             + inverterTemplate
             + dffTemplate
             + dffrTemplate
             + dffsTemplate
             + gateTemplates
             + skin.slice(closingSvg);
    }

    async initNetlistSVG() {
        try {
            if (!window.netlistsvg) {
                return;  // Not available (e.g. static report).
            }
            this.netlistsvg = window.netlistsvg;

            // The bundle passes skinData to onml.p() which expects a raw XML
            // string — not a DOMParser Document.  Fetch the skin as plain text.
            let skin = this.netlistsvg.digitalSkin || this.netlistsvg.defaultSkin;
            if (!skin || typeof skin !== 'string') {
                const resp = await fetch('https://unpkg.com/netlistsvg/lib/default.svg');
                if (!resp.ok) {
                    throw new Error(`Skin fetch failed: ${resp.status} ${resp.statusText}`);
                }
                skin = await resp.text();
            }
            this.skin = this._addOpenRoadSkinSymbols(skin);
            this._netlistsvgReady = true;
            console.log('NetlistSVG ready.');
        } catch (err) {
            console.error('NetlistSVG init failed:', err);
            this.setStatus(`Init error: ${err.message}`);
        }
    }

    // ── Refresh ──────────────────────────────────────────────────────────────

    refresh() {
        const instName = this.appState.selectedInstanceName;
        if (!instName) {
            this.setStatus('Select an instance in the layout to view its schematic.');
            return Promise.resolve(false);
        }

        if (!this._netlistsvgReady) {
            this.setStatus('NetlistSVG not ready yet — try again in a moment.');
            return Promise.resolve(false);
        }

        const wm = this.appState.websocketManager;
        if (!wm) {
            this.setStatus('Waiting for server connection…');
            return Promise.resolve(false);
        }

        this.setStatus(`Loading schematic for ${instName}…`);

        const { faninDepth, fanoutDepth } = this._schematicDepths();
        const readyPromise = wm.readyPromise || Promise.resolve();

        return readyPromise.then(() =>
            wm.request({ type: 'schematic_cone', inst_name: instName,
                         fanin_depth: faninDepth, fanout_depth: fanoutDepth })
                .then(data => {
                    const cells = data.modules && data.modules.top && data.modules.top.cells;
                    if (!cells || Object.keys(cells).length === 0) {
                        this.setStatus('No cells found for selected instance.');
                        return false;
                    }
                    return this.renderNetlist(data).then(() => true);
                })
                .catch(err => {
                    this.setStatus(`Error: ${err}`);
                    return false;
                }));
    }

    _schematicDepths() {  //reads the fanin/fanout input boxes.
        const faninRaw = parseInt(
            this.controls.querySelector('#schematic-fanin-depth').value,
            10);
        const fanoutRaw = parseInt(
            this.controls.querySelector('#schematic-fanout-depth').value,
            10);
        return {
            faninDepth: isNaN(faninRaw) ? 1 : faninRaw,
            fanoutDepth: isNaN(fanoutRaw) ? 1 : fanoutRaw,
        };
    }

    _expandFromInstance(instName) {
        if (!instName) {
            return Promise.resolve(false);
        }

        if (!this._netlistsvgReady) {
            this.setStatus('NetlistSVG not ready yet — try again in a moment.');
            return Promise.resolve(false);
        }

        const wm = this.appState.websocketManager;
        if (!wm) {
            this.setStatus('Waiting for server connection…');
            return Promise.resolve(false);
        }

        this.setStatus(`Expanding schematic from ${instName}…`);

        const { faninDepth, fanoutDepth } = this._schematicDepths();
        const readyPromise = wm.readyPromise || Promise.resolve();

        return readyPromise.then(() =>
            wm.request({ type: 'schematic_cone', inst_name: instName,
                         fanin_depth: faninDepth, fanout_depth: fanoutDepth })
                .then(data => {
                    const cells = data.modules && data.modules.top && data.modules.top.cells;
                    if (!cells || Object.keys(cells).length === 0) {
                        this.setStatus('No cells found for selected instance.');
                        return false;
                    }
                    const netlist = this._currentNetlist
                        ? this._mergeSchematicNetlists(this._currentNetlist, data)
                        : data;
                    return this.renderNetlist(netlist).then(() => true);
                })
                .catch(err => {
                    this.setStatus(`Error: ${err}`);
                    return false;
                }));
    }

    _cloneJson(value) {
        return JSON.parse(JSON.stringify(value));
    }

    _topModule(netlist) {
        return netlist && netlist.modules && netlist.modules.top
            ? netlist.modules.top
            : null;
    }

    _ensureSchematicTopFields(top) {
        top.attributes = top.attributes || {};
        top.ports = top.ports || {};
        top.cells = top.cells || {};
        top.netnames = top.netnames || {};
    }

    _collectSchematicBits(value, bits) {
        if (Array.isArray(value)) {
            value.forEach((item) => this._collectSchematicBits(item, bits));
            return;
        }
        if (value && typeof value === 'object') {
            Object.values(value)
                .forEach((item) => this._collectSchematicBits(item, bits));
            return;
        }
        if (Number.isInteger(value)) {
            bits.add(value);
        }
    }

    _remapSchematicBits(value, bitRemap) {
        if (Array.isArray(value)) {
            return value.map((item) => this._remapSchematicBits(item, bitRemap));
        }
        if (value && typeof value === 'object') {
            const remapped = {};
            for (const [key, item] of Object.entries(value)) {
                remapped[key] = this._remapSchematicBits(item, bitRemap);
            }
            return remapped;
        }
        if (Number.isInteger(value) && bitRemap.has(value)) {
            return bitRemap.get(value);
        }
        return value;
    }

    //keeps existing cells, adds new cells, merges ports/netnames, and remaps net IDs.
    _mergeSchematicNetlists(baseNetlist, addedNetlist) {
        const baseTop = this._topModule(baseNetlist);
        const addedTop = this._topModule(addedNetlist);
        if (!baseTop) {
            return this._cloneJson(addedNetlist);
        }
        if (!addedTop) {
            return this._cloneJson(baseNetlist);
        }

        const merged = this._cloneJson(baseNetlist);
        const mergedTop = this._topModule(merged);
        this._ensureSchematicTopFields(mergedTop);

        const added = this._cloneJson(addedNetlist);
        const addedCloneTop = this._topModule(added);
        this._ensureSchematicTopFields(addedCloneTop);

        const usedBits = new Set();
        this._collectSchematicBits(mergedTop, usedBits);
        let nextBit = Math.max(1, ...usedBits);
        const allocateBit = () => {
            nextBit += 1;
            while (usedBits.has(nextBit)) {
                nextBit += 1;
            }
            usedBits.add(nextBit);
            return nextBit;
        };

        const bitRemap = new Map();
        for (const [netName, addedNet] of Object.entries(addedCloneTop.netnames)) {
            const addedBits = Array.isArray(addedNet.bits) ? addedNet.bits : [];
            let mergedBits = null;
            if (Object.prototype.hasOwnProperty.call(mergedTop.netnames, netName)
                && Array.isArray(mergedTop.netnames[netName].bits)) {
                mergedBits = mergedTop.netnames[netName].bits;
            } else {
                const copiedNet = this._cloneJson(addedNet);
                mergedBits = addedBits.map((bit) =>
                    Number.isInteger(bit) && bit > 1 ? allocateBit() : bit);
                copiedNet.bits = mergedBits;
                mergedTop.netnames[netName] = copiedNet;
            }

            addedBits.forEach((bit, index) => {
                if (Number.isInteger(bit)
                    && bit > 1
                    && index < mergedBits.length) {
                    bitRemap.set(bit, mergedBits[index]);
                }
            });
        }

        for (const [portName, port] of Object.entries(addedCloneTop.ports)) {
            if (Object.prototype.hasOwnProperty.call(mergedTop.ports, portName)) {
                continue;
            }
            const copiedPort = this._cloneJson(port);
            if (Array.isArray(copiedPort.bits)) {
                copiedPort.bits = this._remapSchematicBits(copiedPort.bits, bitRemap);
            }
            mergedTop.ports[portName] = copiedPort;
        }

        for (const [cellName, cell] of Object.entries(addedCloneTop.cells)) {
            if (Object.prototype.hasOwnProperty.call(mergedTop.cells, cellName)) {
                continue;
            }
            const copiedCell = this._cloneJson(cell);
            if (copiedCell.connections) {
                copiedCell.connections = this._remapSchematicBits(
                    copiedCell.connections,
                    bitRemap);
            }
            if (copiedCell.attributes
                && copiedCell.attributes.openroad_symbol_connections) {
                copiedCell.attributes.openroad_symbol_connections
                    = this._remapSchematicBits(
                        copiedCell.attributes.openroad_symbol_connections,
                        bitRemap);
            }
            mergedTop.cells[cellName] = copiedCell;
        }

        return merged;
    }

    setStatus(msg) {
        this.controls.querySelector('#schematic-status').textContent = msg;
    }

    // ── Render ───────────────────────────────────────────────────────────────

    _isSymbolView() {
        return this.controls.querySelector('#schematic-view-style').value !== 'boxes';
    }

    _netlistForView(yosysJson) {
        if (this._isSymbolView()) {
            const cells = yosysJson.modules && yosysJson.modules.top
                        && yosysJson.modules.top.cells || {};
            if (!Object.values(cells).some((cell) =>
                    cell.attributes && cell.attributes.openroad_symbol_type)) {
                return yosysJson;
            }

            const netlist = JSON.parse(JSON.stringify(yosysJson));
            const symbolCells = netlist.modules && netlist.modules.top
                              && netlist.modules.top.cells || {};
            for (const cell of Object.values(symbolCells)) {
                const attributes = cell.attributes || {};
                if (!attributes.openroad_symbol_type) {
                    continue;
                }

                attributes.openroad_master = cell.type;
                cell.type = attributes.openroad_symbol_type;
                cell.port_directions = attributes.openroad_symbol_port_directions || {};
                cell.connections = attributes.openroad_symbol_connections || {};
            }
            return netlist;
        }

        // The backend normalizes supported Liberty cells to NetlistSVG symbol
        // types. Restore the physical master and pin names for users who prefer
        // the original generic-box representation.
        const netlist = JSON.parse(JSON.stringify(yosysJson));
        const cells = netlist.modules && netlist.modules.top
                    && netlist.modules.top.cells || {};
        for (const cell of Object.values(cells)) {
            const attributes = cell.attributes || {};
            if (!attributes.openroad_master) {
                continue;
            }

            const inputPorts = Array.isArray(attributes.openroad_input_ports)
                ? attributes.openroad_input_ports
                : [attributes.openroad_input_port || 'A'];
            const normalizedInputPorts = Array.isArray(attributes.openroad_normalized_input_ports)
                ? attributes.openroad_normalized_input_ports
                : inputPorts.map((_, index) => inputPorts.length === 1 ? 'A' : `A${index + 1}`);
            const outputPorts = Array.isArray(attributes.openroad_output_ports)
                ? attributes.openroad_output_ports
                : [attributes.openroad_output_port || 'Y'];
            const normalizedOutputPorts = Array.isArray(attributes.openroad_normalized_output_ports)
                ? attributes.openroad_normalized_output_ports
                : outputPorts.map((_, index) => index === 0 ? 'Y' : `Y${index + 1}`);

            cell.type = attributes.openroad_master;
            const portDirections = {};
            const connections = {};
            inputPorts.forEach((inputPort, index) => {
                const normalizedPort = normalizedInputPorts[index]
                    || (inputPorts.length === 1 ? 'A' : `A${index + 1}`);
                const inputConnection = cell.connections && cell.connections[normalizedPort];
                if (inputConnection) {
                    portDirections[inputPort] = 'input';
                    connections[inputPort] = inputConnection;
                }
            });
            outputPorts.forEach((outputPort, index) => {
                const normalizedPort = normalizedOutputPorts[index]
                    || (index === 0 ? 'Y' : `Y${index + 1}`);
                const outputConnection = cell.connections && cell.connections[normalizedPort];
                if (outputConnection) {
                    portDirections[outputPort] = 'output';
                    connections[outputPort] = outputConnection;
                }
            });
            cell.port_directions = portDirections;
            cell.connections = connections;
        }
        return netlist;
    }

    _cellGroupForInstance(instName) {
        if (!this._svgEl) {
            return null;
        }

        const prefixed = 'cell_' + instName;
        return this._svgEl.querySelector(`#${CSS.escape(prefixed)}`)
            || this._svgEl.querySelector(`#${CSS.escape(instName)}`)
            || this._closestGroup(
                this._svgEl.querySelector(`.${CSS.escape(prefixed)}`));
    }

    _cellClassIdsForGroup(group) {
        const ids = new Set();
        if (!group) {
            return ids;
        }

        const visit = (el) => {
            const classAttr = el.getAttribute && el.getAttribute('class');
            if (!classAttr) {
                return;
            }
            for (const token of classAttr.split(/\s+/)) {
                if (token.startsWith('cell_')) {
                    ids.add(token);
                }
            }
        };

        visit(group);
        for (const el of group.querySelectorAll('[class]')) {
            visit(el);
        }
        return ids;
    }

    _hitElementsForCell(group, cellIds) {
        const elements = new Set();
        const addShapeElements = (root) => {
            if (!root) {
                return;
            }
            const tagName = root.tagName && root.tagName.toLowerCase();
            if (['path', 'rect', 'circle', 'ellipse', 'polygon', 'polyline', 'line'].includes(tagName)) {
                elements.add(root);
            }
            if (root.querySelectorAll) {
                for (const el of root.querySelectorAll(
                    'path,rect,circle,ellipse,polygon,polyline,line')) {
                    elements.add(el);
                }
            }
        };

        addShapeElements(group);
        for (const cellId of cellIds) {
            for (const el of this._svgEl.querySelectorAll(`.${CSS.escape(cellId)}`)) {
                addShapeElements(el);
            }
        }

        if (elements.size === 0 && group) {
            elements.add(group);
        }
        return [...elements];
    }

    _registerSvgCellHitRecord(instName, group) {
        const cellIds = new Set([`cell_${instName}`, instName]);
        if (group && group.id) {
            cellIds.add(group.id);
        }
        for (const classId of this._cellClassIdsForGroup(group)) {
            cellIds.add(classId);
        }

        for (const cellId of cellIds) {
            this._svgIdToInstName.set(cellId, instName);
        }

        let hitGroup = group;
        if (!hitGroup) {
            for (const cellId of cellIds) {
                hitGroup = this._cellGroupForSvgId(cellId);
                if (hitGroup) {
                    break;
                }
            }
        }

        const hitElements = this._hitElementsForCell(hitGroup, cellIds);
        if (hitElements.length === 0) {
            return;
        }

        this._svgCellHitRecords.push({
            instName,
            cellId: [...cellIds][0],
            group: hitGroup,
            hitElements,
        });
    }

    _openRoadPortLabelMap(cell) {
        const attributes = cell.attributes || {};
        const labels = new Map();

        const inputPorts = Array.isArray(attributes.openroad_input_ports)
            ? attributes.openroad_input_ports
            : (attributes.openroad_input_port ? [attributes.openroad_input_port] : []);
        inputPorts.forEach((port, index) => {
            const normalized = inputPorts.length === 1 ? 'A' : `A${index + 1}`;
            labels.set(normalized, port);
        });

        if (attributes.openroad_output_port) {
            labels.set('Y', attributes.openroad_output_port);
            if (attributes.openroad_symbol_connections
                && Object.prototype.hasOwnProperty.call(
                    attributes.openroad_symbol_connections,
                    'Q')) {
                labels.set('Q', attributes.openroad_output_port);
            }
        }

        if (attributes.openroad_data_port) {
            labels.set('D', attributes.openroad_data_port);
        }
        if (attributes.openroad_clock_port) {
            labels.set('CK', attributes.openroad_clock_port);
        }
        if (attributes.openroad_clear_port) {
            labels.set('RN', attributes.openroad_clear_port);
        }
        if (attributes.openroad_preset_port) {
            labels.set('SN', attributes.openroad_preset_port);
        }

        return labels;
    }

    _isInstanceLabel(label) {
        return label.getAttribute('data-openroad-label') === 'instance'
            || this._svgSkinAttribute(label, 'attribute') === 'ref';
    }

    _textAnchorForLabel(label) {
        if (label.style.textAnchor) {
            return label.style.textAnchor;
        }
        const textAnchor = label.getAttribute('text-anchor');
        if (textAnchor) {
            return textAnchor;
        }
        if (label.classList.contains('inputPortLabel')) {
            return 'end';
        }
        if (label.classList.contains('nodelabel')) {
            return 'middle';
        }
        return 'start';
    }

    _stylePortLabel(label, fontSize = 10) {
        const anchor = this._textAnchorForLabel(label);
        label.style.fill = '#000';
        label.style.stroke = 'none';
        label.style.fontSize = `${fontSize}px`;
        label.style.fontFamily = '"Courier New", monospace';
        label.style.fontWeight = 'bold';
        label.style.textAnchor = anchor;
        label.setAttribute('text-anchor', anchor);
        label.style.pointerEvents = 'none';
    }

    _textBBox(label) {
        try {
            const bbox = label.getBBox();
            if (bbox && bbox.width > 0 && bbox.height > 0) {
                return {
                    x: bbox.x,
                    y: bbox.y,
                    right: bbox.x + bbox.width,
                    bottom: bbox.y + bbox.height,
                    width: bbox.width,
                    height: bbox.height,
                };
            }
        } catch (_) {
            // Fall back to an estimate in tests and during early SVG layout.
        }

        const fontSize = parseFloat(label.style.fontSize || label.getAttribute('font-size') || '5') || 5;
        const text = label.textContent || '';
        const width = Math.max(text.length, 1) * fontSize * 0.65;
        const height = fontSize;
        const x = parseFloat(label.getAttribute('x') || '0') || 0;
        const y = parseFloat(label.getAttribute('y') || '0') || 0;
        const anchor = this._textAnchorForLabel(label);
        const left = anchor === 'middle'
            ? x - width / 2
            : (anchor === 'end' ? x - width : x);
        const top = y - height / 2;
        return {
            x: left,
            y: top,
            right: left + width,
            bottom: top + height,
            width,
            height,
        };
    }

    _groupBoundsWithoutText(group) {
        const hiddenText = [];
        for (const label of group.querySelectorAll('text')) {
            hiddenText.push([label, label.getAttribute('display')]);
            label.setAttribute('display', 'none');
        }

        try {
            const bbox = group.getBBox();
            return {
                x: bbox.x,
                y: bbox.y,
                right: bbox.x + bbox.width,
                bottom: bbox.y + bbox.height,
                width: bbox.width,
                height: bbox.height,
            };
        } finally {
            for (const [label, display] of hiddenText) {
                if (display === null) {
                    label.removeAttribute('display');
                } else {
                    label.setAttribute('display', display);
                }
            }
        }
    }

    _parseSvgNumber(value) {
        const parsed = parseFloat(value);
        return Number.isFinite(parsed) ? parsed : null;
    }

    _portMarkerForLabel(group, normalizedPort) {
        for (const marker of group.querySelectorAll('g')) {
            if (this._svgSkinAttribute(marker, 'pid') === normalizedPort) {
                return marker;
            }
        }
        return null;
    }

    _portMarkerPosition(marker) {
        let x = this._parseSvgNumber(this._svgSkinAttribute(marker, 'x'));
        let y = this._parseSvgNumber(this._svgSkinAttribute(marker, 'y'));
        if (x !== null && y !== null) {
            return { x, y };
        }

        const transform = marker.getAttribute('transform') || '';
        const match = transform.match(
            /translate\(\s*([-+]?\d*\.?\d+)(?:[,\s]+([-+]?\d*\.?\d+))?/);
        if (match) {
            x = x !== null ? x : this._parseSvgNumber(match[1]);
            y = y !== null
                ? y
                : this._parseSvgNumber(match[2] !== undefined ? match[2] : '0');
        }

        return x !== null && y !== null ? { x, y } : null;
    }

    _openRoadPortLabelSide(pin, bodyBounds) {
        const centerX = bodyBounds.x + bodyBounds.width / 2;
        const centerY = bodyBounds.y + bodyBounds.height / 2;
        const pinInsideX = pin.x >= bodyBounds.x && pin.x <= bodyBounds.right;
        const pinInsideY = pin.y >= bodyBounds.y && pin.y <= bodyBounds.bottom;
        const edgeTolerance = 1;

        if (pinInsideX && pin.y <= bodyBounds.y + edgeTolerance) {
            return 'top';
        }
        if (pinInsideX && pin.y >= bodyBounds.bottom - edgeTolerance) {
            return 'bottom';
        }
        if (!pinInsideY) {
            return Math.abs(pin.x - centerX) > Math.abs(pin.y - centerY)
                ? (pin.x < centerX ? 'left' : 'right')
                : (pin.y < centerY ? 'top' : 'bottom');
        }
        return pin.x < centerX ? 'left' : 'right';
    }

    _positionOpenRoadPortLabel(group, label, normalizedPort) {
        const marker = this._portMarkerForLabel(group, normalizedPort);
        if (!marker) {
            return false;
        }

        const pin = this._portMarkerPosition(marker);
        if (!pin) {
            return false;
        }

        let side = label.getAttribute('data-openroad-port-side');
        if (!side) {
            try {
                side = this._openRoadPortLabelSide(pin, this._groupBoundsWithoutText(group));
            } catch (_) {
                side = 'right';
            }
        }

        let anchor = 'middle';
        if (side === 'left') {
            anchor = 'end';
        } else if (side === 'right') {
            anchor = 'start';
        }

        const labelIsInsideMarker = marker.contains(label);
        if (labelIsInsideMarker
            && this._parseSvgNumber(label.getAttribute('x')) !== null
            && this._parseSvgNumber(label.getAttribute('y')) !== null) {
            label.setAttribute('data-openroad-port-side', side);
            label.setAttribute('text-anchor', anchor);
            label.style.textAnchor = anchor;
            return true;
        }

        let x = pin.x;
        let y = pin.y - 4;
        if (side === 'left') {
            x = pin.x - 3;
        } else if (side === 'right') {
            x = pin.x + 4;
        } else if (side === 'top') {
            y = pin.y - 5;
        } else if (side === 'bottom') {
            y = pin.y + 10;
        }

        label.setAttribute('x', String(x));
        label.setAttribute('y', String(y));
        label.setAttribute('data-openroad-port-side', side);
        label.setAttribute('text-anchor', anchor);
        label.style.textAnchor = anchor;
        return true;
    }

    _avoidOpenRoadPortLabelShape(group, label) {
        let bodyBounds;
        try {
            bodyBounds = this._groupBoundsWithoutText(group);
        } catch (_) {
            return;
        }

        const bbox = this._textBBox(label);
        if (this._rectOverlapArea(bbox, bodyBounds) === 0) {
            return;
        }

        const gap = 2;
        const side = label.getAttribute('data-openroad-port-side') || 'right';
        let dx = 0;
        let dy = 0;
        if (side === 'left') {
            dx = bodyBounds.x - gap - bbox.right;
        } else if (side === 'right') {
            dx = bodyBounds.right + gap - bbox.x;
        } else if (side === 'top') {
            dy = bodyBounds.y - gap - bbox.bottom;
        } else if (side === 'bottom') {
            dy = bodyBounds.bottom + gap - bbox.y;
        }

        if (dx !== 0 || dy !== 0) {
            const x = parseFloat(label.getAttribute('x') || '0') || 0;
            const y = parseFloat(label.getAttribute('y') || '0') || 0;
            label.setAttribute('x', String(x + dx));
            label.setAttribute('y', String(y + dy));
        }
    }

    _normalizeSchematicPortText() {
        if (!this._svgEl) {
            return;
        }

        for (const label of this._svgEl.querySelectorAll('text')) {
            if (this._isInstanceLabel(label)) {
                continue;
            }

            this._stylePortLabel(label);
        }
    }

    _updateOpenRoadPortLabels(group, cell) {
        const labels = this._openRoadPortLabelMap(cell);
        for (const label of group.querySelectorAll('text[data-openroad-port]')) {
            const normalizedPort = label.getAttribute('data-openroad-port');
            label.textContent = labels.get(normalizedPort) || normalizedPort;
            this._stylePortLabel(label);
            const positioned = this._positionOpenRoadPortLabel(group, label, normalizedPort);
            this._stylePortLabel(label);
            if (!positioned) {
                this._avoidOpenRoadPortLabelShape(group, label);
            }
        }
    }

    _ensureOpenRoadSymbolLabels(netlist) {
        if (!this._svgEl) {
            return;
        }

        const cells = netlist.modules && netlist.modules.top
                    && netlist.modules.top.cells || {};
        const svgNS = 'http://www.w3.org/2000/svg';
        const netlistsvgNS = 'https://github.com/nturley/netlistsvg';

        this._normalizeSchematicPortText();

        for (const [instName, cell] of Object.entries(cells)) {
            const attributes = cell.attributes || {};
            if (!attributes.openroad_master
                && (typeof cell.type !== 'string' || !cell.type.startsWith('openroad_'))) {
                continue;
            }
            const displayName = attributes.ref || instName;

            const group = this._cellGroupForInstance(instName);
            if (!group) {
                continue;
            }

            let label = null;
            for (const text of group.querySelectorAll('text')) {
                const skinAttribute = text.getAttribute('s:attribute')
                    || text.getAttributeNS(netlistsvgNS, 'attribute');
                if (skinAttribute === 'ref'
                    || text.getAttribute('data-openroad-label') === 'instance') {
                    label = text;
                    break;
                }
            }

            if (!label) {
                label = document.createElementNS(svgNS, 'text');
                const symbolWidth = parseFloat(group.getAttribute('s:width')
                    || group.getAttributeNS(netlistsvgNS, 'width'));
                label.setAttribute('x', Number.isFinite(symbolWidth)
                    ? String(symbolWidth / 2)
                    : '15');
                label.setAttribute('y', '-4');
                label.setAttributeNS(netlistsvgNS, 's:attribute', 'ref');
            }

            this._updateOpenRoadPortLabels(group, cell);
            this._setInstanceLabelText(label, displayName);
            label.setAttribute('class', 'nodelabel');
            label.setAttribute('data-openroad-label', 'instance');
            label.setAttribute('pointer-events', 'none');
            label.setAttribute('style',
                'fill:#000;stroke:none;font-size:10px;font-weight:bold;'
                + 'font-family:"Courier New",monospace;text-anchor:middle;');

            // Move the label to the end so strokes from the symbol or wires do
            // not paint over it.
            group.appendChild(label);
        }
    }

    _instanceLabelLines(name) {
        const text = String(name);
        const maxLineChars = 22;
        if (text.length <= maxLineChars) {
            return [text];
        }

        const lines = [];
        let remaining = text;
        while (remaining.length > maxLineChars) {
            let breakAt = -1;
            for (let i = maxLineChars; i >= 12; --i) {
                if ('.$_/]'.includes(remaining[i])) {
                    breakAt = i + 1;
                    break;
                }
            }
            if (breakAt === -1) {
                breakAt = maxLineChars;
            }
            lines.push(remaining.slice(0, breakAt));
            remaining = remaining.slice(breakAt);
        }
        if (remaining.length > 0) {
            lines.push(remaining);
        }
        return lines;
    }

    _setInstanceLabelText(label, name) {
        const svgNS = 'http://www.w3.org/2000/svg';
        const lines = this._instanceLabelLines(name);
        label.textContent = '';
        label.setAttribute('data-openroad-label-line-count', String(lines.length));
        for (const line of lines) {
            const tspan = document.createElementNS(svgNS, 'tspan');
            tspan.textContent = line;
            label.appendChild(tspan);
        }
    }

    _svgSkinAttribute(el, attrName) {
        const netlistsvgNS = 'https://github.com/nturley/netlistsvg';
        return el.getAttribute(`s:${attrName}`)
            || el.getAttributeNS(netlistsvgNS, attrName);
    }

    _instanceLabelForGroup(group) {
        for (const text of group.querySelectorAll('text')) {
            if (this._svgSkinAttribute(text, 'attribute') === 'ref'
                || text.getAttribute('data-openroad-label') === 'instance') {
                return text;
            }
        }
        return null;
    }

    _groupBoundsWithoutLabel(group, label) {
        const oldDisplay = label.getAttribute('display');
        label.setAttribute('display', 'none');
        try {
            return group.getBBox();
        } finally {
            if (oldDisplay === null) {
                label.removeAttribute('display');
            } else {
                label.setAttribute('display', oldDisplay);
            }
        }
    }

    _groupScreenRectWithoutLabel(group, label, padding = 4) {
        const oldDisplay = label.getAttribute('display');
        label.setAttribute('display', 'none');
        try {
            const rect = group.getBoundingClientRect();
            return {
                left: rect.left - padding,
                right: rect.right + padding,
                top: rect.top - padding,
                bottom: rect.bottom + padding,
                width: rect.width + padding * 2,
                height: rect.height + padding * 2,
            };
        } finally {
            if (oldDisplay === null) {
                label.removeAttribute('display');
            } else {
                label.setAttribute('display', oldDisplay);
            }
        }
    }

    _setLabelPosition(label, placement) {
        label.setAttribute('x', String(placement.x));
        label.setAttribute('y', String(placement.y));
        label.style.textAnchor = placement.anchor;
        const tspans = label.querySelectorAll('tspan');
        tspans.forEach((tspan, index) => {
            tspan.setAttribute('x', String(placement.x));
            tspan.setAttribute('dy', index === 0 ? '0' : '1.1em');
        });
    }

    _expandedScreenRect(el, padding = 2) {
        const rect = el.getBoundingClientRect();
        return {
            left: rect.left - padding,
            right: rect.right + padding,
            top: rect.top - padding,
            bottom: rect.bottom + padding,
            width: rect.width + padding * 2,
            height: rect.height + padding * 2,
        };
    }

    _rectOverlapArea(a, b) {
        const aLeft = Number.isFinite(a.left) ? a.left : a.x;
        const aTop = Number.isFinite(a.top) ? a.top : a.y;
        const aRight = Number.isFinite(a.right) ? a.right : aLeft + a.width;
        const aBottom = Number.isFinite(a.bottom) ? a.bottom : aTop + a.height;
        const bLeft = Number.isFinite(b.left) ? b.left : b.x;
        const bTop = Number.isFinite(b.top) ? b.top : b.y;
        const bRight = Number.isFinite(b.right) ? b.right : bLeft + b.width;
        const bBottom = Number.isFinite(b.bottom) ? b.bottom : bTop + b.height;

        if (![aLeft, aTop, aRight, aBottom, bLeft, bTop, bRight, bBottom]
                .every(Number.isFinite)) {
            return 0;
        }

        const width = Math.max(0, Math.min(aRight, bRight) - Math.max(aLeft, bLeft));
        const height = Math.max(0, Math.min(aBottom, bBottom) - Math.max(aTop, bTop));
        return width * height;
    }

    _labelLineCount(label) {
        const lineCount = parseInt(
            label.getAttribute('data-openroad-label-line-count') || '1',
            10);
        return Number.isFinite(lineCount) && lineCount > 0 ? lineCount : 1;
    }

    _labelPlacementCandidates(bodyBounds, label) {
        const centerX = bodyBounds.x + bodyBounds.width / 2;
        const centerY = bodyBounds.y + bodyBounds.height / 2;
        const lineHeight = 11;
        const labelOffset = (this._labelLineCount(label) - 1) * lineHeight;
        const topGap = 14;
        return [
            {
                x: centerX,
                y: bodyBounds.y - topGap - labelOffset,
                anchor: 'middle',
                preference: 0,
            },
            {
                x: centerX,
                y: bodyBounds.y + bodyBounds.height + 14,
                anchor: 'middle',
                preference: 12,
            },
            {
                x: bodyBounds.x + bodyBounds.width + 10,
                y: centerY + 4 - labelOffset / 2,
                anchor: 'start',
                preference: 24,
            },
            {
                x: bodyBounds.x - 10,
                y: centerY + 4 - labelOffset / 2,
                anchor: 'end',
                preference: 24,
            },
            {
                x: centerX,
                y: centerY + 4 - labelOffset / 2,
                anchor: 'middle',
                preference: 80,
            },
        ];
    }

    _wireObstacleRects(cellGroups) {
        if (!this._svgEl) {
            return [];
        }

        const insideCell = (element) =>
            cellGroups.some((group) => group.contains(element));

        return Array.from(this._svgEl.querySelectorAll('path,line,polyline'))
            .filter((element) => !insideCell(element))
            .map((element) => element.getBoundingClientRect())
            .filter((rect) => rect.width > 0 || rect.height > 0)
            .map((rect) => ({
                left: rect.left - 5,
                right: rect.right + 5,
                top: rect.top - 5,
                bottom: rect.bottom + 5,
                width: rect.width + 10,
                height: rect.height + 10,
            }));
    }

    _layoutInstanceLabels(netlist) {
        if (!this._svgEl) {
            return;
        }

        const cells = netlist.modules && netlist.modules.top
                    && netlist.modules.top.cells || {};
        const records = [];
        for (const instName of Object.keys(cells)) {
            const group = this._cellGroupForInstance(instName);
            if (!group) {
                continue;
            }
            const label = this._instanceLabelForGroup(group);
            if (label) {
                records.push({ group, label });
            }
        }

        if (records.length === 0) {
            return;
        }

        const cellGroups = records.map(({ group }) => group);
        const wireRects = this._wireObstacleRects(cellGroups);
        const movableLabels = new Set(records.map(({ label }) => label));
        const occupiedRects = Array.from(this._svgEl.querySelectorAll('text'))
            .filter((text) => !movableLabels.has(text))
            .map((text) => this._expandedScreenRect(text, 2))
            .filter((rect) => rect.width > 0 && rect.height > 0);
        for (const record of records) {
            try {
                const rect = this._groupScreenRectWithoutLabel(
                    record.group,
                    record.label,
                    5);
                if (rect.width > 0 && rect.height > 0) {
                    occupiedRects.push(rect);
                }
            } catch (_) {
                // Ignore groups that cannot be measured yet.
            }
        }

        records.sort((a, b) => {
            const rectA = a.group.getBoundingClientRect();
            const rectB = b.group.getBoundingClientRect();
            return rectA.top - rectB.top || rectA.left - rectB.left;
        });

        for (const record of records) {
            let bodyBounds;
            try {
                bodyBounds = this._groupBoundsWithoutLabel(record.group, record.label);
            } catch (_) {
                continue;
            }

            let best = null;
            for (const candidate of this._labelPlacementCandidates(bodyBounds, record.label)) {
                this._setLabelPosition(record.label, candidate);
                const rect = this._expandedScreenRect(record.label, 3);
                const wireOverlap = wireRects.reduce(
                    (sum, wire) => sum + this._rectOverlapArea(rect, wire),
                    0);
                const overlap = occupiedRects.reduce(
                    (sum, occupied) => sum + this._rectOverlapArea(rect, occupied),
                    0);
                const score = wireOverlap * 1000
                    + overlap * 100
                    + candidate.preference;
                if (!best || score < best.score) {
                    best = { candidate, score };
                }
                if (wireOverlap === 0
                    && overlap === 0
                    && score === candidate.preference) {
                    break;
                }
            }

            if (best) {
                this._setLabelPosition(record.label, best.candidate);
                occupiedRects.push(this._expandedScreenRect(record.label, 4));
            }
        }
    }

    _unionSvgRect(bounds, rect) {
        if (!rect
            || !Number.isFinite(rect.x)
            || !Number.isFinite(rect.y)
            || !Number.isFinite(rect.right)
            || !Number.isFinite(rect.bottom)
            || rect.right <= rect.x
            || rect.bottom <= rect.y) {
            return bounds;
        }

        if (!bounds) {
            return { ...rect };
        }

        bounds.x = Math.min(bounds.x, rect.x);
        bounds.y = Math.min(bounds.y, rect.y);
        bounds.right = Math.max(bounds.right, rect.right);
        bounds.bottom = Math.max(bounds.bottom, rect.bottom);
        return bounds;
    }

    _screenRectToSvgRect(screenRect) {
        const ctm = this._svgEl && this._svgEl.getScreenCTM();
        if (!ctm
            || screenRect.width <= 0
            || screenRect.height <= 0) {
            return null;
        }

        try {
            const inverse = ctm.inverse();
            const point = this._svgEl.createSVGPoint();
            const corners = [
                [screenRect.left, screenRect.top],
                [screenRect.right, screenRect.top],
                [screenRect.right, screenRect.bottom],
                [screenRect.left, screenRect.bottom],
            ].map(([x, y]) => {
                point.x = x;
                point.y = y;
                return point.matrixTransform(inverse);
            });
            const xs = corners.map((corner) => corner.x);
            const ys = corners.map((corner) => corner.y);

            return {
                x: Math.min(...xs),
                y: Math.min(...ys),
                right: Math.max(...xs),
                bottom: Math.max(...ys),
            };
        } catch (_) {
            return null;
        }
    }

    _localRectToSvgRect(element, rect) {
        const svgCtm = this._svgEl && this._svgEl.getCTM();
        const elementCtm = element.getCTM && element.getCTM();
        if (!svgCtm
            || !elementCtm
            || !rect
            || !Number.isFinite(rect.x)
            || !Number.isFinite(rect.y)
            || !Number.isFinite(rect.width)
            || !Number.isFinite(rect.height)
            || rect.width <= 0
            || rect.height <= 0) {
            return null;
        }

        try {
            const elementToSvg = svgCtm.inverse().multiply(elementCtm);
            const point = this._svgEl.createSVGPoint();
            const corners = [
                [rect.x, rect.y],
                [rect.x + rect.width, rect.y],
                [rect.x + rect.width, rect.y + rect.height],
                [rect.x, rect.y + rect.height],
            ].map(([x, y]) => {
                point.x = x;
                point.y = y;
                return point.matrixTransform(elementToSvg);
            });
            const xs = corners.map((corner) => corner.x);
            const ys = corners.map((corner) => corner.y);

            return {
                x: Math.min(...xs),
                y: Math.min(...ys),
                right: Math.max(...xs),
                bottom: Math.max(...ys),
            };
        } catch (_) {
            return null;
        }
    }

    _elementBBoxToSvgRect(element) {
        try {
            const bbox = element.getBBox();
            return this._localRectToSvgRect(element, bbox);
        } catch (_) {
            return null;
        }
    }

    _openRoadLabelBounds() {
        if (!this._svgEl) {
            return null;
        }

        let bounds = null;
        for (const label of this._svgEl.querySelectorAll('text[data-openroad-label="instance"]')) {
            const tspans = Array.from(label.querySelectorAll('tspan'));
            const lines = tspans.length > 0
                ? tspans.map((tspan) => tspan.textContent || '')
                : [label.textContent || ''];
            const fontSize = parseFloat(label.style.fontSize || label.getAttribute('font-size') || '10') || 10;
            const lineHeight = fontSize * 1.25;
            const charWidth = fontSize * 0.75;
            const maxChars = lines.reduce((max, line) => Math.max(max, line.length), 1);
            const textWidth = maxChars * charWidth;
            const lineCount = Math.max(lines.length, 1);
            const x = parseFloat(label.getAttribute('x') || '0');
            const y = parseFloat(label.getAttribute('y') || '0');
            const anchor = label.style.textAnchor
                || label.getAttribute('text-anchor')
                || 'start';
            const left = anchor === 'middle'
                ? x - textWidth / 2
                : (anchor === 'end' ? x - textWidth : x);
            const padding = fontSize * 1.25;
            const rect = {
                x: left - padding,
                y: y - fontSize * 1.35 - padding,
                width: textWidth + padding * 2,
                height: fontSize * 1.7 + (lineCount - 1) * lineHeight + padding * 2,
            };
            bounds = this._unionSvgRect(bounds, this._localRectToSvgRect(label, rect));
        }
        return bounds;
    }

    _svgContentBoundsFromSvgBBox() {
        if (!this._svgEl) {
            return null;
        }

        const selectors = [
            'g[id^="cell_"]',
            'text',
            'path',
            'rect',
            'circle',
            'ellipse',
            'line',
            'polyline',
            'polygon',
        ];
        let bounds = null;
        for (const element of this._svgEl.querySelectorAll(selectors.join(','))) {
            bounds = this._unionSvgRect(bounds, this._elementBBoxToSvgRect(element));
        }
        return bounds;
    }

    _svgContentBoundsFromScreen() {
        if (!this._svgEl) {
            return null;
        }

        const selectors = [
            'g[id^="cell_"]',
            'text',
            'path',
            'rect',
            'circle',
            'ellipse',
            'line',
            'polyline',
            'polygon',
        ];
        let bounds = null;
        for (const element of this._svgEl.querySelectorAll(selectors.join(','))) {
            const screenRect = element.getBoundingClientRect();
            const svgRect = this._screenRectToSvgRect(screenRect);
            bounds = this._unionSvgRect(bounds, svgRect);
        }
        return bounds;
    }

    _padSvgToContent() {
        if (!this._svgEl) {
            return;
        }

        try {
            let bounds = null;
            const bbox = this._svgEl.getBBox();
            bounds = this._unionSvgRect(bounds, {
                x: bbox.x,
                y: bbox.y,
                right: bbox.x + bbox.width,
                bottom: bbox.y + bbox.height,
            });
            // This is the clipping guard: include rendered text bounds, then
            // grow the SVG viewBox when names extend past the original SVG.
            bounds = this._unionSvgRect(bounds, this._svgContentBoundsFromSvgBBox());
            bounds = this._unionSvgRect(bounds, this._openRoadLabelBounds());
            bounds = this._unionSvgRect(bounds, this._svgContentBoundsFromScreen());
            if (!bounds) {
                return;
            }

            const padding = 32;
            const x = Math.floor(bounds.x - padding);
            const y = Math.floor(bounds.y - padding);
            const width = Math.ceil(bounds.right - bounds.x + padding * 2);
            const height = Math.ceil(bounds.bottom - bounds.y + padding * 2);

            this._svgEl.setAttribute('viewBox', `${x} ${y} ${width} ${height}`);
            this._svgEl.setAttribute('width', String(width));
            this._svgEl.setAttribute('height', String(height));
            this._svgEl.setAttribute('overflow', 'visible');
            this._svgEl.style.overflow = 'visible';
        } catch (_) {
            // Measurement can fail if the SVG has not been attached or rendered yet.
        }
    }

    async renderNetlist(yosysJson) {
        try {
            this.setStatus('Rendering…');
            this._currentNetlist = yosysJson;
            const renderJson = this._netlistForView(yosysJson);

            // netlistsvg.render() is Promise-based in v1.x (async ELK layout),
            // but older versions used a callback: render(skin, json, done).
            // Support both so the widget works with either bundle.
            let svgString;
            const result = this.netlistsvg.render(this.skin, renderJson);
            if (result && typeof result.then === 'function') {
                // Promise-based (v1.x)
                svgString = await result;
            } else {
                // Callback-based (older); wrap in a Promise
                svgString = await new Promise((resolve, reject) => {
                    this.netlistsvg.render(this.skin, renderJson, (err, svg) => {
                        if (err) reject(err); else resolve(svg);
                    });
                });
            }

            if (typeof svgString !== 'string' || !svgString.includes('<svg')) {
                throw new Error('render() did not return a valid SVG string');
            }

            // Inject SVG into the viewport
            this._selectedCell = null;
            this.controls.querySelector('#schematic-zoom-to').disabled = true;
            this._svgIdToInstName.clear();
            this._svgCellHitRecords = [];
            this.svgContainer.innerHTML = svgString;
            this._svgEl = this.svgContainer.querySelector('svg');
            const symbolView = this._isSymbolView();
            if (symbolView) {
                this._ensureOpenRoadSymbolLabels(renderJson);
                this._layoutInstanceLabels(renderJson);
                this._padSvgToContent();
            }

            // Build SVG-id → ODB instance-name map.
            // netlistsvg renders each cell with id="cell_<instName>", so we
            // check each known instance name against that pattern.
            const cells = yosysJson.modules && yosysJson.modules.top
                        && yosysJson.modules.top.cells || {};
            for (const instName of Object.keys(cells)) {
                // Try "cell_<instName>" first (netlistsvg default prefix), then
                // the bare name as a fallback for any other renderer.
                const group = this._cellGroupForInstance(instName);
                this._registerSvgCellHitRecord(instName, group);
            }
            if (this._svgEl) {
                // Remove any CSS size constraints — transform-based zoom handles sizing
                this._svgEl.style.maxWidth = '';
                this._svgEl.style.height = '';
                this._svgEl.style.display = 'block';
            }
            // Reset to identity, then fit once the browser has fully painted
            // the SVG. Two rAFs are used: the first lets the DOM update, the
            // second lets the layout engine commit real text dimensions.
            this._scale = 1;
            this._panX = 0;
            this._panY = 0;
            requestAnimationFrame(() => requestAnimationFrame(() => {
                if (symbolView) {
                    this._layoutInstanceLabels(renderJson);
                    this._padSvgToContent();
                }
                this.fitView();
            }));

            const cellCount = Object.keys(yosysJson.modules.top.cells).length;
            this.setStatus(`${cellCount} cell${cellCount !== 1 ? 's' : ''}`);
        } catch (err) {
            console.error('NetlistSVG render failed:', err);
            this.setStatus(`Render error: ${err.message || err}`);
        }
    }
}
