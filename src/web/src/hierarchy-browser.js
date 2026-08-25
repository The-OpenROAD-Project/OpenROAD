// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Hierarchy browser widget — module tree with coloring.

import { CheckboxTreeModel } from './checkbox-tree-model.js';
import {
    buildTreeIndex, computeEffectiveColors, fmtArea, fmtInt, isHidden,
    serializeColorMap,
} from './color-tree.js';
import {
    HIERARCHY_OFF_HINT, isStaticMode, makeResizableHeaders,
} from './ui-utils.js';

const COLS = [
    'Instance', 'Module', 'Instances', 'Macros', 'Modules',
    'Area', 'Local Inst', 'Local Macros', 'Local Modules',
];

// Must match HierarchyNodeKind enum on the server.
const NODE_KIND = { MODULE: 0, LEAF_GROUP: 1, TYPE_GROUP: 2, INSTANCE: 3 };

export class HierarchyBrowser {
    // `gate` is the visibility flag this view's overlay draws under, handed
    // down by HierarchyPanel so the source↔flag pairing lives only in its
    // VIEWS table.
    constructor(container, app, redrawAllLayers, gate = 'module_view') {
        this._app = app;
        this._redrawAllLayers = redrawAllLayers;
        this._gate = gate;
        this._loaded = false;
        this._nodes = [];      // flat server response
        this._rows = [];       // DFS-ordered rows with depth
        this._childrenMap = new Map();  // id → [child ids]
        this._nodeMap = new Map();      // id → node
        this._collapsed = new Set();   // collapsed node ids

        // Module coloring state: odb_id → {color, effectiveColor, visible}
        this._moduleState = new Map();

        // Checkbox tree model for module visibility (tri-state propagation).
        this._checkModel = null;

        this._build(container);

        // Auto-load in static mode (data is already cached).
        if (isStaticMode(app)) {
            this.update();
        }
    }

    _build(container) {
        const el = document.createElement('div');
        el.className = 'hierarchy-widget';
        // Kept so HierarchyPanel can show/hide this view and drop its view
        // selector into the toolbar, next to Update.
        this.element = el;

        // Toolbar
        const toolbar = document.createElement('div');
        toolbar.className = 'timing-toolbar';
        this.toolbar = toolbar;

        this._updateBtn = document.createElement('button');
        this._updateBtn.className = 'timing-btn';
        this._updateBtn.textContent = 'Update';
        if (isStaticMode(this._app)) {
            this._updateBtn.style.display = 'none';
        }

        this._statusLabel = document.createElement('span');
        this._statusLabel.className = 'timing-path-count';

        toolbar.appendChild(this._updateBtn);
        toolbar.appendChild(this._statusLabel);
        el.appendChild(toolbar);

        // Table container
        this._tableContainer = document.createElement('div');
        this._tableContainer.className = 'hierarchy-table-container';
        this._table = document.createElement('table');
        this._table.className = 'timing-table';
        this._tableContainer.appendChild(this._table);
        el.appendChild(this._tableContainer);

        container.element.appendChild(el);
        this._bindEvents();
        // Draw the empty state now: _render() otherwise runs for the first time
        // inside update(), so a freshly opened panel was a blank rectangle with
        // no column headers and no sign that Update was what it wanted.
        this._render();
    }

    _bindEvents() {
        this._updateBtn.addEventListener('click', () => this.update());
    }

    async update() {
        this._updateBtn.disabled = true;
        this._updateBtn.textContent = 'Loading...';
        this._statusLabel.textContent = '';
        try {
            const data = await this._app.websocketManager.request({
                type: 'module_hierarchy',
            });
            this._nodes = data.nodes || [];
            this._loaded = true;
            this._buildTree();
            this._readServerColors();
            this._computeEffectiveColors();
            this._render();
            this.refreshStatus();
            await this._sendModuleColors();
        } catch (err) {
            this._statusLabel.textContent = 'Error: ' + err.message;
        }
        this._updateBtn.disabled = false;
        this._updateBtn.textContent = 'Update';
    }

    _buildTree() {
        this._collapsed.clear();
        // Tree indexing, the collapse→color rule, the color-map wire format and
        // the number formatting are shared with the Clusters panel and with the
        // server's save-image path; see color-tree.js.
        const index = buildTreeIndex(this._nodes);
        this._childrenMap = index.childrenMap;
        this._nodeMap = index.nodeMap;
        this._rows = index.rows;

        // Default collapse state:
        // - Modules at depth > 1 are collapsed
        // - Leaf groups and type groups are collapsed
        for (const n of this._nodes) {
            const children = this._childrenMap.get(n.id);
            if (!children || children.length === 0) continue;
            const kind = n.node_kind || 0;
            if (kind === NODE_KIND.LEAF_GROUP || kind === NODE_KIND.TYPE_GROUP) {
                this._collapsed.add(n.id);
            } else if (kind === NODE_KIND.MODULE && n.parent_id >= 0) {
                this._collapsed.add(n.id);
            }
        }

        // Build checkbox model for module visibility.
        // Only MODULE nodes with odb_id get checkboxes; others are structural.
        this._checkModel = new CheckboxTreeModel(() => {
            this._checkModel.forEach(node => {
                if (!node.hasCheckbox) return;
                const st = this._moduleState.get(node.data.odb_id);
                if (st) st.visible = node.checked;
            });
            this._sendModuleColors();
        });
        this._checkModel.buildFromNodes(this._nodes.map(n => ({
            id: n.id,
            parentId: n.parent_id,
            hasCheckbox: (n.node_kind || 0) === NODE_KIND.MODULE
                         && n.odb_id != null,
            checked: true,
            data: n,
        })));
    }

    // The status line: the overlay warning, else the module count.  Mirrors
    // ClustersWidget — with one checkbox for both overlays, a user on this
    // view is the likeliest to tick a module and see nothing happen, so this
    // is the view that most needs to say why.  HierarchyPanel calls it when
    // the Hierarchy view checkbox or the source moves.
    refreshStatus() {
        if (!this._loaded) return;
        const modules = this._nodes.filter(
            n => (n.node_kind || 0) === NODE_KIND.MODULE).length;
        this._statusLabel.textContent
            = this._app.visibility
              && this._app.visibility[this._gate] === false
                ? HIERARCHY_OFF_HINT
                : modules + ' modules';
    }

    // Read server-assigned colors for each MODULE node.
    _readServerColors() {
        this._moduleState.clear();
        for (const row of this._rows) {
            const node = this._nodeMap.get(row.id);
            if (!node || (node.node_kind || 0) !== NODE_KIND.MODULE) continue;
            if (node.odb_id == null) continue;
            const c = node.color || [128, 128, 128];
            this._moduleState.set(node.odb_id, {
                color: c,
                effectiveColor: c,
                visible: true,
                nodeId: node.id,
            });
        }
    }

    // When a MODULE is collapsed, all descendant MODULEs inherit its effective
    // color (highest collapsed ancestor wins).  Structural rows (leaf/type
    // folders) carry no color and are simply absent from _moduleState.
    _computeEffectiveColors() {
        computeEffectiveColors(this._rows, this._nodeMap, this._moduleState,
                               this._collapsed);
    }

    // Drop this view's colors from the session, on the way out.  The overlay
    // only draws while its flag is on AND the session holds a map, so an empty
    // map stops it without touching the flag -- which stays a plain derivation
    // of the Hierarchy view checkbox and the remembered source.
    clearOverlay() {
        this._moduleState.clear();
        // Nothing to clear on a static report: there is no session.
        if (isStaticMode(this._app)) return Promise.resolve();
        return this._sendModuleColors();
    }

    // Send the current effective color map to the server.
    async _sendModuleColors() {
        const colors = serializeColorMap(this._moduleState);
        try {
            await this._app.websocketManager.request({
                type: 'set_module_colors',
                colors,
            });
        } catch (err) {
            console.error('set_module_colors failed:', err);
            return;
        }
        // Only the module overlay reads these colors, so refresh that layer
        // instead of every mounted one — a full redraw re-requests every metal
        // layer's grid for a color they ignore.
        if (this._app.modulesLayer) {
            this._app.modulesLayer.refreshTiles();
        } else {
            this._redrawAllLayers();
        }
    }

    _render() {
        this._table.innerHTML = '';

        // Header
        const thead = document.createElement('thead');
        const hr = document.createElement('tr');
        for (const col of COLS) {
            const th = document.createElement('th');
            th.textContent = col;
            hr.appendChild(th);
        }
        thead.appendChild(hr);
        this._table.appendChild(thead);

        // Body
        const tbody = document.createElement('tbody');
        for (const row of this._rows) {
            const node = this._nodeMap.get(row.id);
            if (!node) continue;

            // Check if any ancestor is collapsed
            if (isHidden(node, this._nodeMap, this._collapsed)) continue;

            const tr = document.createElement('tr');
            const kind = node.node_kind || 0;
            const children = this._childrenMap.get(row.id) || [];
            const hasChildren = children.length > 0;
            const isCollapsed = this._collapsed.has(row.id);

            // Style non-module rows
            if (kind === NODE_KIND.LEAF_GROUP || kind === NODE_KIND.TYPE_GROUP) {
                tr.style.fontStyle = 'italic';
                tr.style.color = 'var(--fg-disabled)';
            } else if (kind === NODE_KIND.INSTANCE) {
                tr.style.color = 'var(--fg-secondary)';
            }

            // Column 0: Instance (with tree indent, color swatch, and arrow)
            const tdInst = document.createElement('td');
            tdInst.style.paddingLeft = (8 + row.depth * 16) + 'px';
            tdInst.style.whiteSpace = 'nowrap';

            // Module color swatch + visibility checkbox
            if (kind === NODE_KIND.MODULE && node.odb_id != null) {
                const st = this._moduleState.get(node.odb_id);
                const modelNode = this._checkModel
                    ? this._checkModel.get(node.id) : null;
                if (st && modelNode) {
                    const cb = document.createElement('input');
                    cb.type = 'checkbox';
                    cb.checked = modelNode.checked;
                    cb.indeterminate = modelNode.indeterminate;
                    cb.className = 'hierarchy-module-cb';
                    cb.setAttribute('aria-label',
                                    'Show module colors: '
                                    + node.inst_name);
                    modelNode.cb = cb;
                    cb.addEventListener('change', (e) => {
                        e.stopPropagation();
                        this._checkModel.check(node.id, cb.checked);
                        this._render();
                    });
                    tdInst.appendChild(cb);

                    const swatch = document.createElement('span');
                    swatch.className = 'hierarchy-color-swatch';
                    const [r, g, b] = st.effectiveColor;
                    swatch.style.backgroundColor = `rgb(${r},${g},${b})`;
                    tdInst.appendChild(swatch);
                }
            }

            if (hasChildren) {
                const arrow = document.createElement('span');
                arrow.className = 'hierarchy-arrow';
                arrow.textContent = isCollapsed ? '▶' : '▼';
                arrow.addEventListener('click', (e) => {
                    e.stopPropagation();
                    this._toggleNode(row.id);
                });
                tdInst.appendChild(arrow);
            } else {
                const spacer = document.createElement('span');
                spacer.className = 'hierarchy-arrow';
                spacer.style.visibility = 'hidden';
                spacer.textContent = '▶';
                tdInst.appendChild(spacer);
            }

            const nameSpan = document.createElement('span');
            nameSpan.textContent = node.inst_name;
            tdInst.appendChild(nameSpan);
            tr.appendChild(tdInst);

            // Columns 1-8
            const vals = [
                node.module_name,
                fmtInt(node.insts),
                fmtInt(node.macros),
                fmtInt(node.modules),
                fmtArea(this._app, node.area),
                fmtInt(node.local_insts),
                fmtInt(node.local_macros),
                fmtInt(node.local_modules),
            ];
            for (const v of vals) {
                const td = document.createElement('td');
                td.textContent = v;
                td.style.textAlign = 'right';
                tr.appendChild(td);
            }
            // Module name column should be left-aligned
            tr.children[1].style.textAlign = 'left';

            tbody.appendChild(tr);
        }

        if (this._rows.length === 0) {
            const tr = document.createElement('tr');
            const td = document.createElement('td');
            td.colSpan = COLS.length;
            td.style.textAlign = 'center';
            td.style.color = 'var(--fg-secondary)';
            td.textContent = isStaticMode(this._app) ?
                'No hierarchy data available' :
                'Click "Update" to load hierarchy';
            tr.appendChild(td);
            tbody.appendChild(tr);
        }

        this._table.appendChild(tbody);
        makeResizableHeaders(this._table);
    }

    _toggleNode(id) {
        if (this._collapsed.has(id)) {
            this._collapsed.delete(id);
        } else {
            this._collapsed.add(id);
        }
        // Recompute effective colors since collapse state changed
        this._computeEffectiveColors();
        this._render();
        this._sendModuleColors();
    }

}
