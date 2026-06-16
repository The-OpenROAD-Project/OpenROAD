// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Timing report widget — path summary + detail tables.

import { isStaticMode, makeResizableHeaders } from './ui-utils.js';

export class TimingWidget {
    constructor(app, redrawAllLayers, refreshOverlay) {
        this._app = app;
        this._redrawAllLayers = redrawAllLayers;
        this._refreshOverlay = refreshOverlay || redrawAllLayers;

        this._currentTab = 'setup';
        this._setupPaths = [];
        this._holdPaths = [];
        this._selectedPathIndex = -1;
        this._detailTab = 'data';
        this._selectedDetailIndex = -1;
        // Default to sorting by slack, matching the Qt GUI.
        this._sortCol = 'slack';
        this._sortAscending = true;

        this._build();
    }

    _build() {
        const el = document.createElement('div');
        el.className = 'timing-widget';

        // --- Toolbar ---
        const toolbar = document.createElement('div');
        toolbar.className = 'timing-toolbar';

        this._updateBtn = document.createElement('button');
        this._updateBtn.className = 'timing-btn';
        this._updateBtn.textContent = 'Update';
        if (isStaticMode(this._app)) {
            this._updateBtn.style.display = 'none';
        }

        this._pathCountLabel = document.createElement('span');
        this._pathCountLabel.className = 'timing-path-count';

        toolbar.appendChild(this._updateBtn);
        toolbar.appendChild(this._pathCountLabel);
        el.appendChild(toolbar);

        // --- Setup/Hold Tab Bar ---
        const tabBar = document.createElement('div');
        tabBar.className = 'timing-tab-bar';

        this._setupTab = this._makeTab('Setup', true);
        this._holdTab = this._makeTab('Hold', false);
        tabBar.appendChild(this._setupTab);
        tabBar.appendChild(this._holdTab);
        el.appendChild(tabBar);

        // --- Path listing table ---
        this._pathTableContainer = document.createElement('div');
        this._pathTableContainer.className = 'timing-path-table-container';
        this._pathTable = document.createElement('table');
        this._pathTable.className = 'timing-table';
        this._pathTableContainer.appendChild(this._pathTable);
        el.appendChild(this._pathTableContainer);

        // --- Detail Tab Bar ---
        const detailTabBar = document.createElement('div');
        detailTabBar.className = 'timing-tab-bar';
        this._dataTab = this._makeTab('Data Path', true);
        this._captureTab = this._makeTab('Capture Path', false);
        detailTabBar.appendChild(this._dataTab);
        detailTabBar.appendChild(this._captureTab);
        el.appendChild(detailTabBar);

        // --- Detail table ---
        this._detailTableContainer = document.createElement('div');
        this._detailTableContainer.className = 'timing-detail-table-container';
        this._detailTable = document.createElement('table');
        this._detailTable.className = 'timing-table';
        this._detailTableContainer.appendChild(this._detailTable);
        el.appendChild(this._detailTableContainer);

        // Column-header tooltip (custom div, like the charts/clock-tree
        // tooltips, rather than the easy-to-miss native title tooltip).
        this._headerTooltip = document.createElement('div');
        this._headerTooltip.className = 'timing-header-tooltip';
        el.appendChild(this._headerTooltip);

        this.element = el;

        this._bindEvents();

        if (isStaticMode(this._app)) {
            setTimeout(() => this.update(), 0);
        }
    }

    _makeTab(label, active) {
        const btn = document.createElement('button');
        btn.className = 'timing-tab' + (active ? ' active' : '');
        btn.textContent = label;
        return btn;
    }

    _bindEvents() {
        // Tab switching
        this._setupTab.addEventListener('click', () => {
            this._currentTab = 'setup';
            this._setupTab.classList.add('active');
            this._holdTab.classList.remove('active');
            this._selectedPathIndex = -1;
            this._renderPathTable();
            this._renderDetailTable();
            this._clearTimingHighlight();
        });
        this._holdTab.addEventListener('click', () => {
            this._currentTab = 'hold';
            this._holdTab.classList.add('active');
            this._setupTab.classList.remove('active');
            this._selectedPathIndex = -1;
            this._renderPathTable();
            this._renderDetailTable();
            this._clearTimingHighlight();
        });
        this._dataTab.addEventListener('click', () => {
            this._detailTab = 'data';
            this._dataTab.classList.add('active');
            this._captureTab.classList.remove('active');
            this._renderDetailTable();
        });
        this._captureTab.addEventListener('click', () => {
            this._detailTab = 'capture';
            this._captureTab.classList.add('active');
            this._dataTab.classList.remove('active');
            this._renderDetailTable();
        });

        // Fetch paths
        this._updateBtn.addEventListener('click', () => this.update());

        // Keyboard navigation — path table
        this._pathTableContainer.setAttribute('tabindex', '0');
        this._pathTableContainer.style.outline = 'none';
        this._pathTableContainer.addEventListener('keydown', (e) => {
            if (e.key === 'ArrowDown') {
                e.preventDefault();
                const rows = this._pathTable.querySelectorAll('tbody tr');
                if (this._selectedPathIndex < rows.length - 1) {
                    this._selectPathRow(this._selectedPathIndex + 1);
                }
            } else if (e.key === 'ArrowUp') {
                e.preventDefault();
                if (this._selectedPathIndex > 0) {
                    this._selectPathRow(this._selectedPathIndex - 1);
                }
            }
        });

        // Keyboard navigation — detail table
        this._detailTableContainer.setAttribute('tabindex', '0');
        this._detailTableContainer.style.outline = 'none';
        this._detailTableContainer.addEventListener('keydown', (e) => {
            if (e.key === 'ArrowDown') {
                e.preventDefault();
                const rows = this._detailTable.querySelectorAll('tbody tr');
                if (this._selectedDetailIndex < rows.length - 1) {
                    this._selectDetailRow(this._selectedDetailIndex + 1);
                }
            } else if (e.key === 'ArrowUp') {
                e.preventDefault();
                if (this._selectedDetailIndex > 0) {
                    this._selectDetailRow(this._selectedDetailIndex - 1);
                }
            }
        });
    }

    showPaths(tab, paths) {
        this._currentTab = tab;
        // Copy so sorting never reorders the caller's array.
        if (tab === 'setup') {
            this._setupPaths = [...paths];
            this._setupTab.classList.add('active');
            this._holdTab.classList.remove('active');
        } else {
            this._holdPaths = [...paths];
            this._holdTab.classList.add('active');
            this._setupTab.classList.remove('active');
        }
        this._applySort();
        this._selectedPathIndex = -1;
        this._pathCountLabel.textContent = paths.length + ' paths';
        this._renderPathTable();
        this._renderDetailTable();
        this._clearTimingHighlight();
    }

    async update() {
        this._updateBtn.disabled = true;
        this._updateBtn.textContent = 'Loading...';
        try {
            const [setupData, holdData] = await Promise.all([
                this._app.websocketManager.request({ type: 'timing_report', is_setup: true, max_paths: 100 }),
                this._app.websocketManager.request({ type: 'timing_report', is_setup: false, max_paths: 100 }),
            ]);
            // Copy the arrays: in static mode the response is a shared
            // cached object whose path order must keep matching the
            // server-side overlay indices, so it must not be sorted.
            this._setupPaths = [...(setupData.paths || [])];
            this._holdPaths = [...(holdData.paths || [])];
            this._applySort();
            this._selectedPathIndex = -1;
            this._renderPathTable();
            this._renderDetailTable();
            this._clearTimingHighlight();
        } catch (e) {
            console.error('Timing fetch failed:', e);
        }
        this._updateBtn.disabled = false;
        this._updateBtn.textContent = 'Update';
    }

    // Sort both path lists by the current sort column/order. Paths are
    // tagged with their server-side index first so timing_highlight
    // requests keep mapping to the right path after reordering.
    _applySort() {
        for (const paths of [this._setupPaths, this._holdPaths]) {
            paths.forEach((p, i) => {
                if (p._originalIndex === undefined) p._originalIndex = i;
            });
            const dir = this._sortAscending ? 1 : -1;
            const key = this._sortCol;
            paths.sort((a, b) => {
                const va = a[key];
                const vb = b[key];
                const cmp = (typeof va === 'number' && typeof vb === 'number') ?
                    va - vb :
                    String(va).localeCompare(String(vb));
                return dir * cmp;
            });
        }
    }

    _showHeaderTooltip(e, text) {
        if (e.buttons !== 0) {
            // Mid-drag (e.g. a column resize); don't show a tooltip.
            this._hideHeaderTooltip();
            return;
        }
        const tip = this._headerTooltip;
        tip.textContent = text;
        tip.style.display = 'block';
        // position: fixed — viewport coordinates, offset from the cursor
        // and clamped so the tooltip stays inside the viewport.
        const left = Math.min(e.clientX + 12,
                              window.innerWidth - tip.offsetWidth - 8);
        tip.style.left = Math.max(0, left) + 'px';
        tip.style.top = (e.clientY + 14) + 'px';
    }

    _hideHeaderTooltip() {
        this._headerTooltip.style.display = 'none';
    }

    _sortBy(key) {
        if (this._sortCol === key) {
            this._sortAscending = !this._sortAscending;
        } else {
            this._sortCol = key;
            this._sortAscending = true;
        }
        this._applySort();
        this._selectedPathIndex = -1;
        this._renderPathTable();
        this._renderDetailTable();
        this._clearTimingHighlight();
    }

    _clearTimingHighlight() {
        this._app.websocketManager.request({ type: 'timing_highlight', path_index: -1 })
            .then(() => this._refreshOverlay());
    }

    _selectPathRow(idx) {
        const rows = this._pathTable.querySelectorAll('tbody tr');
        if (idx < 0 || idx >= rows.length) return;
        this._selectedPathIndex = idx;
        for (const row of rows) row.classList.remove('selected');
        rows[idx].classList.add('selected');
        rows[idx].scrollIntoView({ block: 'nearest' });
        this._pathTableContainer.focus();
        this._renderDetailTable();
        // Use _originalIndex when paths were filtered (e.g. by histogram
        // column click in static mode) so the overlay lookup matches.
        const paths = this._currentTab === 'setup' ? this._setupPaths : this._holdPaths;
        const highlightIdx = paths[idx]?._originalIndex ?? idx;
        this._app.websocketManager.request({
            type: 'timing_highlight',
            path_index: highlightIdx,
            is_setup: this._currentTab === 'setup',
        }).then(() => this._refreshOverlay())
          .catch(err => console.error('timing_highlight error:', err));
    }

    _renderPathTable() {
        // The hovered header may be replaced by the re-render, in which
        // case its mouseleave never fires; hide the tooltip explicitly.
        this._hideHeaderTooltip();

        const paths = this._currentTab === 'setup' ? this._setupPaths : this._holdPaths;
        this._pathCountLabel.textContent = paths.length + ' paths';

        // Preserve column widths across re-renders.
        const oldHeaders = this._pathTable.querySelectorAll('thead th');
        const savedWidths = Array.from(oldHeaders, th => th.style.width);

        this._pathTable.innerHTML = '';

        const thead = document.createElement('thead');
        const hr = document.createElement('tr');
        for (const col of TimingWidget.PATH_COLS) {
            const th = document.createElement('th');
            th.classList.add('sortable');
            th.textContent = col.label +
                (col.key === this._sortCol ? (this._sortAscending ? ' ▲' : ' ▼') : '');
            if (col.tooltip) {
                th.addEventListener('mousemove',
                    (e) => this._showHeaderTooltip(e, col.tooltip));
                th.addEventListener('mouseleave',
                    () => this._hideHeaderTooltip());
            }
            th.addEventListener('click', () => this._sortBy(col.key));
            hr.appendChild(th);
        }
        thead.appendChild(hr);
        this._pathTable.appendChild(thead);

        const tbody = document.createElement('tbody');
        paths.forEach((p, idx) => {
            const tr = document.createElement('tr');
            if (idx === this._selectedPathIndex) tr.classList.add('selected');
            for (const col of TimingWidget.PATH_COLS) {
                const td = document.createElement('td');
                const v = p[col.key];
                td.textContent = col.time ? fmtTime(v) : v;
                if (col.key === 'slack' && p.slack < 0) td.classList.add('slack-negative');
                tr.appendChild(td);
            }
            tr.style.cursor = 'pointer';
            tr.addEventListener('click', () => this._selectPathRow(idx));
            tbody.appendChild(tr);
        });

        if (paths.length === 0) {
            const tr = document.createElement('tr');
            const td = document.createElement('td');
            td.colSpan = TimingWidget.PATH_COLS.length;
            td.style.textAlign = 'center';
            td.style.color = 'var(--fg-secondary)';
            td.textContent = isStaticMode(this._app) ?
                'No timing data available' :
                'Click "Update" to load timing paths';
            tr.appendChild(td);
            tbody.appendChild(tr);
        }

        this._pathTable.appendChild(tbody);

        // Install resize grips, reusing any previously saved widths to
        // avoid a measuring reflow.
        makeResizableHeaders(this._pathTable, savedWidths);
    }

    _selectDetailRow(idx) {
        const rows = this._detailTable.querySelectorAll('tbody tr');
        if (idx < 0 || idx >= rows.length) return;
        this._selectedDetailIndex = idx;
        for (const row of rows) {
            row.classList.remove('timing-selected-row');
        }
        rows[idx].classList.add('timing-selected-row');
        rows[idx].scrollIntoView({ block: 'nearest' });
        this._detailTableContainer.focus();

        const paths = this._currentTab === 'setup' ? this._setupPaths : this._holdPaths;
        const path = paths[this._selectedPathIndex];
        const nodes = this._detailTab === 'data' ? path.data_nodes : path.capture_nodes;
        // Use _originalIndex when paths were filtered (e.g. by histogram
        // column click in static mode) so the overlay lookup matches.
        const highlightIdx = path._originalIndex ?? this._selectedPathIndex;
        this._app.websocketManager.request({
            type: 'timing_highlight',
            path_index: highlightIdx,
            is_setup: this._currentTab === 'setup',
            pin_name: nodes[idx].pin,
        }).then(() => this._refreshOverlay());
    }

    _renderDetailTable() {
        // Preserve column widths across re-renders.
        const oldHeaders = this._detailTable.querySelectorAll('thead th');
        const savedWidths = Array.from(oldHeaders, th => th.style.width);

        this._detailTable.innerHTML = '';
        this._selectedDetailIndex = -1;
        const paths = this._currentTab === 'setup' ? this._setupPaths : this._holdPaths;
        if (this._selectedPathIndex < 0 || this._selectedPathIndex >= paths.length) return;

        const path = paths[this._selectedPathIndex];
        const nodes = this._detailTab === 'data' ? path.data_nodes : path.capture_nodes;

        const thead = document.createElement('thead');
        const hr = document.createElement('tr');
        for (const col of TimingWidget.DETAIL_COLS) {
            const th = document.createElement('th');
            th.textContent = col;
            hr.appendChild(th);
        }
        thead.appendChild(hr);
        this._detailTable.appendChild(thead);

        const tbody = document.createElement('tbody');
        nodes.forEach((n, idx) => {
            const tr = document.createElement('tr');
            if (n.clk) tr.classList.add('timing-clock-row');
            const vals = [
                n.pin,
                n.fanout,
                n.rise ? '↑' : '↓',
                fmtTime(n.time),
                fmtTime(n.delay),
                fmtTime(n.slew),
                fmtTime(n.load),
            ];
            for (const v of vals) {
                const td = document.createElement('td');
                td.textContent = v;
                tr.appendChild(td);
            }
            tr.style.cursor = 'pointer';
            tr.addEventListener('click', () => this._selectDetailRow(idx));
            tbody.appendChild(tr);
        });
        this._detailTable.appendChild(tbody);

        // Install resize grips, reusing any previously saved widths to
        // avoid a measuring reflow.
        makeResizableHeaders(this._detailTable, savedWidths);
        if (!savedWidths[0]) {
            // Pin column: set initial width to 30 characters
            const pinTh = this._detailTable.querySelector('thead th');
            if (pinTh) {
                pinTh.style.width = '30ch';
            }
        }
    }

}

export function fmtTime(v) {
    if (v === undefined || v === null) return '';
    return typeof v === 'number' ? v.toFixed(4) : String(v);
}

// Path table columns: label, path field to display/sort by, time formatting,
// and header tooltip (tooltips match the Qt GUI's TimingPathsModel).
TimingWidget.PATH_COLS = [
    { label: 'Clock', key: 'end_clk' },
    { label: 'Required', key: 'required', time: true },
    { label: 'Arrival', key: 'arrival', time: true },
    { label: 'Slack', key: 'slack', time: true },
    { label: 'Skew', key: 'skew', time: true,
      tooltip: 'The difference in arrival times between\n' +
               'source and destination clock pins of a macro/register,\n' +
               'adjusted for CRPR and subtracting a clock period.\n' +
               'Setup and hold times account for internal clock delays.' },
    { label: 'Logic Delay', key: 'path_delay', time: true,
      tooltip: 'Path delay from instances (excluding buffers and consecutive '
               + 'inverter pairs)' },
    { label: 'Logic Depth', key: 'logic_depth',
      tooltip: 'Path instances (excluding buffers and consecutive inverter '
               + 'pairs)' },
    { label: 'Fanout', key: 'fanout' },
    { label: 'Start', key: 'start_pin' },
    { label: 'End', key: 'end_pin' },
];
TimingWidget.DETAIL_COLS = ['Pin', 'Fanout', 'R/F', 'Time', 'Delay', 'Slew', 'Load'];
