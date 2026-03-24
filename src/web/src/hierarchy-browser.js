// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Hierarchy browser widget — module tree with coloring.

import { CheckboxTreeModel } from './checkbox-tree-model.js';
import { makeResizableHeaders } from './ui-utils.js';

const COLS = [
    'Instance', 'Module', 'Instances', 'Macros', 'Modules',
    'Area', 'Local Inst', 'Local Macros', 'Local Modules',
];

// Must match HierarchyNodeKind enum on the server.
const NODE_KIND = { MODULE: 0, LEAF_GROUP: 1, TYPE_GROUP: 2, INSTANCE: 3 };

// 31-color palette matching the Qt GUI's ColorGenerator.
const MODULE_COLORS = [
    [255,0,0], [255,140,0], [255,215,0], [0,255,0], [148,0,211],
    [0,250,154], [220,20,60], [0,255,255], [0,191,255], [0,0,255],
    [173,255,47], [218,112,214], [255,0,255], [30,144,255], [250,128,114],
    [176,224,230], [255,20,147], [123,104,238], [255,250,205], [255,182,193],
    [85,107,47], [139,69,19], [72,61,139], [0,128,0], [60,179,113],
    [184,134,11], [0,139,139], [0,0,139], [50,205,50], [128,0,128],
    [176,48,96],
];

export class HierarchyBrowser {
    constructor(container, app, redrawAllLayers) {
        this._app = app;
        this._redrawAllLayers = redrawAllLayers;
        this._nodes = [];      // flat server response
        this._rows = [];       // DFS-ordered rows with depth
        this._childrenMap = new Map();  // id → [child ids]
        this._collapsed = new Set();   // collapsed node ids

        // Module coloring state: odb_id → {color, effectiveColor, visible}
        this._moduleState = new Map();

        // Checkbox tree model for module visibility (tri-state propagation).
        this._checkModel = null;

        this._build(container);

        // Expose on app so display-controls can interact
        app.hierarchyBrowser = this;
    }

    _build(container) {
        const el = document.createElement('div');
        el.className = 'hierarchy-widget';

        // Toolbar
        const toolbar = document.createElement('div');
        toolbar.className = 'timing-toolbar';

        this._updateBtn = document.createElement('button');
        this._updateBtn.className = 'timing-btn';
        this._updateBtn.textContent = 'Update';

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
            this._buildTree();
            this._assignColors();
            this._computeEffectiveColors();
            this._render();
            const nMods = this._nodes.filter(
                n => (n.node_kind || 0) === NODE_KIND.MODULE).length;
            this._statusLabel.textContent = nMods + ' modules';
            await this._sendModuleColors();
        } catch (err) {
            this._statusLabel.textContent = 'Error: ' + err.message;
        }
        this._updateBtn.disabled = false;
        this._updateBtn.textContent = 'Update';
    }

    _buildTree() {
        this._childrenMap.clear();
        this._collapsed.clear();

        // Initialize children lists
        for (const n of this._nodes) {
            this._childrenMap.set(n.id, []);
        }
        for (const n of this._nodes) {
            if (n.parent_id >= 0 && this._childrenMap.has(n.parent_id)) {
                this._childrenMap.get(n.parent_id).push(n.id);
            }
        }

        // Build id → node lookup
        this._nodeMap = new Map();
        for (const n of this._nodes) {
            this._nodeMap.set(n.id, n);
        }

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

        // Build DFS-ordered rows
        this._rows = [];
        const roots = this._nodes.filter(n => n.parent_id < 0);
        for (const root of roots) {
            this._dfs(root.id, 0);
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

    // Assign a color from the palette to each MODULE node in DFS order.
    _assignColors() {
        this._moduleState.clear();
        let colorIdx = 0;
        for (const row of this._rows) {
            const node = this._nodeMap.get(row.id);
            if (!node || (node.node_kind || 0) !== NODE_KIND.MODULE) continue;
            if (node.odb_id == null) continue;
            const c = MODULE_COLORS[colorIdx % MODULE_COLORS.length];
            this._moduleState.set(node.odb_id, {
                color: c,
                effectiveColor: c,
                visible: true,
                nodeId: node.id,
            });
            colorIdx++;
        }
    }

    // DFS to compute effective colors based on collapse state.
    // When a MODULE is collapsed, all descendant MODULEs inherit its effective color.
    _computeEffectiveColors() {
        for (const row of this._rows) {
            const node = this._nodeMap.get(row.id);
            if (!node || (node.node_kind || 0) !== NODE_KIND.MODULE) continue;
            const st = this._moduleState.get(node.odb_id);
            if (!st) continue;

            // Find nearest ancestor MODULE that is collapsed
            let parentId = node.parent_id;
            let inheritedColor = null;
            while (parentId >= 0) {
                const parent = this._nodeMap.get(parentId);
                if (!parent) break;
                if ((parent.node_kind || 0) === NODE_KIND.MODULE) {
                    const pst = this._moduleState.get(parent.odb_id);
                    if (pst && this._collapsed.has(parent.id)) {
                        inheritedColor = pst.effectiveColor;
                        // Don't break — keep walking up, the highest
                        // collapsed ancestor's effective color wins.
                    }
                }
                parentId = parent.parent_id;
            }
            st.effectiveColor = inheritedColor || st.color;
        }
    }

    // Check if a node has MODULE children (not just LEAF_GROUP/TYPE_GROUP).
    _hasModuleChildren(nodeId) {
        const children = this._childrenMap.get(nodeId) || [];
        return children.some(cid => {
            const c = this._nodeMap.get(cid);
            return c && (c.node_kind || 0) === NODE_KIND.MODULE;
        });
    }

    // Send the current effective color map to the server.
    // Expanded modules with sub-modules are excluded — their "background"
    // instances stay uncolored so child module colors are clearly visible.
    async _sendModuleColors() {
        const parts = [];
        for (const [odbId, st] of this._moduleState) {
            if (!st.visible) continue;
            // Skip expanded modules that have child modules — only
            // collapsed or leaf modules contribute to the color overlay.
            const expanded = !this._collapsed.has(st.nodeId);
            if (expanded && this._hasModuleChildren(st.nodeId)) continue;
            const [r, g, b] = st.effectiveColor;
            parts.push(`${odbId}:${r},${g},${b},100`);
        }
        const colors = parts.join(';');
        try {
            const resp = await this._app.websocketManager.request({
                type: 'set_module_colors',
                colors,
            });
            console.log('set_module_colors:', resp.count, 'modules,',
                         parts.length, 'sent');
        } catch (err) {
            console.error('set_module_colors failed:', err);
        }

        // Refresh the modules layer if it exists
        if (this._app.modulesLayer && this._app.map.hasLayer(this._app.modulesLayer)) {
            this._app.modulesLayer.refreshTiles();
        }
    }

    _dfs(id, depth) {
        this._rows.push({ id, depth });
        const children = this._childrenMap.get(id) || [];
        for (const childId of children) {
            this._dfs(childId, depth + 1);
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
            if (this._isHidden(node)) continue;

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
                fmtArea(node.area),
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
        this._table.appendChild(tbody);
        makeResizableHeaders(this._table);
    }

    _isHidden(node) {
        // Walk up parents; if any ancestor is collapsed, this node is hidden
        let parentId = node.parent_id;
        while (parentId >= 0) {
            if (this._collapsed.has(parentId)) return true;
            const parent = this._nodeMap.get(parentId);
            if (!parent) break;
            parentId = parent.parent_id;
        }
        return false;
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

function fmtInt(v) {
    return v != null ? String(v) : '';
}

function fmtArea(v) {
    if (v == null) return '';
    if (v >= 1e6) return (v / 1e6).toFixed(3) + ' mm²';
    return v.toFixed(3) + ' μm²';
}
