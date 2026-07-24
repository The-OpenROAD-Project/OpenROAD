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
        // Store the unmodified server netlist so view toggles and double-click
        // expansion can rerender or merge from the same schematic state.
        this._currentNetlist = null;

        // Pan/zoom state
        this._scale = 1;
        this._panX = 0;
        this._panY = 0;

        // Select mode state
        this._selectMode = false;
        this._selectedCell = null;

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

        c.addEventListener('mousedown', (e) => {
            if (!this._svgEl || e.button !== 0) return;
            if (e.detail >= 2) {
                this._handleCellDoubleClick(e);
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

    // Click events often target a skin child path/text or the transparent hit
    // rect, so resolve either an SVG id or class token back to a cell id.
    _cellIdFromElement(el) {
        if (!el) {
            return null;
        }
        if (el.id && this._svgIdToInstName.has(el.id)) {
            return el.id;
        }

        const classAttr = el.getAttribute && el.getAttribute('class');
        if (!classAttr) {
            return null;
        }
        for (const token of classAttr.split(/\s+/)) {
            if (token && this._svgIdToInstName.has(token)) {
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

    _cellHitFromTarget(target) {
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

        // Resolve a clicked cell group or skin child shape to an ODB instance.
        const hit = this._cellHitFromTarget(e.target);
        if (!hit) return;

        const name = this._svgIdToInstName.get(hit.cellId);
        if (!name) return;

        this._highlightCellGroup(hit.group);
        this.setStatus(`Selected: ${name}`);
        this._fetchInspect(name);
    }

    _handleCellDoubleClick(e) {
        if (!this._svgEl) {
            return false;
        }

        const hit = this._cellHitFromTarget(e.target);
        if (!hit) {
            return false;
        }

        const name = this._svgIdToInstName.get(hit.cellId);
        if (!name) {
            return false;
        }

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

            // Load OpenROAD's skin (served as a local asset).  It defines
            // proper gate symbols with correctly-placed ports and instance-name
            // labels; renderNetlist() rewrites cell types to match it (see
            // canonicalizeForSkin).  render() passes the skin to onml.p(), which
            // expects a raw XML string, so fetch it as text.
            const resp = await fetch('openroad_skin.svg', { cache: 'no-store' });
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

    // Shared by refresh and double-click expansion.
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

    // Double-click expansion returns a separate cone. Merge it into the displayed
    // netlist while remapping numeric bits so new local nets cannot collide with
    // bits already used by the current schematic.
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
        let nextBit = 1;
        for (const bit of usedBits) {
            if (bit > nextBit) {
                nextBit = bit;
            }
        }
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
                // Same net name means the added cone touches an existing wire.
                mergedBits = mergedTop.netnames[netName].bits;
            } else {
                const copiedNet = addedNet;
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
            const copiedPort = port;
            if (Array.isArray(copiedPort.bits)) {
                copiedPort.bits = this._remapSchematicBits(copiedPort.bits, bitRemap);
            }
            mergedTop.ports[portName] = copiedPort;
        }

        for (const [cellName, cell] of Object.entries(addedCloneTop.cells)) {
            if (Object.prototype.hasOwnProperty.call(mergedTop.cells, cellName)) {
                continue;
            }
            const copiedCell = cell;
            if (copiedCell.connections) {
                copiedCell.connections = this._remapSchematicBits(
                    copiedCell.connections,
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

        // netlistsvg usually emits cell groups as "cell_<instance>".
        const prefixed = 'cell_' + instName;
        return this._cellGroupForSvgId(prefixed)
            || this._cellGroupForSvgId(instName);
    }

    _addCellHitTarget(group, cellId) {
        let bbox;
        try {
            bbox = this._groupBoundsWithoutText(group);
        } catch (_) {
            return;
        }

        if (!bbox
            || bbox.width <= 0
            || bbox.height <= 0
            || !Number.isFinite(bbox.x)
            || !Number.isFinite(bbox.y)) {
            return;
        }

        // Capture clicks inside open gate shapes without scanning coordinates.
        const hitTarget = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
        hitTarget.setAttribute('x', String(bbox.x));
        hitTarget.setAttribute('y', String(bbox.y));
        hitTarget.setAttribute('width', String(bbox.width));
        hitTarget.setAttribute('height', String(bbox.height));
        hitTarget.setAttribute('fill', 'transparent');
        hitTarget.setAttribute('stroke', 'none');
        hitTarget.setAttribute('pointer-events', 'all');
        hitTarget.setAttribute('class', cellId);
        hitTarget.setAttribute('data-openroad-hit-target', 'cell');
        group.insertBefore(hitTarget, group.firstChild);
    }

    _registerSvgCellHitTarget(instName, group) {
        if (!this._svgEl || !instName || !group) {
            return;
        }

        // Map both id forms back to the real ODB instance name.
        const prefixed = 'cell_' + instName;
        this._svgIdToInstName.set(prefixed, instName);
        this._svgIdToInstName.set(instName, instName);
        this._addCellHitTarget(group, prefixed);
    }

    async renderNetlist(yosysJson) {
        try {
            this.setStatus('Rendering…');
            // Keep the original netlist for view toggles and cone expansion.
            this._currentNetlist = yosysJson;

            // Debug aid: the last netlist rendered is exposed so it can be
            // captured for the offline render preview tool
            // (src/web/test/visual). In DevTools:
            //   copy(JSON.stringify(window.__lastSchematic))
            if (typeof window !== 'undefined') window.__lastSchematic = yosysJson;

            // Rewrite recognised logic gates to skin types so netlistsvg can
            // place and route the upstream symbols natively.
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
            this.svgContainer.innerHTML = svgString;
            this._svgEl = this.svgContainer.querySelector('svg');
            this._scopeSkinStyles();
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
                this._registerSvgCellHitTarget(
                    instName,
                    this._cellGroupForInstance(instName));
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

            const cellCount = Object.keys(cells).length;
            this.setStatus(`${cellCount} cell${cellCount !== 1 ? 's' : ''}`);
            return true;
        } catch (err) {
            console.error('NetlistSVG render failed:', err);
            this.setStatus(`Render error: ${err.message || err}`);
            return false;
        }
    }

    _svgSkinAttribute(el, attrName) {
        // Browser SVG parsers may expose netlistsvg's `s:*` attributes either way.
        const netlistsvgNS = 'https://github.com/nturley/netlistsvg';
        return el.getAttribute(`s:${attrName}`)
            || el.getAttributeNS(netlistsvgNS, attrName);
    }

    _openRoadPortLabelMap(cell) {
        const labels = new Map();
        if (!cell.port_labels
            || typeof cell.port_labels !== 'object'
            || Array.isArray(cell.port_labels)) {
            return labels;
        }

        for (const [symbolPort, realPort] of Object.entries(cell.port_labels)) {
            labels.set(symbolPort, realPort);
        }
        return labels;
    }

    _isInstanceLabel(label) {
        return label.getAttribute('data-openroad-label') === 'instance'
            || this._svgSkinAttribute(label, 'attribute') === 'ref';
    }

    _textAnchorForLabel(label) {
        if (label.style.textAnchor) return label.style.textAnchor;
        const textAnchor = label.getAttribute('text-anchor');
        if (textAnchor) return textAnchor;
        if (label.classList.contains('inputPortLabel')) return 'end';
        if (label.classList.contains('nodelabel')) return 'middle';
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

        const fontSize = parseFloat(
            label.style.fontSize || label.getAttribute('font-size') || '10') || 10;
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
        // Measure the symbol body, not labels that may be moved around it.
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
                if (display === null) label.removeAttribute('display');
                else label.setAttribute('display', display);
            }
        }
    }

    _parseSvgNumber(value) {
        const parsed = parseFloat(value);
        return Number.isFinite(parsed) ? parsed : null;
    }

    _parseSvgTranslate(transform) {
        const match = (transform || '').match(
            /translate\(\s*([-+]?\d*\.?\d+)(?:[,\s]+([-+]?\d*\.?\d+))?/);
        if (!match) return null;

        const x = this._parseSvgNumber(match[1]);
        const y = this._parseSvgNumber(match[2] !== undefined ? match[2] : '0');
        return x !== null && y !== null ? { x, y } : null;
    }

    _portMarkerForLabel(group, normalizedPort) {
        for (const marker of group.querySelectorAll('g')) {
            if (this._svgSkinAttribute(marker, 'pid') === normalizedPort) {
                return marker;
            }
        }
        return null;
    }

    _portMarkerPosition(marker, rootGroup) {
        let x = this._parseSvgNumber(this._svgSkinAttribute(marker, 'x'));
        let y = this._parseSvgNumber(this._svgSkinAttribute(marker, 'y'));
        const markerTranslate = this._parseSvgTranslate(marker.getAttribute('transform'));
        if (markerTranslate) {
            x = x !== null ? x : markerTranslate.x;
            y = y !== null ? y : markerTranslate.y;
        }

        if (x === null || y === null) return null;

        // Labels are appended to the outer cell group, but skin pin markers can
        // be nested under translated helper groups.
        for (let el = marker.parentElement; el && el !== rootGroup; el = el.parentElement) {
            const translate = this._parseSvgTranslate(
                el.getAttribute && el.getAttribute('transform'));
            if (translate) {
                x += translate.x;
                y += translate.y;
            }
        }

        return { x, y };
    }

    _ensureOpenRoadPortLabelElement(group, normalizedPort) {
        const existing = group.querySelector(
            `text[data-openroad-port="${CSS.escape(normalizedPort)}"]`);
        if (existing) return existing;

        const marker = this._portMarkerForLabel(group, normalizedPort);
        if (!marker) return null;

        const label = document.createElementNS('http://www.w3.org/2000/svg', 'text');
        label.setAttribute('data-openroad-port', normalizedPort);
        group.appendChild(label);
        return label;
    }

    _positionOpenRoadPortLabel(group, label, normalizedPort, direction) {
        const marker = this._portMarkerForLabel(group, normalizedPort);
        if (!marker) return false;

        const pin = this._portMarkerPosition(marker, group);
        if (!pin) return false;

        // Match upstream placement: inputs sit left of the marker, outputs right.
        const isInput = direction === 'input';
        const anchor = isInput ? 'end' : 'start';
        const x = pin.x + (isInput ? -3 : 4);
        const y = pin.y - 4;
        const side = isInput ? 'left' : 'right';

        label.setAttribute('x', String(x));
        label.setAttribute('y', String(y));
        label.setAttribute('data-openroad-port-side', side);
        label.setAttribute('text-anchor', anchor);
        label.style.textAnchor = anchor;
        return true;
    }

    _normalizeSchematicPortText() {
        if (!this._svgEl) return;

        for (const label of this._svgEl.querySelectorAll('text')) {
            if (this._isInstanceLabel(label)) continue;
            this._stylePortLabel(label);
        }
    }

    _updateOpenRoadPortLabels(group, cell) {
        const labels = this._openRoadPortLabelMap(cell);
        const dirs = cell.port_directions || {};
        for (const normalizedPort of labels.keys()) {
            this._ensureOpenRoadPortLabelElement(group, normalizedPort);
        }

        for (const label of group.querySelectorAll('text[data-openroad-port]')) {
            const normalizedPort = label.getAttribute('data-openroad-port');
            label.textContent = labels.get(normalizedPort) || normalizedPort;
            this._positionOpenRoadPortLabel(
                group, label, normalizedPort, dirs[normalizedPort]);
            this._stylePortLabel(label, 5);
        }
    }

    // The OpenROAD skin supplies geometry/ports; real instance and Liberty
    // pin names are added after render.
    _ensureOpenRoadSymbolLabels(netlist) {
        if (!this._svgEl) return;

        const cells = netlist.modules && netlist.modules.top
                    && netlist.modules.top.cells || {};
        const svgNS = 'http://www.w3.org/2000/svg';
        const netlistsvgNS = 'https://github.com/nturley/netlistsvg';

        this._normalizeSchematicPortText();

        for (const [instName, cell] of Object.entries(cells)) {
            if (!cell.port_labels) continue;
            const displayName = (cell.attributes && cell.attributes.ref) || instName;
            const group = this._cellGroupForInstance(instName);
            if (!group) continue;

            let label = null;
            for (const text of group.querySelectorAll('text')) {
                if (this._isInstanceLabel(text)) {
                    label = text;
                    break;
                }
            }

            if (!label) {
                label = document.createElementNS(svgNS, 'text');
                const symbolWidth = parseFloat(this._svgSkinAttribute(group, 'width'));
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
            group.appendChild(label);
        }
    }

    _instanceLabelLines(name) {
        const text = String(name);
        const maxLineChars = 22;
        if (text.length <= maxLineChars) return [text];

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
            if (breakAt === -1) breakAt = maxLineChars;
            lines.push(remaining.slice(0, breakAt));
            remaining = remaining.slice(breakAt);
        }
        if (remaining.length > 0) lines.push(remaining);
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
            if (oldDisplay === null) label.removeAttribute('display');
            else label.setAttribute('display', oldDisplay);
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
            if (oldDisplay === null) label.removeAttribute('display');
            else label.setAttribute('display', oldDisplay);
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
            { x: centerX, y: bodyBounds.y - topGap - labelOffset, anchor: 'middle', preference: 0 },
            { x: centerX, y: bodyBounds.y + bodyBounds.height + 14, anchor: 'middle', preference: 12 },
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
            { x: centerX, y: centerY + 4 - labelOffset / 2, anchor: 'middle', preference: 80 },
        ];
    }

    _wireObstacleRects(cellGroups) {
        if (!this._svgEl) return [];
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

    // NetlistSVG uses fixed skin label locations; reposition instance labels
    // after layout so names stay readable near ports, wires, and other text.
    _layoutInstanceLabels(netlist) {
        if (!this._svgEl) return;

        const cells = netlist.modules && netlist.modules.top
                    && netlist.modules.top.cells || {};
        const records = [];
        for (const instName of Object.keys(cells)) {
            const group = this._cellGroupForInstance(instName);
            if (!group) continue;
            const label = this._instanceLabelForGroup(group);
            if (label) records.push({ group, label });
        }
        if (records.length === 0) return;

        const cellGroups = records.map(({ group }) => group);
        const wireRects = this._wireObstacleRects(cellGroups);
        const movableLabels = new Set(records.map(({ label }) => label));
        // Treat existing text, wires, and cell bodies as label obstacles.
        const occupiedRects = Array.from(this._svgEl.querySelectorAll('text'))
            .filter((text) => !movableLabels.has(text))
            .map((text) => this._expandedScreenRect(text, 2))
            .filter((rect) => rect.width > 0 && rect.height > 0);
        for (const record of records) {
            try {
                const rect = this._groupScreenRectWithoutLabel(record.group, record.label, 5);
                if (rect.width > 0 && rect.height > 0) occupiedRects.push(rect);
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
                // Prefer avoiding wires first, then labels/cell bodies; placement
                // preference only breaks ties between similarly clear positions.
                const score = wireOverlap * 1000 + overlap * 100 + candidate.preference;
                if (!best || score < best.score) best = { candidate, score };
                if (wireOverlap === 0 && overlap === 0 && score === candidate.preference) {
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

        if (!bounds) return { ...rect };
        bounds.x = Math.min(bounds.x, rect.x);
        bounds.y = Math.min(bounds.y, rect.y);
        bounds.right = Math.max(bounds.right, rect.right);
        bounds.bottom = Math.max(bounds.bottom, rect.bottom);
        return bounds;
    }

    _rectToSvgRect(rect, transform) {
        if (!this._svgEl
            || !transform
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
            const point = this._svgEl.createSVGPoint();
            const corners = [
                [rect.x, rect.y],
                [rect.x + rect.width, rect.y],
                [rect.x + rect.width, rect.y + rect.height],
                [rect.x, rect.y + rect.height],
            ].map(([x, y]) => {
                point.x = x;
                point.y = y;
                return point.matrixTransform(transform);
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

    _screenRectToSvgRect(screenRect) {
        const ctm = this._svgEl && this._svgEl.getScreenCTM();
        if (!ctm || !screenRect) return null;

        try {
            return this._rectToSvgRect({
                x: screenRect.left,
                y: screenRect.top,
                width: screenRect.width,
                height: screenRect.height,
            }, ctm.inverse());
        } catch (_) {
            return null;
        }
    }

    _localRectToSvgRect(element, rect) {
        const svgCtm = this._svgEl && this._svgEl.getCTM();
        const elementCtm = element.getCTM && element.getCTM();
        if (!svgCtm || !elementCtm) return null;

        try {
            return this._rectToSvgRect(rect, svgCtm.inverse().multiply(elementCtm));
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
        if (!this._svgEl) return null;

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

    _svgContentElements() {
        if (!this._svgEl) return [];

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
        return this._svgEl.querySelectorAll(selectors.join(','));
    }

    _svgContentBoundsFromSvgBBox() {
        if (!this._svgEl) return null;

        let bounds = null;
        for (const element of this._svgContentElements()) {
            bounds = this._unionSvgRect(bounds, this._elementBBoxToSvgRect(element));
        }
        return bounds;
    }

    _svgContentBoundsFromScreen() {
        if (!this._svgEl) return null;

        let bounds = null;
        for (const element of this._svgContentElements()) {
            const screenRect = element.getBoundingClientRect();
            const svgRect = this._screenRectToSvgRect(screenRect);
            bounds = this._unionSvgRect(bounds, svgRect);
        }
        return bounds;
    }

    // Repositioned labels may extend outside NetlistSVG's original viewBox, so
    // recompute content bounds after label layout before fitting the schematic.
    _padSvgToContent() {
        if (!this._svgEl) return;

        try {
            let bounds = null;
            const bbox = this._svgEl.getBBox();
            bounds = this._unionSvgRect(bounds, {
                x: bbox.x,
                y: bbox.y,
                right: bbox.x + bbox.width,
                bottom: bbox.y + bbox.height,
            });
            // Combine SVG-space and screen-space measurements because browser
            // text layout and transformed skin geometry are exposed differently.
            bounds = this._unionSvgRect(bounds, this._svgContentBoundsFromSvgBBox());
            bounds = this._unionSvgRect(bounds, this._openRoadLabelBounds());
            bounds = this._unionSvgRect(bounds, this._svgContentBoundsFromScreen());
            if (!bounds) return;

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
// The server tags recognised combinational/register cells with `gate_kind`.
// Before rendering we rewrite them to the gate types drawn by openroad_skin.svg
// and remap real Liberty pins to the skin's port ids.

// gate_kind -> skin symbol type (a Yosys primitive alias the skin recognises).
const SKIN_SIMPLE_TYPE = {
    and: '$_AND_', nand: '$_NAND_', or: '$_OR_', nor: '$_NOR_',
    xor: '$_XOR_', xnor: '$_XNOR_', not: '$_NOT_', buf: '$_BUF_',
};

const SKIN_COMPOUND_TYPES = new Set([
    'aoi21', 'aoi22', 'aoi211', 'aoi221', 'aoi222', 'aoi33',
    'oai21', 'oai22', 'oai211', 'oai221', 'oai222', 'oai33',
]);

const SKIN_MULTI_TYPES = new Set([
    'and3', 'and4', 'or3', 'or4', 'nand3', 'nand4', 'nor3', 'nor4',
]);

const SKIN_REGISTER_TYPE = {
    dff: '$_DFF_',
    dffr: '$dffr',
    dffs: '$dffs',
};
const SKIN_REGISTER_TYPES = new Set(Object.keys(SKIN_REGISTER_TYPE));
const PID_LETTERS = ['A', 'B', 'C', 'D', 'E', 'F'];

function normalizedPortMap(cell) {
    const ports = cell && cell.gate_ports;
    if (!ports || typeof ports !== 'object' || Array.isArray(ports)) {
        return null;
    }

    const map = {};
    for (const [symbolPort, realPort] of Object.entries(ports)) {
        if (typeof realPort === 'string' && realPort.length > 0) {
            map[symbolPort] = realPort;
        }
    }
    return Object.keys(map).length > 0 ? map : null;
}

function isDffFamilyType(cell) {
    const type = cell && cell.type;
    if (typeof type !== 'string') return false;
    const upper = type.toUpperCase();
    return upper.startsWith('DFF')
        || upper.includes('_DFF')
        || upper.includes('$DFF');
}

// Some rendered cones contain DFF-shaped cells without backend gate_kind.  Use
// the DFF skin when the visible D/clock/Q pins are present; for DFF-family cell
// names, ignore extra input controls such as enables so the shape still reads as
// a flip-flop instead of a plain box.  Keep this name-gated so unrelated cells
// retain netlistsvg's generic-box fallback.
function inferredRegisterPortMap(cell) {
    const conns = cell && cell.connections;
    const dirs = cell && cell.port_directions;
    if (!conns || !dirs) return null;

    const hasPort = (port) => Object.prototype.hasOwnProperty.call(conns, port);
    const isInput = (port) => hasPort(port) && dirs[port] === 'input';
    const isOutput = (port) => hasPort(port) && dirs[port] === 'output';
    const clock = ['CK', 'CLK', 'C'].find((port) => isInput(port));
    if (!isInput('D') || !clock || (!isOutput('Q') && !isOutput('QN'))) {
        return null;
    }

    const reset = isInput('RN') ? 'RN' : null;
    const set = isInput('SN') ? 'SN' : null;
    if (reset && set) return null;

    const allowed = new Set(['D', clock, 'Q', 'QN']);
    if (reset) allowed.add(reset);
    if (set) allowed.add(set);
    const allowExtraInputs = isDffFamilyType(cell);
    for (const port of Object.keys(conns)) {
        if (allowed.has(port)) continue;
        if (dirs[port] === 'input' && allowExtraInputs) {
            continue;
        }
        if (dirs[port] === 'input' || dirs[port] === 'output') {
            return null;
        }
    }

    const ports = { D: 'D', CK: clock };
    if (reset) ports.RN = reset;
    if (set) ports.SN = set;
    if (isOutput('Q')) ports.Q = 'Q';
    if (isOutput('QN')) ports.QN = 'QN';

    return {
        kind: reset ? 'dffr' : (set ? 'dffs' : 'dff'),
        ports,
    };
}

function skinPidForGatePort(symbolPort) {
    const inputMatch = /^A([1-6])$/.exec(symbolPort);
    if (inputMatch) return PID_LETTERS[Number(inputMatch[1]) - 1] || symbolPort;
    if (symbolPort === 'CK' || symbolPort === 'CLK') return 'C';
    return symbolPort;
}

// Rewrite one cell to an OpenROAD skin gate symbol when it carries a recognised
// `gate_kind`: set its `type` to the canonical symbol name and remap its
// `connections`/`port_directions` keys to the symbol's port ids.  Pins not part
// of the gate (power/ground) are dropped.  Cells that aren't recognised — or
// whose computed type has no symbol — are returned unchanged so they render as
// a labelled generic box with their real pin names.
export function canonicalizeCell(cell) {
    const inferredRegister = cell && !cell.gate_kind
        ? inferredRegisterPortMap(cell)
        : null;
    const kind = cell && (cell.gate_kind
        || (inferredRegister && inferredRegister.kind));
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
    const gatePorts = normalizedPortMap(cell)
        || (inferredRegister && inferredRegister.ports);
    const mapGatePorts = () => {
        for (const [symbolPort, realPort] of Object.entries(gatePorts || {})) {
            pidOf[realPort] = skinPidForGatePort(symbolPort);
        }
    };

    if (SKIN_REGISTER_TYPES.has(kind)) {
        if (!gatePorts) return cell;
        // Registers use explicit skin aliases; plain dff/dffr/dffs can render
        // as generic boxes in some netlistsvg paths.
        type = SKIN_REGISTER_TYPE[kind];
        mapGatePorts();
    } else if (kind === 'aoi' || kind === 'oai') {
        const terms = Array.isArray(cell.gate_terms) ? cell.gate_terms : [];
        if (!terms.length) return cell;
        const sizes = terms.map((t) => t.length);
        type = kind + sizes.slice().sort((a, b) => b - a).join('');
        if (!SKIN_COMPOUND_TYPES.has(type)) return cell;
        // Match the skin's AOI/OAI port layout: single-literal terms use A first.
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
            type = SKIN_SIMPLE_TYPE[kind];
        } else if (n <= 2) {
            type = SKIN_SIMPLE_TYPE[kind];
        } else {
            type = kind + n;
            if (!SKIN_MULTI_TYPES.has(type)) return cell;
        }
        if (!type) return cell;
        if (gatePorts) {
            // Backend gate_ports use A1/A2/CK ids; upstream skin pids are A/B/C.
            mapGatePorts();
        } else {
            inPins.forEach((pin, idx) => {
                pidOf[pin] = PID_LETTERS[idx] || ('I' + idx);
            });
        }
    }
    if (outPin !== null && !pidOf[outPin]) pidOf[outPin] = 'Y';

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
// OpenROAD skin's gate symbols.  Cell keys (instance names) are preserved so
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
