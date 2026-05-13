// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// DRC viewer widget — category selector, violation tree, load/highlight.
// Matches the capabilities of the Qt GUI DRCWidget.

import { dbuRectToBounds } from './coordinates.js';

export class DrcWidget {
    constructor(app, redrawAllLayers) {
        this._app = app;
        this._redrawAllLayers = redrawAllLayers;
        this._categories = [];
        this._activeCategory = '';
        this._markerTree = null;  // server response for current category
        this._expandedNodes = new Set();

        this._build();
        this._app.websocketManager.readyPromise.then(() => this.refresh());
    }

    _build() {
        const el = document.createElement('div');
        el.className = 'drc-widget';

        const toolbar = document.createElement('div');
        toolbar.className = 'drc-toolbar';

        this._categorySelect = document.createElement('select');
        this._categorySelect.className = 'drc-category-select';
        toolbar.appendChild(this._categorySelect);

        this._loadBtn = document.createElement('button');
        this._loadBtn.className = 'drc-btn';
        this._loadBtn.textContent = 'Load...';
        toolbar.appendChild(this._loadBtn);

        el.appendChild(toolbar);

        this._infoBar = document.createElement('div');
        this._infoBar.className = 'drc-info-bar';
        el.appendChild(this._infoBar);

        this._treeContainer = document.createElement('div');
        this._treeContainer.className = 'drc-tree-container';
        el.appendChild(this._treeContainer);

        this.element = el;
        this._bindEvents();
    }

    _bindEvents() {
        this._categorySelect.addEventListener('change', () => {
            this._activeCategory = this._categorySelect.value;
            this._loadMarkers();
        });

        this._loadBtn.addEventListener('click', () => {
            this._showLoadDialog();
        });
    }

    _loadCategories() {
        return this._app.websocketManager.request({ type: 'drc_categories' })
            .then(data => {
                this._categories = data.categories || [];
                this._renderCategorySelect();
            })
            .catch(err => {
                console.error('Failed to load DRC categories:', err);
            });
    }

    refresh() {
        this._loadCategories().then(() => {
            // Auto-select first category if none is active
            if (!this._activeCategory && this._categories.length > 0) {
                this._activeCategory = this._categories[0].name;
                this._categorySelect.value = this._activeCategory;
            }
            if (this._activeCategory) {
                this._loadMarkers();
            }
        });
    }

    _renderCategorySelect() {
        const select = this._categorySelect;
        const prev = select.value;
        select.innerHTML = '';

        const empty = document.createElement('option');
        empty.value = '';
        empty.textContent = '(none)';
        select.appendChild(empty);

        for (const cat of this._categories) {
            const opt = document.createElement('option');
            opt.value = cat.name;
            opt.textContent = `${cat.name} (${cat.count})`;
            select.appendChild(opt);
        }

        // Restore previous selection or auto-select if there's an active category
        if (this._activeCategory && this._categories.some(c => c.name === this._activeCategory)) {
            select.value = this._activeCategory;
        } else if (prev && this._categories.some(c => c.name === prev)) {
            select.value = prev;
            this._activeCategory = prev;
        } else {
            this._activeCategory = '';
        }
    }

    _loadMarkers() {
        if (!this._activeCategory) {
            this._markerTree = null;
            this._treeContainer.innerHTML = '';
            this._infoBar.textContent = '';
            this._redrawAllLayers();
            return;
        }

        this._treeContainer.innerHTML = '<div class="drc-loading">Loading...</div>';

        this._app.websocketManager.request({
            type: 'drc_markers',
            category: this._activeCategory
        }).then(data => {
            this._markerTree = data;
            this._renderTree();
            this._redrawAllLayers();
        }).catch(err => {
            this._treeContainer.innerHTML = '';
            const errDiv = document.createElement('div');
            errDiv.className = 'drc-error';
            errDiv.textContent = `Error: ${err}`;
            this._treeContainer.appendChild(errDiv);
            console.error('Failed to load DRC markers:', err);
        });
    }

    _renderTree() {
        this._treeContainer.innerHTML = '';

        if (!this._markerTree) return;

        if (this._markerTree.error) {
            const errDiv = document.createElement('div');
            errDiv.className = 'drc-error';
            errDiv.textContent = this._markerTree.error;
            this._treeContainer.appendChild(errDiv);
            return;
        }

        const total = this._markerTree.total_count || 0;
        this._infoBar.textContent = `${total} total violation${total !== 1 ? 's' : ''}`;

        const subcats = this._markerTree.subcategories || [];
        const directMarkers = this._markerTree.markers || [];
        const topNodes = [...subcats];
        if (directMarkers.length > 0) {
            topNodes.unshift({
                name: this._markerTree.name || 'Markers',
                count: directMarkers.length,
                markers: directMarkers,
            });
        }
        if (topNodes.length === 0) {
            this._treeContainer.innerHTML = '<div class="drc-empty">No violations</div>';
            return;
        }

        const tree = document.createElement('div');
        tree.className = 'drc-tree';

        for (const node of topNodes) {
            tree.appendChild(this._buildCategoryNode(node));
        }

        this._treeContainer.appendChild(tree);
    }

    _buildCategoryNode(category) {
        const node = document.createElement('div');
        node.className = 'drc-tree-node';

        const header = document.createElement('div');
        header.className = 'drc-tree-header';

        const toggle = document.createElement('span');
        toggle.className = 'drc-toggle';
        const hasChildren = (category.subcategories && category.subcategories.length > 0)
            || (category.markers && category.markers.length > 0);
        const nodeKey = category.name;
        const expanded = this._expandedNodes.has(nodeKey);
        toggle.textContent = hasChildren ? (expanded ? '\u25BC' : '\u25B6') : ' ';
        header.appendChild(toggle);

        const checkbox = document.createElement('input');
        checkbox.type = 'checkbox';
        checkbox.checked = this._allMarkersVisible(category);
        checkbox.className = 'drc-visibility-check';
        checkbox.addEventListener('change', (e) => {
            e.stopPropagation();
            this._toggleCategoryVisibility(category, checkbox.checked);
        });
        header.appendChild(checkbox);

        const label = document.createElement('span');
        label.className = 'drc-category-label';
        label.textContent = category.name;
        header.appendChild(label);

        const count = document.createElement('span');
        count.className = 'drc-count';
        count.textContent = `${category.count} markers`;
        header.appendChild(count);

        node.appendChild(header);

        const children = document.createElement('div');
        children.className = 'drc-tree-children';
        children.style.display = expanded ? '' : 'none';

        if (category.subcategories) {
            for (const sub of category.subcategories) {
                children.appendChild(this._buildCategoryNode(sub));
            }
        }

        if (category.markers) {
            for (const marker of category.markers) {
                children.appendChild(this._buildMarkerNode(marker));
            }
        }

        node.appendChild(children);

        if (hasChildren) {
            header.addEventListener('click', (e) => {
                if (e.target === checkbox) return;
                const isExpanded = children.style.display !== 'none';
                children.style.display = isExpanded ? 'none' : '';
                toggle.textContent = isExpanded ? '\u25B6' : '\u25BC';
                if (isExpanded) {
                    this._expandedNodes.delete(nodeKey);
                } else {
                    this._expandedNodes.add(nodeKey);
                }
            });
            header.style.cursor = 'pointer';
        }

        return node;
    }

    _buildMarkerNode(marker) {
        const row = document.createElement('div');
        row.className = 'drc-marker-row';
        row.dataset.markerId = marker.id;
        if (!marker.visited) {
            row.classList.add('drc-unvisited');
        }

        const checkbox = document.createElement('input');
        checkbox.type = 'checkbox';
        checkbox.checked = marker.visible;
        checkbox.className = 'drc-visibility-check';
        checkbox.addEventListener('change', (e) => {
            e.stopPropagation();
            marker.visible = checkbox.checked;
            this._updateMarker(marker.id, 'visible', checkbox.checked);
        });
        row.appendChild(checkbox);

        const idx = document.createElement('span');
        idx.className = 'drc-marker-index';
        idx.textContent = marker.index;
        row.appendChild(idx);

        const name = document.createElement('span');
        name.className = 'drc-marker-name';
        name.textContent = marker.name;
        if (marker.layer) {
            name.title = `Layer: ${marker.layer}`;
        }
        row.appendChild(name);

        if (marker.layer) {
            const layerBadge = document.createElement('span');
            layerBadge.className = 'drc-layer-badge';
            layerBadge.textContent = marker.layer;
            row.appendChild(layerBadge);
        }

        row.addEventListener('click', (e) => {
            if (e.target === checkbox) return;
            this._highlightMarker(marker);
        });

        row.addEventListener('dblclick', (e) => {
            if (e.target === checkbox) return;
            this._highlightMarker(marker, true);
        });

        return row;
    }

    _highlightMarker(marker, openInspector = false) {
        this._app.websocketManager.request({
            type: 'drc_highlight',
            marker_id: marker.id
        }).then(data => {
            if (data.ok && data.bbox) {
                marker.visited = true;
                const row = this._treeContainer.querySelector(
                    `.drc-marker-row[data-marker-id="${marker.id}"]`);
                if (row) row.classList.remove('drc-unvisited');

                this._zoomToBBox(data.bbox);
                this._redrawAllLayers();

                if (openInspector && this._app.focusComponent) {
                    this._app.focusComponent('Inspector');
                }
            }
        }).catch(err => {
            console.error('Failed to highlight DRC marker:', err);
        });
    }

    _zoomToBBox(bbox) {
        if (!this._app.designScale || !this._app.map) return;

        const [xMin, yMin, xMax, yMax] = bbox;
        const dx = xMax - xMin;
        const dy = yMax - yMin;

        // Match the Qt GUI zoom logic (mainWindow.cpp selectDRC handler):
        //   zoomout_dist = 10 microns
        //   zoomout_box  = 2 * max(bbox width, bbox height)
        //   margin       = min(zoomout_dist, zoomout_box)
        const dbuPerMicron = this._app.getDbuPerMicron();
        const zoomoutDist = 10 * dbuPerMicron;
        const zoomoutBox = 2 * Math.max(dx, dy);
        const margin = Math.min(zoomoutDist, zoomoutBox);

        this._app.map.fitBounds(
            dbuRectToBounds(
                xMin - margin, yMin - margin,
                xMax + margin, yMax + margin,
                this._app.designScale, this._app.designMaxDXDY,
                this._app.designOriginX, this._app.designOriginY));
    }

    _updateMarker(markerId, field, value) {
        this._app.websocketManager.request({
            type: 'drc_update_marker',
            marker_id: markerId,
            field: field,
            value: !!value
        }).then(() => {
            this._redrawAllLayers();
            if (field === 'visible') {
                this._update3DHighlights();
                if (value) {
                    this._flashHighlight();
                }
            }
        }).catch(err => {
            console.error('Failed to update DRC marker:', err);
        });
    }

    _flashHighlight() {
        const container = this._app.map?.getContainer();
        if (!container) return;
        container.classList.remove('drc-flashing');
        // Force reflow so re-adding the class restarts the animation.
        void container.offsetWidth;
        container.classList.add('drc-flashing');
        container.addEventListener('animationend', () => {
            container.classList.remove('drc-flashing');
        }, { once: true });
    }

    _update3DHighlights() {
        const viewer = this._app.threeDViewerWidget;
        if (!viewer) return;

        const visible = [];
        this._collectVisibleMarkers(this._markerTree, visible);

        if (visible.length > 0) {
            viewer.highlightDRC(visible);
        } else {
            viewer.clearHighlightDRC();
        }
    }

    _collectVisibleMarkers(node, out) {
        if (!node) return;
        if (node.markers) {
            for (const m of node.markers) {
                if (m.visible) {
                    out.push(m);
                }
            }
        }
        if (node.subcategories) {
            for (const sub of node.subcategories) {
                this._collectVisibleMarkers(sub, out);
            }
        }
    }

    _setMarkersVisible(node, visible) {
        if (!node) return;
        if (node.markers) {
            for (const m of node.markers) {
                m.visible = visible;
            }
        }
        if (node.subcategories) {
            for (const sub of node.subcategories) {
                this._setMarkersVisible(sub, visible);
            }
        }
    }

    _snapshotMarkerVisibility(node, out = new Map()) {
        if (!node) return out;
        if (node.markers) {
            for (const m of node.markers) {
                out.set(m.id, m.visible);
            }
        }
        if (node.subcategories) {
            for (const sub of node.subcategories) {
                this._snapshotMarkerVisibility(sub, out);
            }
        }
        return out;
    }

    _restoreMarkerVisibility(node, snapshot) {
        if (!node) return;
        if (node.markers) {
            for (const m of node.markers) {
                if (snapshot.has(m.id)) {
                    m.visible = snapshot.get(m.id);
                }
            }
        }
        if (node.subcategories) {
            for (const sub of node.subcategories) {
                this._restoreMarkerVisibility(sub, snapshot);
            }
        }
    }

    _allMarkersVisible(category) {
        if (category.markers) {
            for (const m of category.markers) {
                if (!m.visible) return false;
            }
        }
        if (category.subcategories) {
            for (const sub of category.subcategories) {
                if (!this._allMarkersVisible(sub)) return false;
            }
        }
        return true;
    }

    _toggleCategoryVisibility(category, visible) {
        const snapshot = this._snapshotMarkerVisibility(category);
        this._setMarkersVisible(category, visible);
        this._renderTree();

        this._app.websocketManager.request({
            type: 'drc_update_category_visibility',
            category: category.name,
            visible: !!visible
        }).then(() => {
            this._redrawAllLayers();
            this._update3DHighlights();
            if (visible) {
                this._flashHighlight();
            }
        }).catch(err => {
            console.error('Failed to update category visibility:', err);
            this._restoreMarkerVisibility(category, snapshot);
            this._renderTree();
        });
    }

    _showLoadDialog() {
        const dialog = new DrcFileDialog(this._app, (category) => {
            this._activeCategory = category;
            this._loadCategories().then(() => {
                this._categorySelect.value = this._activeCategory;
                this._loadMarkers();
            });
        });
        dialog.open();
    }

    // Called externally to select a specific category (e.g. from inspector)
    selectCategory(name) {
        this._activeCategory = name;
        this._categorySelect.value = name;
        this._loadMarkers();
    }
}

// Modal file browser for selecting a DRC report on the server. The dialog owns
// its DOM overlay and tracks listeners so that `close()` fully tears down.
class DrcFileDialog {
    constructor(app, onLoaded) {
        this._app = app;
        this._onLoaded = onLoaded;
        this._currentPath = '';
        this._disposers = [];
        this._closed = false;
    }

    open() {
        this._overlay = document.createElement('div');
        this._overlay.className = 'modal-overlay';
        this._overlay.innerHTML = `
            <div class="modal-dialog file-browser-dialog">
                <h3>Load DRC Report</h3>
                <div class="fb-breadcrumb"></div>
                <div class="fb-file-list"></div>
                <input type="text" class="fb-path-input"
                       placeholder="Server-side path to .rpt, .drc, or .json file">
                <div class="modal-error" style="display:none"></div>
                <div class="modal-buttons">
                    <button class="cancel">Cancel</button>
                    <button class="primary ok" disabled>Load</button>
                </div>
            </div>`;
        document.body.appendChild(this._overlay);

        this._breadcrumb = this._overlay.querySelector('.fb-breadcrumb');
        this._fileList = this._overlay.querySelector('.fb-file-list');
        this._pathInput = this._overlay.querySelector('.fb-path-input');
        this._errorDiv = this._overlay.querySelector('.modal-error');
        this._okBtn = this._overlay.querySelector('.ok');
        this._cancelBtn = this._overlay.querySelector('.cancel');

        this._listen(this._pathInput, 'input', () => this._updateOkState());
        this._listen(this._pathInput, 'keydown', (e) => {
            if (e.key === 'Enter') {
                const val = this._pathInput.value.trim();
                if (val.endsWith('/')) {
                    this._navigate(val.replace(/\/+$/, '') || '/');
                } else {
                    this._submit();
                }
            } else if (e.key === 'Escape') {
                this.close();
            }
        });
        this._listen(this._okBtn, 'click', () => this._submit());
        this._listen(this._cancelBtn, 'click', () => this.close());
        this._listen(this._overlay, 'click', (e) => {
            if (e.target === this._overlay) this.close();
        });

        this._navigate('');
    }

    close() {
        if (this._closed) return;
        this._closed = true;
        for (const off of this._disposers) off();
        this._disposers.length = 0;
        if (this._overlay) {
            this._overlay.remove();
            this._overlay = null;
        }
    }

    _listen(target, event, handler) {
        target.addEventListener(event, handler);
        this._disposers.push(() => target.removeEventListener(event, handler));
    }

    _updateOkState() {
        this._okBtn.disabled = !this._pathInput.value.trim();
    }

    _renderBreadcrumb(dirPath) {
        this._breadcrumb.innerHTML = '';
        const parts = dirPath.split('/').filter(Boolean);

        const rootSpan = document.createElement('span');
        rootSpan.className = 'fb-crumb';
        rootSpan.textContent = '/';
        this._listen(rootSpan, 'click', () => this._navigate('/'));
        this._breadcrumb.appendChild(rootSpan);

        let accumulated = '';
        for (const part of parts) {
            accumulated += '/' + part;
            const sep = document.createElement('span');
            sep.className = 'fb-crumb-sep';
            sep.textContent = ' / ';
            this._breadcrumb.appendChild(sep);

            const crumb = document.createElement('span');
            crumb.className = 'fb-crumb';
            crumb.textContent = part;
            const target = accumulated;
            this._listen(crumb, 'click', () => this._navigate(target));
            this._breadcrumb.appendChild(crumb);
        }
    }

    _renderEntries(entries) {
        this._fileList.innerHTML = '';
        if (!entries || entries.length === 0) {
            const empty = document.createElement('div');
            empty.className = 'fb-empty';
            empty.textContent = '(empty directory)';
            this._fileList.appendChild(empty);
            return;
        }

        for (const entry of entries) {
            const row = document.createElement('div');
            row.className = 'fb-entry';
            if (entry.is_dir) row.classList.add('fb-dir');

            const icon = document.createElement('span');
            icon.className = 'fb-icon';
            icon.textContent = entry.is_dir ? '\u{1F4C1}' : '\u{1F4C4}';
            row.appendChild(icon);

            const name = document.createElement('span');
            name.className = 'fb-name';
            name.textContent = entry.name;
            row.appendChild(name);

            if (entry.is_dir) {
                this._listen(row, 'click',
                    () => this._navigate(this._currentPath + '/' + entry.name));
            } else {
                this._listen(row, 'click', () => {
                    const prev = this._fileList.querySelector('.fb-selected');
                    if (prev) prev.classList.remove('fb-selected');
                    row.classList.add('fb-selected');
                    this._pathInput.value = this._currentPath + '/' + entry.name;
                    this._updateOkState();
                });
                this._listen(row, 'dblclick', () => {
                    this._pathInput.value = this._currentPath + '/' + entry.name;
                    this._updateOkState();
                    this._submit();
                });
            }

            this._fileList.appendChild(row);
        }
    }

    async _navigate(dirPath) {
        this._errorDiv.style.display = 'none';
        this._fileList.innerHTML = '<div class="fb-loading">Loading...</div>';
        try {
            const resp = await this._app.websocketManager.request({
                type: 'list_dir',
                path: dirPath || '',
            });
            if (this._closed) return;
            this._currentPath = resp.path;
            this._renderBreadcrumb(resp.path);
            this._renderEntries(resp.entries);
            this._pathInput.value = '';
            this._updateOkState();
        } catch (err) {
            if (this._closed) return;
            this._fileList.innerHTML = '';
            this._errorDiv.textContent = err.message || String(err);
            this._errorDiv.style.display = '';
        }
    }

    async _submit() {
        const path = this._pathInput.value.trim();
        if (!path) return;

        this._okBtn.disabled = true;
        this._okBtn.textContent = 'Loading...';
        this._errorDiv.style.display = 'none';

        try {
            const resp = await this._app.websocketManager.request({
                type: 'drc_load_report',
                path,
            });
            if (this._closed) return;
            if (resp.ok) {
                const category = resp.category;
                this.close();
                this._onLoaded(category);
            } else {
                this._errorDiv.textContent = resp.error || 'Failed to load report';
                this._errorDiv.style.display = '';
                this._okBtn.disabled = false;
                this._okBtn.textContent = 'Load';
            }
        } catch (err) {
            if (this._closed) return;
            this._errorDiv.textContent = err.message || 'Request failed';
            this._errorDiv.style.display = '';
            this._okBtn.disabled = false;
            this._okBtn.textContent = 'Load';
        }
    }
}
