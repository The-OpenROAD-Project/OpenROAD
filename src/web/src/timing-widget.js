// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Timing report widget — path summary + detail tables.

import { makeResizableHeaders } from './ui-utils.js';

export class TimingWidget {
    constructor(app, redrawAllLayers) {
        this._app = app;
        this._redrawAllLayers = redrawAllLayers;

        this._currentTab = 'setup';
        this._setupPaths = [];
        this._holdPaths = [];
        this._selectedPathIndex = -1;
        this._detailTab = 'data';
        this._selectedDetailIndex = -1;
        this._inspectorPopover = null;
        this._inspectorCloseHandlers = null;
        this._pathSort = { key: 'slack', dir: 'asc' };
        this._detailSort = null;

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

        this.element = el;

        this._bindEvents();
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
        if (tab === 'setup') {
            this._setupPaths = paths;
            this._setupTab.classList.add('active');
            this._holdTab.classList.remove('active');
        } else {
            this._holdPaths = paths;
            this._holdTab.classList.add('active');
            this._setupTab.classList.remove('active');
        }
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
                this._app.websocketManager.request({ type: 'timing_report', is_setup: 1, max_paths: 100 }),
                this._app.websocketManager.request({ type: 'timing_report', is_setup: 0, max_paths: 100 }),
            ]);
            this._setupPaths = setupData.paths || [];
            this._holdPaths = holdData.paths || [];
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

    _clearTimingHighlight() {
        this._app.websocketManager.request({ type: 'timing_highlight', path_index: -1 })
            .then(() => this._redrawAllLayers());
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
            is_setup: this._currentTab === 'setup' ? 1 : 0,
        }).then(() => this._redrawAllLayers())
          .catch(err => console.error('timing_highlight error:', err));
    }

    _renderPathTable() {
        const paths = this._currentTab === 'setup' ? this._setupPaths : this._holdPaths;
        this._pathCountLabel.textContent = paths.length + ' paths';

        // Preserve column widths across re-renders.
        const oldHeaders = this._pathTable.querySelectorAll('thead th');
        const savedWidths = Array.from(oldHeaders, th => th.style.width);

        // Sort in place, tracking the selected path by object identity
        // so its row index follows the re-sort.
        const selectedPath = this._selectedPathIndex >= 0
            ? paths[this._selectedPathIndex] : null;
        sortRowsInPlace(paths, this._pathSort);
        if (selectedPath) {
            this._selectedPathIndex = paths.indexOf(selectedPath);
        }

        this._pathTable.innerHTML = '';

        const thead = document.createElement('thead');
        const hr = document.createElement('tr');
        for (const col of TimingWidget.PATH_COLS) {
            const th = makeSortableHeader(col, this._pathSort, () => {
                this._togglePathSort(col.key);
            });
            hr.appendChild(th);
        }
        thead.appendChild(hr);
        this._pathTable.appendChild(thead);

        const tbody = document.createElement('tbody');
        paths.forEach((p, idx) => {
            const tr = document.createElement('tr');
            if (idx === this._selectedPathIndex) tr.classList.add('selected');
            const vals = [
                p.end_clk,
                fmtTime(p.required),
                fmtTime(p.arrival),
                fmtTime(p.slack),
                fmtTime(p.skew),
                fmtTime(p.path_delay),
                p.logic_depth,
                p.fanout,
                p.start_pin,
                p.end_pin,
            ];
            vals.forEach((v, ci) => {
                const td = document.createElement('td');
                td.textContent = v;
                if (ci === 3 && p.slack < 0) td.classList.add('slack-negative');
                tr.appendChild(td);
            });
            tr.style.cursor = 'pointer';
            tr.addEventListener('click', () => this._selectPathRow(idx));
            tbody.appendChild(tr);
        });
        this._pathTable.appendChild(tbody);

        // Restore previous widths if available, otherwise compute fresh.
        if (savedWidths.length > 0 && savedWidths[0]) {
            const newHeaders = this._pathTable.querySelectorAll('thead th');
            this._pathTable.style.tableLayout = 'fixed';
            newHeaders.forEach((th, i) => {
                if (i < savedWidths.length) th.style.width = savedWidths[i];
            });
        } else {
            makeResizableHeaders(this._pathTable);
        }
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
            is_setup: this._currentTab === 'setup' ? 1 : 0,
            pin_name: nodes[idx].pin,
        }).then(() => this._redrawAllLayers());
    }

    _showInspector(n) {
        this._closeInspector();

        const popover = document.createElement('div');
        popover.className = 'timing-inspector-popover';

        const closeBtn = document.createElement('button');
        closeBtn.className = 'close-btn';
        closeBtn.textContent = '×';
        closeBtn.addEventListener('click', () => this._closeInspector());
        popover.appendChild(closeBtn);

        const title = document.createElement('div');
        title.className = 'timing-inspector-title';
        title.textContent = 'Pin Inspector';
        popover.appendChild(title);

        const rows = [
            ['Pin', n.pin],
            ['Direction', n.direction],
            ['Instance', n.instance],
            ['Master', n.master],
            ['Net', n.net],
            ['Fanout', n.fanout],
            ['Time', fmtTime(n.time)],
            ['Delay', fmtTime(n.delay)],
            ['Slew', fmtTime(n.slew)],
            ['Load', fmtTime(n.load)],
        ];
        const table = document.createElement('table');
        for (const [k, v] of rows) {
            if (v === undefined || v === null || v === '') continue;
            const tr = document.createElement('tr');
            const kth = document.createElement('th');
            kth.textContent = k;
            const vtd = document.createElement('td');
            vtd.textContent = String(v);
            tr.appendChild(kth);
            tr.appendChild(vtd);
            table.appendChild(tr);
        }
        popover.appendChild(table);

        this.element.appendChild(popover);
        this._inspectorPopover = popover;

        const onKeyDown = (e) => {
            if (e.key === 'Escape') this._closeInspector();
        };
        const onMouseDown = (e) => {
            if (!popover.contains(e.target)) this._closeInspector();
        };
        document.addEventListener('keydown', onKeyDown);
        // Defer outside-click listener so the dblclick that opened us
        // doesn't instantly close us.
        const timeoutId = setTimeout(() => {
            document.addEventListener('mousedown', onMouseDown, true);
        }, 0);
        this._inspectorCloseHandlers = { onKeyDown, onMouseDown, timeoutId };
    }

    _togglePathSort(key) {
        if (this._pathSort && this._pathSort.key === key) {
            this._pathSort = { key, dir: this._pathSort.dir === 'asc' ? 'desc' : 'asc' };
        } else {
            this._pathSort = { key, dir: 'asc' };
        }
        this._renderPathTable();
        this._renderDetailTable();
    }

    _toggleDetailSort(key) {
        if (this._detailSort && this._detailSort.key === key) {
            this._detailSort = { key, dir: this._detailSort.dir === 'asc' ? 'desc' : 'asc' };
        } else {
            this._detailSort = { key, dir: 'asc' };
        }
        this._renderDetailTable();
    }

    _closeInspector() {
        if (!this._inspectorPopover) return;
        this._inspectorPopover.remove();
        this._inspectorPopover = null;
        if (this._inspectorCloseHandlers) {
            clearTimeout(this._inspectorCloseHandlers.timeoutId);
            document.removeEventListener('keydown', this._inspectorCloseHandlers.onKeyDown);
            document.removeEventListener('mousedown', this._inspectorCloseHandlers.onMouseDown, true);
            this._inspectorCloseHandlers = null;
        }
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
        const nodesSrc = this._detailTab === 'data' ? path.data_nodes : path.capture_nodes;
        const nodes = this._detailSort
            ? sortRowsInPlace([...nodesSrc], this._detailSort)
            : nodesSrc;

        const thead = document.createElement('thead');
        const hr = document.createElement('tr');
        for (const col of TimingWidget.DETAIL_COLS) {
            const th = makeSortableHeader(col, this._detailSort, () => {
                this._toggleDetailSort(col.key);
            });
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
            tr.addEventListener('dblclick', (e) => {
                e.preventDefault();
                this._showInspector(n);
            });
            tbody.appendChild(tr);
        });
        this._detailTable.appendChild(tbody);

        if (savedWidths.length > 0 && savedWidths[0]) {
            const newHeaders = this._detailTable.querySelectorAll('thead th');
            this._detailTable.style.tableLayout = 'fixed';
            newHeaders.forEach((th, i) => {
                if (i < savedWidths.length) th.style.width = savedWidths[i];
            });
        } else {
            makeResizableHeaders(this._detailTable);
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

// Numeric-aware comparison honoring sort direction.
export function compareValues(a, b, dir) {
    if (a === b) return 0;
    const mul = dir === 'desc' ? -1 : 1;
    const na = typeof a === 'number';
    const nb = typeof b === 'number';
    if (na && nb) return (a - b) * mul;
    // Missing values sort last regardless of direction.
    if (a === undefined || a === null || a === '') return 1;
    if (b === undefined || b === null || b === '') return -1;
    return String(a).localeCompare(String(b)) * mul;
}

// Stable sort of `rows` by `sort = {key, dir}`.  Array.prototype.sort
// is stable since ES2019 (Node 12+); we rely on that.
export function sortRowsInPlace(rows, sort) {
    if (!sort) return rows;
    rows.sort((a, b) => compareValues(a[sort.key], b[sort.key], sort.dir));
    return rows;
}

// Build a <th> with label, click-to-sort, optional tooltip, and sort
// indicator.  Clicks on the resize grip are ignored so resizing and
// sorting do not conflict.
export function makeSortableHeader(col, activeSort, onClick) {
    const th = document.createElement('th');
    th.textContent = col.label;
    if (col.tooltip) th.title = col.tooltip;
    if (activeSort && activeSort.key === col.key) {
        const ind = document.createElement('span');
        ind.className = 'sort-indicator';
        ind.textContent = activeSort.dir === 'asc' ? ' ▲' : ' ▼';
        th.appendChild(ind);
    }
    th.style.cursor = 'pointer';
    th.addEventListener('click', (e) => {
        if (e.target && e.target.classList
            && e.target.classList.contains('col-resize-grip')) {
            return;
        }
        onClick();
    });
    return th;
}

// Column descriptors: {label, key, tooltip?}.  `key` points at the row
// object field used for sorting; the Skew / Logic Delay / Logic Depth
// tooltips mirror staGui.cpp:183–196 for parity with the Qt GUI.
TimingWidget.PATH_COLS = [
    { label: 'Clock', key: 'end_clk' },
    { label: 'Required', key: 'required' },
    { label: 'Arrival', key: 'arrival' },
    { label: 'Slack', key: 'slack' },
    { label: 'Skew', key: 'skew',
      tooltip: 'The difference in arrival times between\n'
             + 'source and destination clock pins of a macro/register,\n'
             + 'adjusted for CRPR and subtracting a clock period.\n'
             + 'Setup and hold times account for internal clock delays.' },
    { label: 'Logic Delay', key: 'path_delay',
      tooltip: 'Path delay from instances (excluding buffers and consecutive '
             + 'inverter pairs)' },
    { label: 'Logic Depth', key: 'logic_depth',
      tooltip: 'Path instances (excluding buffers and consecutive inverter '
             + 'pairs)' },
    { label: 'Fanout', key: 'fanout' },
    { label: 'Start', key: 'start_pin' },
    { label: 'End', key: 'end_pin' },
];

TimingWidget.DETAIL_COLS = [
    { label: 'Pin', key: 'pin' },
    { label: 'Fanout', key: 'fanout' },
    { label: 'R/F', key: 'rise' },
    { label: 'Time', key: 'time' },
    { label: 'Delay', key: 'delay' },
    { label: 'Slew', key: 'slew' },
    { label: 'Load', key: 'load' },
];
