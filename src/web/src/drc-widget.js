// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// DRC Viewer widget — matches the OpenROAD Qt DRC Viewer layout:
//   [Category Dropdown]  [Load...]
//   Type  |  Violation
//   ─────────────────────
//   ▶ Category (N markers)
//     1  |  RuleName
//     2  |  RuleName

import { dbuRectToBounds } from './coordinates.js';

export class DRCWidget {
    constructor(container, app, redrawAllLayers) {
        this._app = app;
        this._redrawAllLayers = redrawAllLayers;
        this._categories = [];         // [{name, count, violations}]
        this._activeCategory = null;   // name of selected category
        this._selectedIndex = -1;      // flat violation index
        this._collapsed = new Set();   // collapsed category names
        this._visibleIndexes = new Set(); // indexes of currently visible violations

        this._build(container);
    }

    _build(container) {
        const el = document.createElement('div');
        el.className = 'drc-widget';

        // --- Top bar: category dropdown + Load button ---
        const topBar = document.createElement('div');
        topBar.className = 'drc-topbar';

        this._categorySelect = document.createElement('select');
        this._categorySelect.className = 'drc-category-select';

        this._loadBtn = document.createElement('button');
        this._loadBtn.className = 'drc-btn';
        this._loadBtn.textContent = 'Load...';

        topBar.appendChild(this._categorySelect);
        topBar.appendChild(this._loadBtn);
        el.appendChild(topBar);

        // --- Tree table: Type | Violation ---
        this._tableContainer = document.createElement('div');
        this._tableContainer.className = 'drc-table-container';
        this._tableContainer.setAttribute('tabindex', '0');
        this._tableContainer.style.outline = 'none';

        this._table = document.createElement('table');
        this._table.className = 'drc-table';

        const thead = document.createElement('thead');
        const hr = document.createElement('tr');
        for (const label of ['Vis', 'Type', 'Violation']) {
            const th = document.createElement('th');
            th.textContent = label;
            hr.appendChild(th);
        }
        thead.appendChild(hr);
        this._table.appendChild(thead);

        this._tbody = document.createElement('tbody');
        this._table.appendChild(this._tbody);
        this._tableContainer.appendChild(this._table);
        el.appendChild(this._tableContainer);

        container.element.appendChild(el);

        this._bindEvents();
        // Populate an empty dropdown, then try to load existing categories
        this._refreshDropdown();
        this.update();
    }

    _bindEvents() {
        this._categorySelect.addEventListener('change', () => {
            this._activeCategory = this._categorySelect.value || null;
            this._selectedIndex = -1;
            this._clearHighlight();
            this._renderBody();
        });

        this._loadBtn.addEventListener('click', () => this._showLoadDialog());

        this._tableContainer.addEventListener('keydown', (e) => {
            const visibleViolations = this._getVisibleViolations();
            const pos = visibleViolations.findIndex(
                v => v.index === this._selectedIndex);
            if (e.key === 'ArrowDown') {
                e.preventDefault();
                if (pos < visibleViolations.length - 1) {
                    this._selectViolation(visibleViolations[pos + 1]);
                }
            } else if (e.key === 'ArrowUp') {
                e.preventDefault();
                if (pos > 0) {
                    this._selectViolation(visibleViolations[pos - 1]);
                }
            }
        });
    }

    // Returns violations of the currently active category (or all if none selected)
    _getVisibleViolations() {
        if (!this._activeCategory) return [];
        const cat = this._categories.find(c => c.name === this._activeCategory);
        return cat ? cat.violations : [];
    }

    setHasChipInsts(hasChipInsts) {
        this._hasChipInsts = hasChipInsts;
    }

    // ─── Update (refresh from backend) ────────────────────────────────────────

    async update() {
        try {
            await this._app.websocketManager.readyPromise;
            const data = await this._app.websocketManager.request({
                type: 'drc_report',
            });
            this._categories = data.categories || [];
            // Keep current selection if still valid, else default to first
            const stillValid = this._categories.some(
                c => c.name === this._activeCategory);
            if (!stillValid) {
                this._activeCategory = this._categories.length > 0
                    ? this._categories[0].name : null;
            }
            this._selectedIndex = -1;
            this._refreshDropdown();
            this._renderBody();
            this._clearHighlight();
        } catch (e) {
            console.error('DRC fetch failed:', e);
        }
    }

    // ─── Dropdown ─────────────────────────────────────────────────────────────

    _refreshDropdown() {
        this._categorySelect.replaceChildren();

        if (this._categories.length === 0) {
            const opt = document.createElement('option');
            opt.value = '';
            opt.textContent = '(no categories)';
            this._categorySelect.appendChild(opt);
            this._categorySelect.disabled = true;
            return;
        }

        this._categorySelect.disabled = false;
        for (const cat of this._categories) {
            const opt = document.createElement('option');
            opt.value = cat.name;
            opt.textContent = `${cat.name} (${cat.count})`;
            if (cat.name === this._activeCategory) opt.selected = true;
            this._categorySelect.appendChild(opt);
        }
    }

    // ─── Tree rendering ───────────────────────────────────────────────────────

    _renderBody() {
        this._tbody.replaceChildren();

        if (!this._activeCategory) return;

        const cat = this._categories.find(c => c.name === this._activeCategory);
        if (!cat) return;

        const collapsed = this._collapsed.has(cat.name);

        // Category header row
        const headerRow = document.createElement('tr');
        headerRow.className = 'drc-category-row';

        const visTd = document.createElement('td');
        const catCheckbox = document.createElement('input');
        catCheckbox.type = 'checkbox';
        catCheckbox.checked = cat.violations.every(v => this._visibleIndexes.has(v.index));
        catCheckbox.indeterminate = !catCheckbox.checked && cat.violations.some(v => this._visibleIndexes.has(v.index));
        catCheckbox.addEventListener('change', (e) => {
            const checked = e.target.checked;
            cat.violations.forEach(v => {
                if (checked) this._visibleIndexes.add(v.index);
                else this._visibleIndexes.delete(v.index);
            });
            this._sendVisibleRequest();
            this._renderBody();
        });
        visTd.appendChild(catCheckbox);

        const arrowTd = document.createElement('td');
        arrowTd.className = 'drc-category-arrow';
        arrowTd.colSpan = 1;
        arrowTd.textContent = collapsed ? '▶' : '▼';

        const nameTd = document.createElement('td');
        nameTd.textContent = `${cat.name}  (${cat.count} marker${cat.count !== 1 ? 's' : ''})`;

        headerRow.appendChild(visTd);
        headerRow.appendChild(arrowTd);
        headerRow.appendChild(nameTd);

        arrowTd.addEventListener('click', () => {
            if (collapsed) {
                this._collapsed.delete(cat.name);
            } else {
                this._collapsed.add(cat.name);
            }
            this._renderBody();
        });
        nameTd.addEventListener('click', () => {
            if (collapsed) {
                this._collapsed.delete(cat.name);
            } else {
                this._collapsed.add(cat.name);
            }
            this._renderBody();
        });

        this._tbody.appendChild(headerRow);

        if (!collapsed) {
            cat.violations.forEach((v, localIdx) => {
                const tr = document.createElement('tr');
                tr.className = 'drc-violation-row';
                if (v.index === this._selectedIndex) tr.classList.add('selected');
                if (!v.is_visited) tr.style.fontWeight = 'bold';

                const vVisTd = document.createElement('td');
                const vCheckbox = document.createElement('input');
                vCheckbox.type = 'checkbox';
                vCheckbox.checked = this._visibleIndexes.has(v.index);
                vCheckbox.addEventListener('change', (e) => {
                    if (e.target.checked) this._visibleIndexes.add(v.index);
                    else this._visibleIndexes.delete(v.index);
                    this._sendVisibleRequest();
                    this._renderBody();
                });
                vVisTd.appendChild(vCheckbox);

                const indexTd = document.createElement('td');
                indexTd.className = 'drc-index-cell';
                indexTd.textContent = localIdx + 1;

                const ruleTd = document.createElement('td');
                ruleTd.textContent = v.rule || v.layer || '';

                tr.appendChild(vVisTd);
                tr.appendChild(indexTd);
                tr.appendChild(ruleTd);

                tr.style.cursor = 'pointer';
                indexTd.addEventListener('click', () => this._selectViolation(v));
                ruleTd.addEventListener('click', () => this._selectViolation(v));

                this._tbody.appendChild(tr);
            });
        }
    }

    _sendVisibleRequest() {
        this._app.websocketManager.request({
            type: 'drc_set_visible',
            indexes: Array.from(this._visibleIndexes).map(String)
        }).then(() => {
            this._redrawAllLayers();
            this._update3DHighlights();
        }).catch(err => console.error('drc_set_visible error:', err));
    }

    _update3DHighlights() {
        if (!this._app.threeDViewerWidget) return;
        const visibleViolations = this._getVisibleViolations().filter(
            v => this._visibleIndexes.has(v.index)
        );
        if (visibleViolations.length > 0) {
            this._app.threeDViewerWidget.highlightDRC(visibleViolations);
        } else {
            this._app.threeDViewerWidget.clearHighlightDRC();
        }
    }

    _selectViolation(v) {
        this._selectedIndex = v.index;

        if (!v.is_visited) {
            v.is_visited = true;
            this._app.websocketManager.request({
                type: 'drc_mark_visited',
                index: v.index,
            }).catch(err => console.error('drc_mark_visited error:', err));
        }

        // Auto-enable visibility for selected violation
        if (!this._visibleIndexes.has(v.index)) {
            this._visibleIndexes.add(v.index);
            this._sendVisibleRequest();
        }

        this._renderBody();

        // Scroll selected row into view
        const selectedRow = this._tbody.querySelector('tr.selected');
        if (selectedRow) {
            selectedRow.scrollIntoView({ block: 'nearest' });
        }
        this._tableContainer.focus();

        this._app.websocketManager.request({
            type: 'drc_highlight',
            index: v.index,
        }).then(() => {
            this._redrawAllLayers();
            this._zoomToBbox(v.bbox);

            // If the 3D Viewer isn't open, open it so we can highlight
            if (!this._app.threeDViewerWidget && this._app.focusComponent) {
                this._app.focusComponent('3DViewer');
            }

            this._update3DHighlights();
        }).catch(err => console.error('drc_highlight error:', err));
    }

    _clearHighlight() {
        this._selectedIndex = -1;
        this._app.websocketManager.request({ type: 'drc_highlight', index: -1 })
            .then(() => {
                this._redrawAllLayers();
                this._update3DHighlights();
            })
            .catch(err => console.error('drc_highlight clear error:', err));
    }

    _zoomToBbox(bbox) {
        if (!this._app.map || !this._app.designScale || !bbox) return;
        const [x1, y1, x2, y2] = bbox;
        const { designScale: scale, designMaxDXDY: maxDXDY,
                designOriginX: ox, designOriginY: oy } = this._app;
        const dx = Math.max((x2 - x1) * 0.1, 100);
        const dy = Math.max((y2 - y1) * 0.1, 100);
        const bounds = dbuRectToBounds(
            x1 - dx, y1 - dy, x2 + dx, y2 + dy,
            scale, maxDXDY, ox, oy);
        this._app.map.fitBounds(bounds);
    }

    // ─── Load dialog ──────────────────────────────────────────────────────────

    _showLoadDialog() {
        const overlay = document.createElement('div');
        overlay.className = 'modal-overlay';

        overlay.innerHTML = `
            <div class="modal-dialog file-browser-dialog">
                <h3>Load DRC Report</h3>
                <div class="fb-breadcrumb"></div>
                <div class="fb-file-list"></div>
                <input type="text" class="fb-path-input"
                       placeholder="Server-side file path (.rpt, .drc, .json)">
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
            okBtn.disabled = !pathInput.value.trim();
        };

        const renderBreadcrumb = (dirPath) => {
            breadcrumb.replaceChildren();
            const parts = dirPath.split('/').filter(Boolean);
            const root = document.createElement('span');
            root.className = 'fb-crumb';
            root.textContent = '/';
            root.addEventListener('click', () => navigate('/'));
            breadcrumb.appendChild(root);
            let accum = '';
            for (const part of parts) {
                accum += '/' + part;
                const sep = document.createElement('span');
                sep.className = 'fb-crumb-sep';
                sep.textContent = ' / ';
                breadcrumb.appendChild(sep);
                const crumb = document.createElement('span');
                crumb.className = 'fb-crumb';
                crumb.textContent = part;
                const target = accum;
                crumb.addEventListener('click', () => navigate(target));
                breadcrumb.appendChild(crumb);
            }
        };

        const renderEntries = (entries) => {
            fileList.replaceChildren();
            if (!entries || entries.length === 0) {
                const empty = document.createElement('div');
                empty.className = 'fb-empty';
                empty.textContent = '(empty directory)';
                fileList.appendChild(empty);
                return;
            }
            for (const entry of entries) {
                const row = document.createElement('div');
                row.className = 'fb-entry' + (entry.is_dir ? ' fb-dir' : '');
                const icon = document.createElement('span');
                icon.className = 'fb-icon';
                icon.textContent = entry.is_dir ? '\u{1F4C1}' : '\u{1F4C4}';
                const name = document.createElement('span');
                name.className = 'fb-name';
                name.textContent = entry.name;
                row.appendChild(icon);
                row.appendChild(name);

                if (entry.is_dir) {
                    row.addEventListener('click', () =>
                        navigate(currentPath + '/' + entry.name));
                } else {
                    row.addEventListener('click', () => {
                        fileList.querySelectorAll('.fb-selected')
                            .forEach(el => el.classList.remove('fb-selected'));
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
                fileList.replaceChildren();
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
                    type: 'drc_load',
                    path,
                });
                if (resp.error || resp.type === 'error') {
                    throw new Error(resp.result || 'Load failed');
                }
                close();
                // Refresh the widget with the newly loaded report
                await this.update();
                // Select the newly loaded category
                if (resp.category) {
                    this._activeCategory = resp.category;
                    this._refreshDropdown();
                    this._renderBody();
                }
            } catch (err) {
                errorDiv.textContent = err.message || 'Load failed';
                errorDiv.style.display = '';
                okBtn.disabled = false;
                okBtn.textContent = 'Load';
            }
        };

        cancelBtn.addEventListener('click', close);
        okBtn.addEventListener('click', submit);
        pathInput.addEventListener('input', updateOkState);
        pathInput.addEventListener('keydown', (e) => {
            if (e.key === 'Enter') submit();
        });
        overlay.addEventListener('click', (e) => {
            if (e.target === overlay) close();
        });

        // Start at current directory
        navigate('');
    }
}
