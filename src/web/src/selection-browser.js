// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Selection browser — tables of all selected and highlighted objects
// (Qt GUI parity: SelectHighlightWindow's Selected/Highlighted tabs).
// Rows carry the stable index from the last fetch, so sorting/filtering
// never acts on the wrong object (the Qt dock's context menus use proxy
// rows without mapToSource and mis-target under sorting).

import { dbuRectToBounds } from './coordinates.js';
import { isStaticMode, makeResizableHeaders } from './ui-utils.js';

export class SelectionBrowser {
    constructor(container, app, refreshOverlay) {
        this._app = app;
        this._refreshOverlay = refreshOverlay || (() => {});
        this._data = { selection: [], groups: [], truncated: false };
        this._tab = 'selected';
        this._filter = '';
        this._sortKey = null;   // 'name' | 'type' | null (fetch order)
        this._sortAsc = true;
        this._pendingRefresh = null;
        this._inFlight = false;
        this._visible = true;

        this._el = document.createElement('div');
        this._el.className = 'selection-browser';
        container.element.appendChild(this._el);

        // GoldenLayout tab visibility — skip background refreshes.
        container.on?.('show', () => {
            this._visible = true;
            this.refresh();
        });
        container.on?.('hide', () => {
            this._visible = false;
        });

        this._render();
        this.refresh();
    }

    // Debounced refresh, hooked from main.js's overlay scheduler: every
    // selection/highlight mutation already refreshes the overlay, so one
    // hook keeps the panel in sync (mirrors the Qt dock's 100 ms
    // coalescing timers) without instrumenting each call site.
    scheduleRefresh() {
        if (!this._visible || this._pendingRefresh !== null) return;
        this._pendingRefresh = setTimeout(() => {
            this._pendingRefresh = null;
            this.refresh();
        }, 150);
    }

    refresh() {
        if (isStaticMode(this._app) || this._inFlight) return;
        this._inFlight = true;
        this._app.websocketManager.request({ type: 'list_selection' })
            .then((data) => {
                this._inFlight = false;
                if (!data.ok) return;
                this._data = data;
                this._render();
            })
            .catch(() => { this._inFlight = false; });
    }

    // ── data shaping ────────────────────────────────────────────────

    // Flatten the active tab into entries carrying their stable server
    // index (and group for the Highlighted tab).
    _entries() {
        let entries;
        if (this._tab === 'selected') {
            entries = (this._data.selection || [])
                .map((row, index) => ({ row, index, group: -1 }));
        } else {
            entries = (this._data.groups || []).flatMap((members, group) =>
                members.map((row, index) => ({ row, index, group })));
        }
        const filter = this._filter.trim().toLowerCase();
        if (filter) {
            entries = entries.filter(({ row }) =>
                (row.name || '').toLowerCase().includes(filter)
                || (row.type || '').toLowerCase().includes(filter));
        }
        if (this._sortKey) {
            const key = this._sortKey;
            const dir = this._sortAsc ? 1 : -1;
            entries = entries.slice().sort((a, b) =>
                dir * String(a.row[key] || '').localeCompare(
                    String(b.row[key] || '')));
        }
        return entries;
    }

    _fmtCoord(v) {
        if (this._app.showDbu) return String(v);
        const dbu = this._app.getDbuPerMicron
            ? this._app.getDbuPerMicron() : 1000;
        return (v / dbu).toFixed(3);
    }

    _fmtBounds(bbox) {
        if (!bbox) return '<none>';
        const [x1, y1, x2, y2] = bbox;
        return `(${this._fmtCoord(x1)}, ${this._fmtCoord(y1)}), `
            + `(${this._fmtCoord(x2)}, ${this._fmtCoord(y2)})`;
    }

    // ── interactions ────────────────────────────────────────────────

    _inspectRow(entry, focusInspector) {
        const msg = entry.group < 0
            ? { type: 'inspect_selection', index: entry.index,
                use_dbu: this._app.showDbu }
            : { type: 'inspect_group', group: entry.group,
                index: entry.index, use_dbu: this._app.showDbu };
        this._app.websocketManager.request(msg)
            .then((data) => {
                if (!data.ok) { this.refresh(); return; }
                if (this._app.updateInspector) this._app.updateInspector(data);
                if (focusInspector && this._app.focusComponent) {
                    this._app.focusComponent('Inspector');
                }
                this._refreshOverlay();
            })
            .catch(() => {});
    }

    _deselectRow(entry) {
        this._app.websocketManager.request(
            { type: 'deselect', index: entry.index })
            .then(() => {
                this.refresh();
                this._refreshOverlay();
            })
            .catch(() => {});
    }

    _clearHighlights() {
        this._app.websocketManager.request(
            { type: 'clear_highlights', group: -1 })
            .then(() => {
                this.refresh();
                this._refreshOverlay();
                if (this._app.refreshInspector) this._app.refreshInspector();
            })
            .catch(() => {});
    }

    _zoomToBBox(bbox) {
        if (!bbox || !this._app.designScale || !this._app.map) return;
        const [xMin, yMin, xMax, yMax] = bbox;
        // Same margin rule as the DRC widget / Qt zoom-to handlers.
        const dbuPerMicron = this._app.getDbuPerMicron
            ? this._app.getDbuPerMicron() : 1000;
        const margin = Math.min(10 * dbuPerMicron,
                                2 * Math.max(xMax - xMin, yMax - yMin));
        this._app.map.fitBounds(dbuRectToBounds(
            xMin - margin, yMin - margin, xMax + margin, yMax + margin,
            this._app.designScale, this._app.designMaxDXDY,
            this._app.designOriginX, this._app.designOriginY));
    }

    // ── rendering ───────────────────────────────────────────────────

    _render() {
        // Rebuilding the table detaches the hovered <tr> before its
        // mouseleave can fire, which would otherwise strand a continuous
        // hover animation; stop it up front.
        if (this._app.stopSelectionAnimation) {
            this._app.stopSelectionAnimation();
        }
        this._el.innerHTML = '';

        const selCount = (this._data.selection || []).length;
        const hlCount = (this._data.groups || [])
            .reduce((n, g) => n + g.length, 0);

        // Tabs with live counters (the Qt dock has none).
        const tabs = document.createElement('div');
        tabs.className = 'sb-tabs';
        for (const [key, label, count] of [
            ['selected', 'Selected', selCount],
            ['highlighted', 'Highlighted', hlCount]]) {
            const tab = document.createElement('button');
            tab.className = 'sb-tab' + (this._tab === key ? ' active' : '');
            tab.textContent = `${label} (${count})`;
            tab.addEventListener('click', () => {
                this._tab = key;
                this._render();
            });
            tabs.appendChild(tab);
        }
        const refreshBtn = document.createElement('button');
        refreshBtn.className = 'sb-tool';
        refreshBtn.title = 'Refresh';
        refreshBtn.textContent = '⟳';
        refreshBtn.addEventListener('click', () => this.refresh());
        tabs.appendChild(refreshBtn);
        if (this._tab === 'highlighted' && hlCount > 0) {
            const clearBtn = document.createElement('button');
            clearBtn.className = 'sb-tool';
            clearBtn.title = 'Clear all highlights';
            clearBtn.textContent = 'Clear';
            clearBtn.addEventListener('click', () => this._clearHighlights());
            tabs.appendChild(clearBtn);
        }
        this._el.appendChild(tabs);

        // Real substring filter over name/type (the Qt dock only has
        // keyboardSearch — and reads the wrong box on its second tab).
        const filter = document.createElement('input');
        filter.type = 'text';
        filter.className = 'sb-filter';
        filter.placeholder = 'Filter by name or type…';
        filter.value = this._filter;
        filter.addEventListener('input', () => {
            this._filter = filter.value;
            this._renderTable();
        });
        this._el.appendChild(filter);

        if (this._data.truncated) {
            const note = document.createElement('div');
            note.className = 'sb-truncated';
            note.textContent
                = 'List truncated — showing the first 1000 entries per set.';
            this._el.appendChild(note);
        }

        this._tableWrap = document.createElement('div');
        this._tableWrap.className = 'sb-table-wrap';
        this._el.appendChild(this._tableWrap);
        this._renderTable();
    }

    _renderTable() {
        this._tableWrap.innerHTML = '';
        const entries = this._entries();
        if (!entries.length) {
            const empty = document.createElement('div');
            empty.className = 'sb-empty';
            empty.textContent = this._tab === 'selected'
                ? 'Nothing selected.' : 'Nothing highlighted.';
            this._tableWrap.appendChild(empty);
            return;
        }

        const table = document.createElement('table');
        table.className = 'sb-table';
        const thead = document.createElement('thead');
        const headRow = document.createElement('tr');
        const columns = [['name', 'Object'], ['type', 'Type'],
                         [null, 'Bounds']];
        if (this._tab === 'highlighted') columns.push([null, 'Group']);
        columns.push([null, '']);  // per-row actions
        for (const [key, label] of columns) {
            const th = document.createElement('th');
            th.textContent = label;
            if (key) {
                th.classList.add('sb-sortable');
                if (this._sortKey === key) {
                    th.textContent += this._sortAsc ? ' ▲' : ' ▼';
                }
                th.addEventListener('click', () => {
                    if (this._sortKey === key) {
                        this._sortAsc = !this._sortAsc;
                    } else {
                        this._sortKey = key;
                        this._sortAsc = true;
                    }
                    this._renderTable();
                });
            }
            headRow.appendChild(th);
        }
        thead.appendChild(headRow);
        table.appendChild(thead);

        const tbody = document.createElement('tbody');
        const palette = this._app.techData?.highlight_colors || [];
        for (const entry of entries) {
            const tr = document.createElement('tr');
            tr.className = 'sb-row';

            const nameTd = document.createElement('td');
            nameTd.textContent = entry.row.name || '';
            const typeTd = document.createElement('td');
            typeTd.textContent = entry.row.type || '';
            const boundsTd = document.createElement('td');
            boundsTd.textContent = this._fmtBounds(entry.row.bbox);
            tr.appendChild(nameTd);
            tr.appendChild(typeTd);
            tr.appendChild(boundsTd);

            if (this._tab === 'highlighted') {
                const groupTd = document.createElement('td');
                const chip = document.createElement('span');
                chip.className = 'sb-chip';
                const c = palette[entry.group];
                if (c) {
                    chip.style.backgroundColor
                        = `rgb(${c[0]}, ${c[1]}, ${c[2]})`;
                }
                groupTd.appendChild(chip);
                groupTd.appendChild(document.createTextNode(
                    ` Group ${entry.group + 1}`));
                tr.appendChild(groupTd);
            }

            const actionsTd = document.createElement('td');
            actionsTd.className = 'sb-actions';
            if (entry.row.bbox) {
                const zoomBtn = document.createElement('button');
                zoomBtn.className = 'sb-row-btn';
                zoomBtn.title = 'Zoom to';
                zoomBtn.textContent = '⌖';
                zoomBtn.addEventListener('click', (e) => {
                    e.stopPropagation();
                    this._zoomToBBox(entry.row.bbox);
                });
                actionsTd.appendChild(zoomBtn);
            }
            if (this._tab === 'selected') {
                const deselBtn = document.createElement('button');
                deselBtn.className = 'sb-row-btn';
                deselBtn.title = 'De-select';
                deselBtn.textContent = '✕';
                deselBtn.addEventListener('click', (e) => {
                    e.stopPropagation();
                    this._deselectRow(entry);
                });
                actionsTd.appendChild(deselBtn);
            }
            tr.appendChild(actionsTd);

            tr.addEventListener('click', () => this._inspectRow(entry, false));
            tr.addEventListener('dblclick',
                                () => this._inspectRow(entry, true));
            // Web equivalent of Qt's selectionFocus: hovering a row
            // animates the object in the layout until the pointer leaves.
            if (entry.row.bbox) {
                tr.addEventListener('mouseenter', () => {
                    if (this._app.animateSelection) {
                        this._app.animateSelection(entry.row.bbox,
                                                   { repeats: 0 });
                    }
                });
                tr.addEventListener('mouseleave', () => {
                    if (this._app.stopSelectionAnimation) {
                        this._app.stopSelectionAnimation();
                    }
                });
            }
            tbody.appendChild(tr);
        }
        table.appendChild(tbody);
        this._tableWrap.appendChild(table);
        makeResizableHeaders(table);
    }
}
