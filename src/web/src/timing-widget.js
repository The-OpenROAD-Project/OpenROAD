// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Timing report widget — path summary + detail tables.

import { makeResizableHeaders } from './ui-utils.js';

export class TimingWidget {
    constructor(container, app, redrawAllLayers) {
        this._app = app;
        this._redrawAllLayers = redrawAllLayers;

        this._currentTab = 'setup';
        this._setupPaths = [];
        this._holdPaths = [];
        this._selectedPathIndex = -1;
        this._detailTab = 'data';
        this._selectedDetailIndex = -1;

        this._build(container);
    }

    _build(container) {
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

        container.element.appendChild(el);

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
        this._app.websocketManager.request({
            type: 'timing_highlight',
            path_index: idx,
            is_setup: this._currentTab === 'setup' ? 1 : 0,
        }).then(() => this._redrawAllLayers())
          .catch(err => console.error('timing_highlight error:', err));
    }

    _renderPathTable() {
        const paths = this._currentTab === 'setup' ? this._setupPaths : this._holdPaths;
        this._pathCountLabel.textContent = paths.length + ' paths';
        this._pathTable.innerHTML = '';

        const thead = document.createElement('thead');
        const hr = document.createElement('tr');
        for (const col of TimingWidget.PATH_COLS) {
            const th = document.createElement('th');
            th.textContent = col;
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
        makeResizableHeaders(this._pathTable);
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
        this._app.websocketManager.request({
            type: 'timing_highlight',
            path_index: this._selectedPathIndex,
            is_setup: this._currentTab === 'setup' ? 1 : 0,
            pin_name: nodes[idx].pin,
        }).then(() => this._redrawAllLayers());
    }

    _renderDetailTable() {
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
        makeResizableHeaders(this._detailTable);
        // Pin column: set initial width to 30 characters
        const pinTh = this._detailTable.querySelector('thead th');
        if (pinTh) {
            pinTh.style.width = '30ch';
        }
    }

}

export function fmtTime(v) {
    if (v === undefined || v === null) return '';
    return typeof v === 'number' ? v.toFixed(4) : String(v);
}

TimingWidget.PATH_COLS = ['Clock', 'Required', 'Arrival', 'Slack', 'Skew',
                          'Logic Delay', 'Logic Depth', 'Fanout', 'Start', 'End'];
TimingWidget.DETAIL_COLS = ['Pin', 'Fanout', 'R/F', 'Time', 'Delay', 'Slew', 'Load'];
