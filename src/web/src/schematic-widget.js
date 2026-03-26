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
        this.controls.querySelector('#schematic-fit').addEventListener('click', () => this.fitView());
        this.controls.querySelector('#schematic-zoom-in').addEventListener('click', () => this._zoomStep(1.5));
        this.controls.querySelector('#schematic-zoom-out').addEventListener('click', () => this._zoomStep(1 / 1.5));
        this.controls.querySelector('#schematic-select').addEventListener('click', () => this._toggleSelectMode());
        this.controls.querySelector('#schematic-zoom-to').addEventListener('click', () => this._zoomToSelected());

        this.appState.schematicWidget = this;

        this.netlistsvg = null;
        this.skin = null;
        this._svgEl = null;

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

    _handleSelectClick(e) {
        if (!this._svgEl) return;

        // netlistsvg renders each cell as <g id="cell_instName" transform="...">
        // Walk up from the click target to the first <g> whose id maps to a
        // known ODB instance (i.e. is present in _svgIdToInstName).
        // This ensures we never try to inspect port/net/wire groups, which
        // would either give "Instance not found" or an unexpected request type.
        let el = e.target;
        let cellGroup = null;
        while (el && el !== this._svgEl) {
            if (el.tagName === 'g' && el.id && this._svgIdToInstName.has(el.id)) {
                cellGroup = el;
                break;
            }
            el = el.parentElement;
        }

        this._clearCellHighlight();
        if (!cellGroup) return;

        // Guaranteed non-undefined because we used .has() above.
        const name = this._svgIdToInstName.get(cellGroup.id);

        // Highlight: append a rect in the cell group's own coordinate space
        // using getBBox() so no transform math is needed.
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
        } catch (_) { /* getBBox can fail on hidden elements */ }

        this.controls.querySelector('#schematic-zoom-to').disabled = false;
        this.setStatus(`Selected: ${name}`);
        this._fetchInspect(name);
    }

    _fetchInspect(instName) {
        const wm = this.appState.websocketManager;
        if (!wm) return;
        console.log('[Schematic] inspect:', instName);
        wm.request({ type: 'schematic_inspect', inst_name: instName })
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
                throw new Error('netlistsvg not loaded — check the <script> tags in index.html');
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
            this.skin = skin;
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
            return;
        }

        if (!this._netlistsvgReady) {
            this.setStatus('NetlistSVG not ready yet — try again in a moment.');
            return;
        }

        const wm = this.appState.websocketManager;
        if (!wm) {
            this.setStatus('Waiting for server connection…');
            return;
        }

        this.setStatus(`Loading schematic for ${instName}…`);

        const faninRaw    = parseInt(this.controls.querySelector('#schematic-fanin-depth').value,  10);
        const fanoutRaw   = parseInt(this.controls.querySelector('#schematic-fanout-depth').value, 10);
        const faninDepth  = isNaN(faninRaw)  ? 1 : faninRaw;
        const fanoutDepth = isNaN(fanoutRaw) ? 1 : fanoutRaw;

        wm.readyPromise.then(() => {
            wm.request({ type: 'schematic_cone', inst_name: instName,
                         fanin_depth: faninDepth, fanout_depth: fanoutDepth })
                .then(data => {
                    const cells = data.modules && data.modules.top && data.modules.top.cells;
                    if (!cells || Object.keys(cells).length === 0) {
                        this.setStatus('No cells found for selected instance.');
                        return;
                    }
                    this.renderNetlist(data);
                })
                .catch(err => {
                    this.setStatus(`Error: ${err}`);
                });
        });
    }

    setStatus(msg) {
        this.controls.querySelector('#schematic-status').textContent = msg;
    }

    // ── Render ───────────────────────────────────────────────────────────────

    async renderNetlist(yosysJson) {
        try {
            this.setStatus('Rendering…');

            // netlistsvg.render() is Promise-based in v1.x (async ELK layout),
            // but older versions used a callback: render(skin, json, done).
            // Support both so the widget works with either bundle.
            let svgString;
            const result = this.netlistsvg.render(this.skin, yosysJson);
            if (result && typeof result.then === 'function') {
                // Promise-based (v1.x)
                svgString = await result;
            } else {
                // Callback-based (older); wrap in a Promise
                svgString = await new Promise((resolve, reject) => {
                    this.netlistsvg.render(this.skin, yosysJson, (err, svg) => {
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

            // Build SVG-id → ODB instance-name map.
            // netlistsvg renders each cell with id="cell_<instName>", so we
            // check each known instance name against that pattern.
            const cells = yosysJson.modules && yosysJson.modules.top
                        && yosysJson.modules.top.cells || {};
            for (const instName of Object.keys(cells)) {
                // Try "cell_<instName>" first (netlistsvg default prefix), then
                // the bare name as a fallback for any other renderer.
                const prefixed = 'cell_' + instName;
                if (this._svgEl && this._svgEl.querySelector(`#${CSS.escape(prefixed)}`)) {
                    this._svgIdToInstName.set(prefixed, instName);
                } else {
                    this._svgIdToInstName.set(instName, instName);
                }
            }
            if (this._svgEl) {
                // Remove any CSS size constraints — transform-based zoom handles sizing
                this._svgEl.style.maxWidth = '';
                this._svgEl.style.height = '';
                this._svgEl.style.display = 'block';
            }

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
}
