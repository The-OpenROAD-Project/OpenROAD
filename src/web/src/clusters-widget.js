// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Clusters widget — dbGroup tree with per-cluster coloring.
//
// MPL stores its clustering hierarchy in ODB as nested dbGroups of type
// VISUAL_DEBUG when `rtl_macro_placer -keep_clustering_data` is used, which is
// what makes the soft-macro placement inspectable after the fact (issue
// #7959).  Other dbGroup users (power/voltage domains) show up here too, told
// apart by the Type column.
//
// The tree mirrors HierarchyBrowser: a checkbox controls whether the cluster
// contributes its color to the `_clusters` tile layer, the swatch shows the
// effective color, and collapsing a node makes its subtree paint in the
// node's own color.  Clicking a row selects the dbGroup — the Inspector
// describes it and the layout paints *only* that cluster's instances, in its
// own color (see _selectRow); clicking it again brings the other clusters
// back.  Double-clicking zooms to the cluster's bounding box.

import { CheckboxTreeModel } from './checkbox-tree-model.js';
import {
    buildTreeIndex, computeEffectiveColors, fmtArea, fmtInt, isHidden,
    serializeColorMap,
} from './color-tree.js';
import {
    beginSelection, isCurrentSelection, isStaticMode, makeResizableHeaders,
    onSelectionReset, zoomToBBox,
} from './ui-utils.js';

const COLS = [
    'Cluster', 'Type', 'Instances', 'Macros', 'Groups',
    'Area', 'Local Inst', 'Local Macros',
];

export class ClustersWidget {
    constructor(container, app, redrawAllLayers) {
        this._app = app;
        this._redrawAllLayers = redrawAllLayers;
        this._nodes = [];               // flat server response
        this._rows = [];                // DFS-ordered rows with depth
        this._childrenMap = new Map();  // id → [child ids]
        this._nodeMap = new Map();      // id → node
        this._collapsed = new Set();    // collapsed node ids

        // Cluster coloring state: odb_id → {color, effectiveColor, visible}
        this._groupState = new Map();

        // Checkbox tree model for cluster visibility (tri-state propagation).
        this._checkModel = null;

        this._selectedOdbId = null;
        // Distinguishes "nothing loaded yet" from "loaded, and this design has
        // no clusters" — the two need opposite advice, and conflating them told
        // the user to press Update right after they pressed it.
        this._loaded = false;

        this._build(container);

        // Expose on app so display-controls / tests can interact.
        app.clustersWidget = this;

        // Another panel taking the selection leaves this row's highlight
        // claiming a selection the server no longer holds — and its isolated
        // color map showing one cluster for a selection that moved elsewhere.
        //
        // Guarded on being the live panel: the resetter list has no unregister,
        // so a panel closed and reopened by the layout would otherwise leave its
        // dead predecessors resending the full map on every selection, undoing
        // the isolation the live one just set up.
        onSelectionReset(app, () => {
            if (app.clustersWidget !== this) return;
            const wasIsolated = this._selectedOdbId != null;
            this._clearSelectedRow();
            if (wasIsolated) {
                this._sendGroupColors();
            }
        });

        // Auto-load in static mode (data is already cached).
        if (isStaticMode(app)) {
            this.update();
        }
    }

    _build(container) {
        const el = document.createElement('div');
        el.className = 'hierarchy-widget';

        const toolbar = document.createElement('div');
        toolbar.className = 'timing-toolbar';

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

    // One set of listeners on the table, resolving the row from the event, for
    // the whole life of the panel.  Attaching them per row instead cost 2-4
    // closures per row on every rebuild: measured on this widget, a checkbox
    // click went from 14 ms at 100 rows to 530 ms at 5000, all of it DOM churn.
    _installTableHandlers() {
        const rowOf = (e) => {
            const tr = e.target.closest ? e.target.closest('tr[data-row-id]')
                                        : null;
            if (!tr) return null;
            const id = Number(tr.dataset.rowId);
            const node = this._nodeMap.get(id);
            return node ? { tr, id, node } : null;
        };
        const isCheckbox = (t) => t.tagName === 'INPUT' && t.type === 'checkbox';

        this._table.addEventListener('change', (e) => {
            if (!isCheckbox(e.target)) return;
            const row = rowOf(e);
            if (!row || !this._checkModel) return;
            this._checkModel.check(row.id, e.target.checked);
            // A check changes checkbox states (down the subtree and up the
            // ancestors) and nothing else — the tree shape and the colors are
            // untouched — so the rows do not need rebuilding.
            this._syncCheckboxes();
        });

        this._table.addEventListener('click', (e) => {
            // The checkbox reports through `change`; letting its click through
            // here would also select the row.
            if (isCheckbox(e.target)) return;
            // A double click delivers two clicks before `dblclick`.  Acting on
            // the second one undid the first: it selected the cluster and then
            // deselected it, so zooming to a cluster left it unselected, with the
            // layout flashing between the isolated and the full color map.  The
            // collapse arrow had the same problem — two toggles back to where it
            // started.
            if (e.detail > 1) return;
            const row = rowOf(e);
            if (!row) return;
            if (e.target.classList.contains('hierarchy-arrow')) {
                if ((this._childrenMap.get(row.id) || []).length > 0) {
                    this._toggleNode(row.id);
                }
                return;
            }
            this._selectRow(row.tr, row.node);
        });

        this._table.addEventListener('dblclick', (e) => {
            const row = rowOf(e);
            if (row) this._zoomToNode(row.node);
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
            this._statusLabel.textContent = this._nodes.length + ' clusters';
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

        // A reload can hand back a tree without the selected cluster in it; the
        // selection has to go with it, or the isolation below would filter the
        // color map down to a cluster that no longer exists and paint nothing.
        if (this._selectedOdbId != null
            && !this._nodes.some(n => n.odb_id === this._selectedOdbId)) {
            this._selectedOdbId = null;
        }

        // Default collapse state: every non-root cluster that has children
        // starts collapsed, so a top-level cluster paints its whole subtree in
        // one color.  computeDefaultGroupColors() on the server mirrors this,
        // which is what keeps `save_image -web` consistent with the viewer.
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
            // Hiding the selected cluster (directly, or via an ancestor whose
            // uncheck propagated down to it) must release its highlight too,
            // otherwise its instances stay lit while the panel shows it off.
            const selected = this._selectedOdbId != null
                ? this._groupState.get(this._selectedOdbId) : null;
            if (selected && !selected.visible) {
                // Resends the colors itself, so this must not do it again.
                this._deselectCluster(this._selectedOdbId);
            } else {
                this._sendGroupColors();
            }
            this._hintIfClusterViewOff();
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

    // When a cluster is collapsed, all descendants inherit its effective color
    // (the highest collapsed ancestor wins) — the rule lives in color-tree.js,
    // shared with the Hierarchy panel and mirrored by the server's save-image
    // path.
    _computeEffectiveColors() {
        computeEffectiveColors(this._rows, this._nodeMap, this._groupState,
                               this._collapsed);
    }

    // The clusters the color map is narrowed to while one is selected: the
    // selected cluster and everything under it, since the `_clusters` layer
    // keys off each instance's own dbGroup and a map holding only the top
    // cluster would leave the nested ones unpainted.  null = no selection, so
    // every checked cluster paints.
    //
    // Derived rather than stored: a cached set is a second copy of the
    // selection that every path changing it has to remember to repair, and a
    // stale one filters the map down to nothing.
    _isolatedOdbIds() {
        if (this._selectedOdbId == null) return null;
        const selected
            = this._nodes.find(n => n.odb_id === this._selectedOdbId);
        if (!selected) return null;
        const ids = new Set();
        const walk = (id) => {
            const node = this._nodeMap.get(id);
            if (node && node.odb_id != null) {
                ids.add(node.odb_id);
            }
            for (const childId of this._childrenMap.get(id) || []) {
                walk(childId);
            }
        };
        walk(selected.id);
        return ids;
    }

    // Warn when the cluster overlay is off: neither the checkboxes nor a
    // selection change anything on screen until it is on, so a click on a row
    // would otherwise look like it did nothing at all.
    _hintIfClusterViewOff() {
        if (this._app.visibility
            && this._app.visibility.cluster_view === false) {
            this._statusLabel.textContent = 'Cluster view is off — enable '
                + 'Display Controls → Cluster view → Colors';
            return true;
        }
        return false;
    }

    // Status line for a freshly selected cluster: the two reasons its color
    // may not show up, else back to the cluster count.
    _setSelectionStatus(node) {
        const st = node && node.odb_id != null
            ? this._groupState.get(node.odb_id) : null;
        if (st && !st.visible) {
            this._statusLabel.textContent
                = 'Cluster is hidden — tick its checkbox to color it';
            return;
        }
        if (this._hintIfClusterViewOff()) return;
        this._statusLabel.textContent = this._nodes.length + ' clusters';
    }

    async _sendGroupColors() {
        try {
            await this._app.websocketManager.request({
                type: 'set_group_colors',
                colors: serializeColorMap(this._groupState, undefined,
                                          this._isolatedOdbIds()),
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

    _clearSelectedRow() {
        // Found in the DOM rather than remembered: _render() rebuilds the table
        // on every change, so a kept row reference would be detached half the
        // time while _selectedOdbId stays true either way.
        const row = this._table.querySelector('tr.selected');
        if (row) {
            row.classList.remove('selected');
        }
        // Dropping the selection drops the isolation with it (_isolatedOdbIds
        // reads this field), so the map goes back to whatever the checkboxes
        // say.  The colors are not resent here — callers do that once they know
        // whether a new selection follows, so switching clusters costs one
        // set_group_colors, not two.
        this._selectedOdbId = null;
    }

    // Drop `odbId` from the server's selection so its instances stop being
    // painted in its color.  Used when the row is clicked again and when the
    // cluster is hidden: leaving the isolation up would color a cluster the
    // panel says is neither selected nor visible.
    _deselectCluster(odbId) {
        this._clearSelectedRow();
        // Back to the full map: every checked cluster paints again.
        this._sendGroupColors();
        this._statusLabel.textContent = this._nodes.length + ' clusters';
        if (isStaticMode(this._app)) return;
        // Same ownership discipline as _selectRow: a deselect that lands after
        // another panel has taken the selection must not clear the Inspector
        // out from under it.
        const token = beginSelection(this._app);
        this._app.websocketManager.request({
            type: 'select_group',
            odb_id: odbId,
            deselect: true,
            use_dbu: this._app.showDbu,
        }).then(data => {
            if (!isCurrentSelection(this._app, token)) return;
            if (this._app.updateInspector) {
                this._app.updateInspector(data);
            }
            if (this._app.refreshOverlay) {
                this._app.refreshOverlay();
            }
        }).catch(err => console.error('select_group (deselect) failed:', err));
    }

    // Select the dbGroup so the Inspector shows its properties, and isolate it
    // in the layout: its instances — and those of its nested clusters — are the
    // only ones the `_clusters` overlay paints, each in the color its swatch
    // shows.  That overlay draws per tile from the spatial index, so it shows
    // the whole cluster no matter how many instances it holds, which the
    // selection highlight could not (kMaxHighlightShapes).  Hence `no_highlight`
    // below: the yellow veil would sit on top of the very color being shown.
    _selectRow(tr, node) {
        if (isStaticMode(this._app)) return;
        // Clicking the selected row again is how a selection is undone from
        // here — the panel is the only place that can.
        if (this._selectedOdbId === node.odb_id) {
            this._deselectCluster(node.odb_id);
            return;
        }
        // Cleared before beginSelection, which runs this panel's own reset
        // handler: with the selection already gone the handler has nothing to
        // restore, so the isolated map below is the only one sent.  Two color
        // maps in flight per click is one round trip too many — and, with more
        // than one server io thread, an order the session could apply backwards.
        this._clearSelectedRow();
        const token = beginSelection(this._app);
        this._selectedOdbId = node.odb_id;
        tr.classList.add('selected');

        this._sendGroupColors();
        this._setSelectionStatus(node);

        this._app.websocketManager.request({
            type: 'select_group',
            odb_id: node.odb_id,
            no_highlight: true,
            use_dbu: this._app.showDbu,
        }).then(data => {
            // Clicks can land out of order; only the newest one may drive the
            // Inspector.
            if (!isCurrentSelection(this._app, token)) return;
            if (this._app.updateInspector) {
                this._app.updateInspector(data);
            }
            if (this._app.refreshOverlay) {
                this._app.refreshOverlay();
            }
        }).catch(err => {
            console.error('select_group failed:', err);
            if (isCurrentSelection(this._app, token)) {
                this._clearSelectedRow();
                // The cluster is not selected after all; drop the isolation so
                // the layout does not keep showing one cluster on its own.
                this._sendGroupColors();
            }
        });
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

            if (!isStaticMode(this._app)) {
                tr.style.cursor = 'pointer';
            }
            // The table is rebuilt on every collapse/checkbox change, so the
            // selected row has to be re-marked from the remembered cluster id
            // — otherwise the highlight in the layout would have no visible
            // owner in the panel.
            if (node.odb_id != null && node.odb_id === this._selectedOdbId) {
                tr.classList.add('selected');
            }
            tbody.appendChild(tr);
        }

        if (this._rows.length === 0) {
            const tr = document.createElement('tr');
            const td = document.createElement('td');
            td.colSpan = COLS.length;
            td.style.textAlign = 'center';
            td.style.color = 'var(--fg-secondary)';
            if (isStaticMode(this._app)) {
                td.textContent = 'No cluster data available';
            } else if (this._loaded) {
                // The design was read and simply has no dbGroups: say so, and
                // say what produces them.  This is the common case for an ODB
                // written by a normal flow run.
                td.textContent = 'This design has no clusters. Run '
                    + 'rtl_macro_placer -keep_clustering_data (before tapcells) '
                    + 'to store MPL\'s clustering in the ODB.';
            } else {
                td.textContent = 'Click "Update" to load clusters';
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
