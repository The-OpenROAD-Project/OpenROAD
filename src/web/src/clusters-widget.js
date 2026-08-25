// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Clusters widget — dbGroup tree with per-cluster coloring, mirroring
// HierarchyBrowser.  A checkbox feeds the cluster's color to the `_clusters`
// layer and a double click zooms to it; the row itself does nothing, as in
// HierarchyBrowser.

import { CheckboxTreeModel } from './checkbox-tree-model.js';
import {
    buildTreeIndex, computeEffectiveColors, fmtArea, fmtInt, isHidden,
    serializeColorMap,
} from './color-tree.js';
import {
    HIERARCHY_OFF_HINT, isStaticMode, makeResizableHeaders, zoomToBBox,
} from './ui-utils.js';

const COLS = [
    'Group', 'Type', 'Instances', 'Macros', 'Subgroups',
    'Area', 'Local Inst', 'Local Macros',
];

export class ClustersWidget {
    // `gate` is the visibility flag this view's overlay draws under, handed
    // down by HierarchyPanel so the source↔flag pairing lives only in its
    // VIEWS table.
    constructor(container, app, redrawAllLayers, gate = 'cluster_view') {
        this._app = app;
        this._redrawAllLayers = redrawAllLayers;
        this._gate = gate;
        this._nodes = [];               // flat server response
        this._rows = [];                // DFS-ordered rows with depth
        this._childrenMap = new Map();  // id → [child ids]
        this._nodeMap = new Map();      // id → node
        this._collapsed = new Set();    // collapsed node ids

        // Cluster coloring state: odb_id → {color, effectiveColor, visible}
        this._groupState = new Map();

        // Checkbox tree model for cluster visibility (tri-state propagation).
        this._checkModel = null;

        // Distinguishes "nothing loaded yet" from "loaded, and this design has
        // no clusters" — the two need opposite advice, and conflating them told
        // the user to press Update right after they pressed it.
        this._loaded = false;

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

        this._tableContainer = document.createElement('div');
        this._tableContainer.className = 'hierarchy-table-container';
        this._table = document.createElement('table');
        this._table.className = 'timing-table';
        this._tableContainer.appendChild(this._table);
        el.appendChild(this._tableContainer);

        container.element.appendChild(el);
        this._updateBtn.addEventListener('click', () => this.update());
        this._installTableHandlers();
        // Draw the empty state now: _render() otherwise runs for the first time
        // inside update(), so a freshly opened panel was a blank rectangle with
        // no column headers and no sign that Update was what it wanted.
        this._render();
    }

    // Delegated once on the table, for the panel's whole life: per-row
    // listeners made a checkbox click cost 530 ms at 5000 rows.
    _installTableHandlers() {
        // The row's node, or null off any row.  `data-row-id` is the node id
        // buildTreeIndex keyed the map by, so the node carries it back.
        const rowOf = (e) => {
            const tr = e.target.closest ? e.target.closest('tr[data-row-id]')
                                        : null;
            return tr ? this._nodeMap.get(Number(tr.dataset.rowId)) || null
                      : null;
        };

        this._table.addEventListener('change', (e) => {
            const cb = e.target;
            if (cb.tagName !== 'INPUT' || cb.type !== 'checkbox') return;
            const node = rowOf(e);
            if (!node || !this._checkModel) return;
            this._checkModel.check(node.id, cb.checked);
            // A check changes checkbox states (down the subtree and up the
            // ancestors) and nothing else — the tree shape and the colors are
            // untouched — so the rows do not need rebuilding.
            this._syncCheckboxes();
        });

        // Only the arrow acts on a click.  A double click delivers two clicks
        // before the dblclick, and acting on the second one would undo the
        // collapse the first one made.
        this._table.addEventListener('click', (e) => {
            if (e.detail > 1) return;
            if (!e.target.classList.contains('hierarchy-arrow')) return;
            const node = rowOf(e);
            if (!node) return;
            if ((this._childrenMap.get(node.id) || []).length > 0) {
                this._toggleNode(node.id);
            }
        });

        this._table.addEventListener('dblclick', (e) => {
            const node = rowOf(e);
            if (node) this._zoomToNode(node);
        });
    }

    // Push the model's tri-state onto the checkboxes already in the DOM.  The
    // model caches each row's element (see _render), so this touches only the
    // inputs whose state actually changed shape.
    _syncCheckboxes() {
        if (!this._checkModel) return;
        this._checkModel.forEach(node => {
            if (!node.hasCheckbox || !node.cb) return;
            // Written only where it differs: assigning either property is a
            // style invalidation, and a click on one box walks every node.
            if (node.cb.checked !== node.checked) {
                node.cb.checked = node.checked;
            }
            if (node.cb.indeterminate !== node.indeterminate) {
                node.cb.indeterminate = node.indeterminate;
            }
        });
    }

    async update() {
        this._updateBtn.disabled = true;
        this._updateBtn.textContent = 'Loading...';
        this._statusLabel.textContent = '';
        try {
            const data = await this._app.websocketManager.request({
                type: 'group_hierarchy',
            });
            this._nodes = data.nodes || [];
            this._loaded = true;
            this._buildTree();
            this._readServerColors();
            this._computeEffectiveColors();
            this._render();
            this.refreshStatus();
            await this._sendGroupColors();
        } catch (err) {
            this._statusLabel.textContent = 'Error: ' + err.message;
        }
        this._updateBtn.disabled = false;
        this._updateBtn.textContent = 'Update';
    }

    _buildTree() {
        this._collapsed.clear();
        const index = buildTreeIndex(this._nodes);
        this._childrenMap = index.childrenMap;
        this._nodeMap = index.nodeMap;
        this._rows = index.rows;

        // Every non-root cluster with children starts collapsed, so a
        // top-level cluster paints its whole subtree in one color.
        // computeDefaultGroupColors() on the server mirrors this.
        for (const n of this._nodes) {
            const children = this._childrenMap.get(n.id);
            if (!children || children.length === 0) continue;
            if (n.parent_id >= 0) {
                this._collapsed.add(n.id);
            }
        }

        this._checkModel = new CheckboxTreeModel(() => {
            this._checkModel.forEach(node => {
                if (!node.hasCheckbox) return;
                const st = this._groupState.get(node.data.odb_id);
                if (st) st.visible = node.checked;
            });
            this._sendGroupColors();
            this.refreshStatus();
        });
        this._checkModel.buildFromNodes(this._nodes.map(n => ({
            id: n.id,
            parentId: n.parent_id,
            hasCheckbox: n.odb_id != null,
            checked: true,
            data: n,
        })));
    }

    _readServerColors() {
        this._groupState.clear();
        for (const node of this._nodes) {
            if (node.odb_id == null) continue;
            const c = node.color || [128, 128, 128];
            this._groupState.set(node.odb_id, {
                color: c,
                effectiveColor: c,
                visible: true,
            });
        }
    }

    // A collapsed cluster paints its whole subtree; the rule lives in
    // color-tree.js, shared with the Hierarchy view and the server.
    _computeEffectiveColors() {
        computeEffectiveColors(this._rows, this._nodeMap, this._groupState,
                               this._collapsed);
    }

    // The status line: the overlay warning, else the cluster count.  The one
    // writer, called on every change that can flip it — a checkbox here, a
    // load, and the Hierarchy view checkbox or a source switch from outside
    // (HierarchyPanel).  Without the warning the checkboxes change nothing on
    // screen and ticking one looks like it did nothing at all.
    refreshStatus() {
        // Before the first Update there is no count to show, and writing one
        // would talk over the table's "click Update" placeholder.
        if (!this._loaded) return;
        // The layer's own gate, not ui_hierarchy_view: it is what decides
        // whether this view's colors reach the tiles.
        this._statusLabel.textContent
            = this._app.visibility
              && this._app.visibility[this._gate] === false
                ? HIERARCHY_OFF_HINT
                : this._nodes.length + ' groups';
    }

    // Drop this view's colors from the session, on the way out.  The overlay
    // only draws while its flag is on AND the session holds a map, so an empty
    // map stops it without touching the flag -- which stays a plain derivation
    // of the Hierarchy view checkbox and the remembered source.
    clearOverlay() {
        this._groupState.clear();
        // Nothing to clear on a static report: there is no session.
        if (isStaticMode(this._app)) return Promise.resolve();
        return this._sendGroupColors();
    }

    async _sendGroupColors() {
        try {
            await this._app.websocketManager.request({
                type: 'set_group_colors',
                colors: serializeColorMap(this._groupState),
            });
        } catch (err) {
            console.error('set_group_colors failed:', err);
            return;
        }
        // Only the cluster overlay reads these colors, so refresh that layer
        // instead of every mounted one — a full redraw re-requests every metal
        // layer's grid for a color they ignore.
        if (this._app.clustersLayer) {
            this._app.clustersLayer.refreshTiles();
        } else {
            this._redrawAllLayers();
        }
    }

    _zoomToNode(node) {
        // A cluster with no members reports a zero box; nothing to zoom to.
        const bbox = node.bbox;
        if (!bbox || bbox[2] <= bbox[0] || bbox[3] <= bbox[1]) return;
        zoomToBBox(this._app, bbox);
    }

    _render() {
        this._table.innerHTML = '';

        const thead = document.createElement('thead');
        const hr = document.createElement('tr');
        for (const col of COLS) {
            const th = document.createElement('th');
            th.textContent = col;
            hr.appendChild(th);
        }
        thead.appendChild(hr);
        this._table.appendChild(thead);

        const tbody = document.createElement('tbody');
        for (const row of this._rows) {
            const node = this._nodeMap.get(row.id);
            if (!node || isHidden(node, this._nodeMap, this._collapsed)) continue;

            const tr = document.createElement('tr');
            tr.dataset.rowId = String(row.id);
            const children = this._childrenMap.get(row.id) || [];
            const isCollapsed = this._collapsed.has(row.id);

            // Column 0: name (tree indent, checkbox, swatch, arrow)
            const tdName = document.createElement('td');
            tdName.style.paddingLeft = (8 + row.depth * 16) + 'px';
            tdName.style.whiteSpace = 'nowrap';

            const st = this._groupState.get(node.odb_id);
            const modelNode = this._checkModel
                ? this._checkModel.get(node.id) : null;
            if (st && modelNode) {
                const cb = document.createElement('input');
                cb.type = 'checkbox';
                cb.checked = modelNode.checked;
                cb.indeterminate = modelNode.indeterminate;
                cb.className = 'hierarchy-module-cb';
                cb.setAttribute('aria-label',
                                'Show cluster colors: ' + node.name);
                // Kept on the model so _syncCheckboxes can update it in
                // place; the change handler is delegated on the table.
                modelNode.cb = cb;
                tdName.appendChild(cb);

                const swatch = document.createElement('span');
                swatch.className = 'hierarchy-color-swatch';
                const [r, g, b] = st.effectiveColor;
                swatch.style.backgroundColor = `rgb(${r},${g},${b})`;
                tdName.appendChild(swatch);
            }

            const arrow = document.createElement('span');
            arrow.className = 'hierarchy-arrow';
            if (children.length > 0) {
                arrow.textContent = isCollapsed ? '▶' : '▼';
            } else {
                arrow.style.visibility = 'hidden';
                arrow.textContent = '▶';
            }
            tdName.appendChild(arrow);

            const nameSpan = document.createElement('span');
            nameSpan.textContent = node.name;
            tdName.appendChild(nameSpan);
            tr.appendChild(tdName);

            const vals = [
                node.type,
                fmtInt(node.insts),
                fmtInt(node.macros),
                fmtInt(node.groups),
                fmtArea(this._app, node.area),
                fmtInt(node.local_insts),
                fmtInt(node.local_macros),
            ];
            for (const v of vals) {
                const td = document.createElement('td');
                td.textContent = v;
                td.style.textAlign = 'right';
                tr.appendChild(td);
            }
            tr.children[1].style.textAlign = 'left';

            tbody.appendChild(tr);
        }

        if (this._rows.length === 0) {
            const tr = document.createElement('tr');
            const td = document.createElement('td');
            td.colSpan = COLS.length;
            td.style.textAlign = 'center';
            td.style.color = 'var(--fg-secondary)';
            if (isStaticMode(this._app)) {
                td.textContent = 'No instance group data available';
            } else if (this._loaded) {
                // The design was read and simply has no dbGroups: say so, and
                // say what produces them.  This is the common case for an ODB
                // written by a normal flow run.
                td.textContent = 'This design has no instance groups. Run '
                    + 'rtl_macro_placer -keep_clustering_data (before tapcells) '
                    + 'to store MPL\'s clustering in the ODB.';
            } else {
                td.textContent = 'Click "Update" to load instance groups';
            }
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
        // Collapse state changed, so the inherited colors did too.
        this._computeEffectiveColors();
        this._render();
        this._sendGroupColors();
    }

}
