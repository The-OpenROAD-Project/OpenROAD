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
        this._loadCategories();
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

        this._refreshBtn = document.createElement('button');
        this._refreshBtn.className = 'drc-btn';
        this._refreshBtn.textContent = 'Refresh';
        toolbar.appendChild(this._refreshBtn);

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

        this._refreshBtn.addEventListener('click', () => {
            this._loadCategories();
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
        if (subcats.length === 0) {
            this._treeContainer.innerHTML = '<div class="drc-empty">No violations</div>';
            return;
        }

        const tree = document.createElement('div');
        tree.className = 'drc-tree';

        for (const subcat of subcats) {
            tree.appendChild(this._buildCategoryNode(subcat));
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
            this._toggleCategoryVisibility(category, checkbox.checked, node);
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
        const dbuPerMicron = this._app.techData?.dbu_per_micron || 1000;
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
            value: value ? 1 : 0
        }).then(() => {
            this._redrawAllLayers();
        }).catch(err => {
            console.error('Failed to update DRC marker:', err);
        });
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

    _toggleCategoryVisibility(category, visible, nodeEl) {
        this._app.websocketManager.request({
            type: 'drc_update_category_visibility',
            category: category.name,
            visible: visible ? 1 : 0
        }).then(() => {
            const checks = nodeEl.querySelectorAll(
                '.drc-marker-row > .drc-visibility-check');
            for (const cb of checks) {
                cb.checked = visible;
            }
            const catChecks = nodeEl.querySelectorAll(
                '.drc-tree-header > .drc-visibility-check');
            for (const cb of catChecks) {
                cb.checked = visible;
            }
            this._redrawAllLayers();
        }).catch(err => {
            console.error('Failed to update category visibility:', err);
        });
    }

    _showLoadDialog() {
        const overlay = document.createElement('div');
        overlay.className = 'modal-overlay';

        overlay.innerHTML = `
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

        document.body.appendChild(overlay);

        const breadcrumb = overlay.querySelector('.fb-breadcrumb');
        const fileList = overlay.querySelector('.fb-file-list');
        const pathInput = overlay.querySelector('.fb-path-input');
        const errorDiv = overlay.querySelector('.modal-error');
        const okBtn = overlay.querySelector('.ok');
        const cancelBtn = overlay.querySelector('.cancel');
        let currentPath = '';

        const close = () => overlay.remove();

        const updateOkState = () => {
            const val = pathInput.value.trim();
            okBtn.disabled = !val;
        };

        const renderBreadcrumb = (dirPath) => {
            breadcrumb.innerHTML = '';
            const parts = dirPath.split('/').filter(Boolean);
            const rootSpan = document.createElement('span');
            rootSpan.className = 'fb-crumb';
            rootSpan.textContent = '/';
            rootSpan.addEventListener('click', () => navigate('/'));
            breadcrumb.appendChild(rootSpan);

            let accumulated = '';
            for (const part of parts) {
                accumulated += '/' + part;
                const sep = document.createElement('span');
                sep.className = 'fb-crumb-sep';
                sep.textContent = ' / ';
                breadcrumb.appendChild(sep);

                const crumb = document.createElement('span');
                crumb.className = 'fb-crumb';
                crumb.textContent = part;
                const target = accumulated;
                crumb.addEventListener('click', () => navigate(target));
                breadcrumb.appendChild(crumb);
            }
        };

        const renderEntries = (entries) => {
            fileList.innerHTML = '';
            if (!entries || entries.length === 0) {
                const empty = document.createElement('div');
                empty.className = 'fb-empty';
                empty.textContent = '(empty directory)';
                fileList.appendChild(empty);
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
                    row.addEventListener('click', () => {
                        navigate(currentPath + '/' + entry.name);
                    });
                } else {
                    row.addEventListener('click', () => {
                        const prev = fileList.querySelector('.fb-selected');
                        if (prev) prev.classList.remove('fb-selected');
                        row.classList.add('fb-selected');
                        pathInput.value = currentPath + '/' + entry.name;
                        updateOkState();
                    });
                    row.addEventListener('dblclick', () => {
                        pathInput.value = currentPath + '/' + entry.name;
                        updateOkState();
                        submit();
                    });
                }

                fileList.appendChild(row);
            }
        };

        const navigate = async (dirPath) => {
            errorDiv.style.display = 'none';
            fileList.innerHTML = '<div class="fb-loading">Loading...</div>';
            try {
                const resp = await this._app.websocketManager.request({
                    type: 'list_dir',
                    path: dirPath || '',
                });
                currentPath = resp.path;
                renderBreadcrumb(resp.path);
                renderEntries(resp.entries);
                pathInput.value = '';
                updateOkState();
            } catch (err) {
                fileList.innerHTML = '';
                errorDiv.textContent = err.message || String(err);
                errorDiv.style.display = '';
            }
        };

        const submit = async () => {
            const path = pathInput.value.trim();
            if (!path) return;

            okBtn.disabled = true;
            okBtn.textContent = 'Loading...';
            errorDiv.style.display = 'none';

            try {
                const resp = await this._app.websocketManager.request({
                    type: 'drc_load_report',
                    path: path,
                });

                if (resp.ok) {
                    close();
                    this._activeCategory = resp.category;
                    this._loadCategories().then(() => {
                        this._categorySelect.value = this._activeCategory;
                        this._loadMarkers();
                    });
                } else {
                    errorDiv.textContent = resp.error || 'Failed to load report';
                    errorDiv.style.display = '';
                    okBtn.disabled = false;
                    okBtn.textContent = 'Load';
                }
            } catch (err) {
                errorDiv.textContent = err.message || 'Request failed';
                errorDiv.style.display = '';
                okBtn.disabled = false;
                okBtn.textContent = 'Load';
            }
        };

        pathInput.addEventListener('input', updateOkState);
        pathInput.addEventListener('keydown', (e) => {
            if (e.key === 'Enter') {
                const val = pathInput.value.trim();
                if (val.endsWith('/')) {
                    navigate(val.replace(/\/+$/, '') || '/');
                } else {
                    submit();
                }
            }
            if (e.key === 'Escape') close();
        });
        okBtn.addEventListener('click', submit);
        cancelBtn.addEventListener('click', close);
        overlay.addEventListener('click', (e) => { if (e.target === overlay) close(); });

        navigate('');
    }

    // Called externally to select a specific category (e.g. from inspector)
    selectCategory(name) {
        this._activeCategory = name;
        this._categorySelect.value = name;
        this._loadMarkers();
    }
}
