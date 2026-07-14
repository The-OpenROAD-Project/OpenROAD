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

        e.preventDefault();
        e.stopPropagation();
        this.appState.selectedInstanceName = name;
        this._highlightCellGroup(hit.group);
        this._fetchInspect(name);
        return this._expandFromInstance(name);
    }

    _fetchInspect(instName) {
        const wm = this.appState.websocketManager;
        if (!wm) return Promise.resolve(false);
        console.log('[Schematic] inspect:', instName);
        return wm.request({
            type: 'schematic_inspect',
            inst_name: instName,
            use_dbu: this.appState.showDbu,
        })
            .then(data => {
                if (this.appState.updateInspector) {
                    this.appState.updateInspector(data);
                }
                if (this.appState.focusComponent) {
                    this.appState.focusComponent('Inspector');
                }
                // Refresh overlay tiles to show instance highlight
                if (this.appState.refreshOverlay) {
                    this.appState.refreshOverlay();
                } else if (this.appState.redrawAllLayers) {
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

    async initNetlistSVG() {
        try {
            if (!window.netlistsvg) {
                return;  // Not available (e.g. static report).
            }
            this.netlistsvg = window.netlistsvg;

            // Load OpenROAD's custom skin (served as a local asset).  It defines
            // proper gate symbols with correctly-placed ports and instance-name
            // labels; renderNetlist() rewrites cell types to match it (see
            // canonicalizeForSkin).  render() passes the skin to onml.p(), which
            // expects a raw XML string, so fetch it as text.
            const resp = await fetch('openroad_skin.svg');
            if (!resp.ok) {
                throw new Error(`Skin fetch failed: ${resp.status} ${resp.statusText}`);
            }
            this.skin = await resp.text();
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

    _schematicDepths() {
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

    _isSymbolView() {
        return this.controls.querySelector('#schematic-view-style').value !== 'boxes';
    }

    _netlistForView(yosysJson) {
        return this._isSymbolView() ? canonicalizeForSkin(yosysJson) : yosysJson;
    }

    setStatus(msg) {
        this.controls.querySelector('#schematic-status').textContent = msg;
    }

    // ── Render ───────────────────────────────────────────────────────────────

    _cellGroupForInstance(instName) {
        if (!this._svgEl || !instName) {
            return null;
        }

        const prefixed = 'cell_' + instName;
        const byId =
            this._svgEl.querySelector(`#${CSS.escape(prefixed)}`)
            || this._svgEl.querySelector(`#${CSS.escape(instName)}`);
        if (byId) {
            return this._closestGroup(byId) || byId;
        }

        const byClass =
            this._svgEl.querySelector(`.${CSS.escape(prefixed)}`)
            || this._svgEl.querySelector(`.${CSS.escape(instName)}`);
        return byClass ? this._closestGroup(byClass) : null;
    }

    _cellClassIdsForGroup(group, instName) {
        const cellIds = new Set();
        if (!group) {
            return cellIds;
        }
        const prefixed = 'cell_' + instName;
        if (group.id && (group.id === prefixed || group.id === instName)) {
            cellIds.add(group.id);
        }
        const classAttr = group.getAttribute && group.getAttribute('class');
        if (classAttr) {
            classAttr.split(/\s+/)
                .filter(Boolean)
                .filter((token) => token === prefixed || token === instName)
                .forEach((token) => cellIds.add(token));
        }
        return cellIds;
    }

    _hitElementsForCell(group, cellIds) {
        const hitElements = new Set();
        const shapeSelector = 'path,rect,circle,ellipse,polygon,polyline,line';
        if (group) {
            group.querySelectorAll(shapeSelector)
                .forEach((el) => hitElements.add(el));
        }
        for (const cellId of cellIds) {
            this._svgEl.querySelectorAll(`.${CSS.escape(cellId)}`)
                .forEach((el) => {
                    if (el.matches(shapeSelector)) {
                        hitElements.add(el);
                    }
                    el.querySelectorAll(shapeSelector)
                        .forEach((shape) => hitElements.add(shape));
                });
        }
        if (hitElements.size === 0 && group) {
            hitElements.add(group);
        }
        return Array.from(hitElements);
    }

    _registerSvgCellHitRecord(instName, group) {
        if (!this._svgEl || !instName || !group) {
            return;
        }

        const prefixed = 'cell_' + instName;
        const cellIds = this._cellClassIdsForGroup(group, instName);
        cellIds.add(prefixed);
        cellIds.add(instName);

        for (const cellId of cellIds) {
            this._svgIdToInstName.set(cellId, instName);
        }

        const hitElements = this._hitElementsForCell(group, cellIds);
        this._svgCellHitRecords.push({
            instName,
            cellId: Array.from(cellIds)[0] || prefixed,
            group,
            hitElements,
        });
    }

    async renderNetlist(yosysJson) {
        try {
            this.setStatus('Rendering…');
            this._currentNetlist = yosysJson;

            // Debug aid: the last netlist rendered is exposed so it can be
            // captured for the offline render preview tool
            // (src/web/test/visual). In DevTools:
            //   copy(JSON.stringify(window.__lastSchematic))
            if (typeof window !== 'undefined') window.__lastSchematic = yosysJson;

            // Rewrite recognised logic gates to the canonical types our custom
            // skin draws (proper gate symbols with correctly-placed ports), so
            // netlistsvg renders and routes them natively.
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
            this._scopeSkinStyles();

            // Build SVG-id → ODB instance-name map.
            // netlistsvg renders each cell with id="cell_<instName>", so we
            // check each known instance name against that pattern.
            const cells = yosysJson.modules && yosysJson.modules.top
                        && yosysJson.modules.top.cells || {};
            for (const instName of Object.keys(cells)) {
                this._registerSvgCellHitRecord(
                    instName,
                    this._cellGroupForInstance(instName));
            }
            if (this._svgEl) {
                // Remove any CSS size constraints — transform-based zoom handles sizing
                this._svgEl.style.maxWidth = '';
                this._svgEl.style.height = '';
                this._svgEl.style.display = 'block';
            }

            // Label the matched gate symbols with the design's real pin names.
            this._applyPinLabels(renderJson);

            // Reset to identity, then fit once the browser has fully painted
            // the SVG. Two rAFs are used: the first lets the DOM update, the
            // second lets the layout engine commit real dimensions.
            this._scale = 1;
            this._panX = 0;
            this._panY = 0;
            requestAnimationFrame(() => requestAnimationFrame(() => this.fitView()));

            const cellCount = Object.keys(yosysJson.modules.top.cells).length;
            this.setStatus(`${cellCount} cell${cellCount !== 1 ? 's' : ''}`);
        } catch (err) {
            console.error('NetlistSVG render failed:', err);
            this.setStatus(`Render error: ${err.message || err}`);
        }
    }

    // Label the matched gate symbols with the design's real pin names.
    // netlistsvg labels generic-box ports itself, but matched symbols use the
    // skin's (label-less) ports, so we add the names here.  `renderJson` is the
    // canonicalized netlist; cells rewritten to a gate symbol carry a
    // `port_labels` map (symbol port id -> real pin name).  The rendered ports
    // keep their `s:pid`/`s:x`/`s:y` attributes, so we place each label by
    // attribute (no getBBox; works before layout).
    _applyPinLabels(renderJson) {
        if (!this._svgEl) return;
        const NS = 'http://www.w3.org/2000/svg';
        const NLNS = 'https://github.com/nturley/netlistsvg';
        const cells = (renderJson.modules && renderJson.modules.top
                       && renderJson.modules.top.cells) || {};
        const attr = (el, name) =>
            el.getAttribute('s:' + name) ?? el.getAttributeNS(NLNS, name);

        for (const [name, cell] of Object.entries(cells)) {
            const labels = cell.port_labels;
            if (!labels) continue;
            const group =
                this._svgEl.querySelector('#' + CSS.escape('cell_' + name)) ||
                this._svgEl.querySelector('#' + CSS.escape(name));
            if (!group) continue;
            const dirs = cell.port_directions || {};

            // Snapshot children: appending labels below mutates the live
            // HTMLCollection, which would otherwise re-enter the loop.
            for (const el of Array.from(group.children)) {
                if (el.tagName !== 'g') continue;
                const pid = attr(el, 'pid');
                if (!pid || !(pid in labels)) continue;
                const sx = parseFloat(attr(el, 'x'));
                const sy = parseFloat(attr(el, 'y'));
                if (!isFinite(sx) || !isFinite(sy)) continue;

                const text = document.createElementNS(NS, 'text');
                const isInput = dirs[pid] === 'input';
                if (isInput) {
                    text.setAttribute('class', 'inputPortLabel');
                    text.setAttribute('x', sx - 3);
                } else {
                    text.setAttribute('x', sx + 4);
                }
                text.setAttribute('y', sy - 4);
                // Half the default 10px text size so the pin names stay
                // proportionate to the small gate symbols.  Use inline style:
                // the skin's `text { font-size:10px }` rule beats a presentation
                // attribute, but not an inline style.
                text.setAttribute('style', 'font-size:5px');
                text.setAttribute('pointer-events', 'none');
                text.textContent = labels[pid];
                group.appendChild(text);
            }
        }
    }

    // netlistsvg's skin embeds a <style> with unscoped element selectors
    // (e.g. `svg { fill:none; stroke:#000 }`, `text { fill:#000 }`). A <style>
    // inside an inline SVG is NOT scoped to that SVG -- once injected into the
    // page its rules apply document-wide, clobbering the fill/stroke of every
    // other inline-SVG icon in the app (the inspector/ruler buttons rendered as
    // black, unfilled outlines). Prefix each rule with the widget's container
    // class so the skin only styles this schematic.
    _scopeSkinStyles() {
        if (!this._svgEl) return;
        const scope = '.schematic-widget';
        // Recurse so selectors nested inside @media/@supports blocks (which
        // expose .cssRules but no .selectorText) are scoped too.
        const scopeRules = (rules) => {
            for (const rule of rules) {
                if (rule.selectorText) {
                    rule.selectorText = scopeCssSelector(rule.selectorText, scope);
                } else if (rule.cssRules) {
                    scopeRules(rule.cssRules);
                }
            }
        };
        for (const styleEl of this._svgEl.querySelectorAll('style')) {
            const sheet = styleEl.sheet;
            if (!sheet) continue;
            try {
                scopeRules(sheet.cssRules);
            } catch (e) {
                continue;  // Should not happen for same-origin inline styles.
            }
        }
    }
}

// Prefix every comma-separated selector in `selectorText` with `scope` so the
// rule only matches descendants of the scope container. Idempotent: a selector
// already carrying the scope is left untouched, so a re-render never nests the
// prefix. "Already scoped" means it begins with `scope` and the next character
// is not part of an identifier -- so `.scope`, `.scope svg`, `.scope>svg` and
// `.scope.active` are kept, while a different class like `.scope-foo` is still
// scoped.
export function scopeCssSelector(selectorText, scope) {
    return selectorText
        .split(',')
        .map((sel) => {
            const trimmed = sel.trim();
            if (!trimmed) return '';
            const after = trimmed.charAt(scope.length);
            const alreadyScoped = trimmed.startsWith(scope)
                && (after === '' || !/[\w-]/.test(after));
            return alreadyScoped ? trimmed : `${scope} ${trimmed}`;
        })
        .filter(Boolean)
        .join(', ');
}

// ── Skin canonicalization ──────────────────────────────────────────────────
//
// The server tags recognised combinational cells with `gate_kind`
// (and/nand/or/nor/xor/xnor/not/buf, or aoi/oai with `gate_terms`).  Before
// rendering we rewrite those cells to the canonical gate types drawn by the
// custom skin (openroad_skin.svg) and remap their pins to the symbol's port ids
// (A, B, …, Y).  netlistsvg then renders proper gate symbols and routes the
// wires to the symbol-defined port positions — no overlay or alignment needed.

// gate_kind -> skin symbol type (a Yosys primitive alias the skin recognises).
const SKIN_SIMPLE_TYPE = {
    and: '$_AND_', nand: '$_NAND_', or: '$_OR_', nor: '$_NOR_',
    xor: '$_XOR_', xnor: '$_XNOR_', not: '$_NOT_', buf: '$_BUF_',
};

// Compound gate types the skin actually draws.  Unsupported arities are left as
// labelled generic boxes (which keep the design's real pin names).
const SKIN_COMPOUND_TYPES = new Set([
    'aoi21', 'aoi22', 'aoi211', 'aoi221', 'aoi222', 'aoi33',
    'oai21', 'oai22', 'oai211', 'oai221', 'oai222', 'oai33',
]);

// Wider (>2-input) basic gates the skin draws, named kind+arity.  2-input gates
// use the Yosys primitive aliases in SKIN_SIMPLE_TYPE instead.
const SKIN_MULTI_TYPES = new Set([
    'and3', 'and4', 'or3', 'or4', 'nand3', 'nand4', 'nor3', 'nor4',
]);

const PID_LETTERS = ['A', 'B', 'C', 'D', 'E', 'F'];

// Rewrite one cell to a custom-skin gate symbol when it carries a recognised
// `gate_kind`: set its `type` to the canonical symbol name and remap its
// `connections`/`port_directions` keys to the symbol's port ids.  Pins not part
// of the gate (power/ground) are dropped.  Cells that aren't recognised — or
// whose computed type has no symbol — are returned unchanged so they render as
// a labelled generic box with their real pin names.
export function canonicalizeCell(cell) {
    const kind = cell && cell.gate_kind;
    if (!kind) return cell;

    const dirs = cell.port_directions || {};
    let outPin = null;
    const inPins = [];
    for (const [pin, dir] of Object.entries(dirs)) {
        if (dir === 'output' && outPin === null) outPin = pin;
        else if (dir === 'input') inPins.push(pin);
    }

    let type;
    const pidOf = {};  // real pin name -> symbol port id

    if (kind === 'aoi' || kind === 'oai') {
        const terms = Array.isArray(cell.gate_terms) ? cell.gate_terms : [];
        if (!terms.length) return cell;
        const sizes = terms.map((t) => t.length);
        type = kind + sizes.slice().sort((a, b) => b - a).join('');
        if (!SKIN_COMPOUND_TYPES.has(type)) return cell;
        // Assign input pids term-by-term, smallest term first (so a 1-pin
        // literal term gets 'A'); matches the symbol's port layout.
        const ordered = terms.map((t) => t.slice())
            .sort((a, b) => a.length - b.length);
        let i = 0;
        for (const term of ordered) {
            for (const pin of term) {
                pidOf[pin] = PID_LETTERS[i] || ('I' + i);
                i++;
            }
        }
    } else {
        const n = inPins.length;
        if (kind === 'not' || kind === 'buf') {
            type = SKIN_SIMPLE_TYPE[kind];          // 1-input
        } else if (n <= 2) {
            type = SKIN_SIMPLE_TYPE[kind];          // 2-input Yosys primitive
        } else {
            type = kind + n;                        // wider gate, e.g. nand3
            if (!SKIN_MULTI_TYPES.has(type)) return cell;
        }
        if (!type) return cell;
        inPins.forEach((pin, idx) => { pidOf[pin] = PID_LETTERS[idx] || ('I' + idx); });
    }
    if (outPin !== null) pidOf[outPin] = 'Y';

    const conns = cell.connections || {};
    const newConns = {};
    const newDirs = {};
    for (const [pin, bits] of Object.entries(conns)) {
        if (pidOf[pin]) newConns[pidOf[pin]] = bits;
    }
    for (const [pin, dir] of Object.entries(dirs)) {
        if (pidOf[pin]) newDirs[pidOf[pin]] = dir;
    }
    // Keep the real pin name for each symbol port id so the viewer can label
    // the symbol with the design's pin names (netlistsvg ignores this field).
    const portLabels = {};
    for (const [pin, pid] of Object.entries(pidOf)) portLabels[pid] = pin;
    return {
        ...cell, type,
        connections: newConns, port_directions: newDirs,
        port_labels: portLabels,
    };
}

// Return a copy of the netlist with recognised logic gates rewritten to the
// custom skin's gate symbols.  Cell keys (instance names) are preserved so
// selection/inspect keep working.
export function canonicalizeForSkin(json) {
    const top = json && json.modules && json.modules.top;
    if (!top || !top.cells) return json;
    const cells = {};
    for (const [name, cell] of Object.entries(top.cells)) {
        cells[name] = canonicalizeCell(cell);
    }
    return { ...json, modules: { ...json.modules, top: { ...top, cells } } };
}
