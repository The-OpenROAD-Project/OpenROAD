// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Canvas-based slack histogram widget.

import { getThemeColors } from './theme.js';

// Layout margins (pixels)
export const kLeftMargin = 60;
export const kRightMargin = 20;
export const kTopMargin = 30;
export const kBottomMargin = 50;
export const kBarGap = 2;

// Colors matching the Qt GUI (chartsWidget.cpp:798-803)
const kNegativeFill = '#f08080';    // lightcoral
const kNegativeBorder = '#8b0000';  // darkred
const kPositiveFill = '#90ee90';    // lightgreen
const kPositiveBorder = '#006400';  // darkgreen

// Pure layout computation — extracted for testability.
export function computeHistogramLayout(histogramData, canvasWidth, canvasHeight) {
    const bins = histogramData?.bins;
    if (!bins || bins.length === 0) {
        return { bars: [], yMax: 0, yTicks: [], chartArea: null };
    }

    const chartLeft = kLeftMargin;
    const chartRight = canvasWidth - kRightMargin;
    const chartTop = kTopMargin;
    const chartBottom = canvasHeight - kBottomMargin;
    const chartWidth = chartRight - chartLeft;
    const chartHeight = chartBottom - chartTop;

    if (chartWidth <= 0 || chartHeight <= 0) {
        return { bars: [], yMax: 0, yTicks: [], chartArea: null };
    }

    const chartArea = { left: chartLeft, right: chartRight,
                        top: chartTop, bottom: chartBottom };

    // Find max count for Y scale
    let maxCount = 0;
    for (const bin of bins) {
        if (bin.count > maxCount) maxCount = bin.count;
    }
    if (maxCount === 0) maxCount = 1;

    // Compute nice Y-axis max and ticks
    const { yMax, yTicks } = computeYAxis(maxCount);

    // Compute bar rectangles
    const barWidth = (chartWidth - kBarGap * (bins.length - 1)) / bins.length;
    const bars = [];
    for (let i = 0; i < bins.length; i++) {
        const bin = bins[i];
        const barHeight = (bin.count / yMax) * chartHeight;
        bars.push({
            x: chartLeft + i * (barWidth + kBarGap),
            y: chartBottom - barHeight,
            width: barWidth,
            height: barHeight,
            count: bin.count,
            lower: bin.lower,
            upper: bin.upper,
            negative: bin.negative,
        });
    }

    return { bars, yMax, yTicks, chartArea };
}

// Compute nice Y-axis max and tick values.
function computeYAxis(maxCount) {
    if (maxCount <= 10) {
        const ticks = [];
        for (let i = 0; i <= maxCount; i++) ticks.push(i);
        return { yMax: maxCount, yTicks: ticks };
    }

    // Snap to a nice ceiling
    const digits = Math.floor(Math.log10(maxCount)) + 1;
    const firstDigit = Math.floor(maxCount / Math.pow(10, digits - 1));
    const snapMax = (firstDigit + 1) * Math.pow(10, digits - 1);

    let total = Math.pow(10, digits);
    if (firstDigit < 5) total /= 2;
    const interval = Math.ceil(total / 10);

    const ticks = [];
    for (let v = 0; v <= snapMax; v += interval) {
        ticks.push(v);
    }

    return { yMax: snapMax, yTicks: ticks };
}

export class ChartsWidget {
    constructor(container, app, redrawAllLayers) {
        this._app = app;
        this._redrawAllLayers = redrawAllLayers;
        this._currentTab = 'setup';
        this._histogramData = null;
        this._bars = [];
        this._yMax = 0;
        this._yTicks = [];
        this._chartArea = null;
        this._hoveredBar = null;

        this._build(container);
    }

    // ---- DOM construction ----

    _build(container) {
        const el = document.createElement('div');
        el.className = 'charts-widget';

        // Toolbar
        const toolbar = document.createElement('div');
        toolbar.className = 'charts-toolbar';

        this._updateBtn = document.createElement('button');
        this._updateBtn.className = 'timing-btn';
        this._updateBtn.textContent = 'Update';

        this._statusLabel = document.createElement('span');
        this._statusLabel.className = 'timing-path-count';

        toolbar.appendChild(this._updateBtn);
        toolbar.appendChild(this._statusLabel);
        el.appendChild(toolbar);

        // Tab bar (Setup / Hold)
        const tabBar = document.createElement('div');
        tabBar.className = 'timing-tab-bar';

        this._setupTab = document.createElement('div');
        this._setupTab.className = 'timing-tab active';
        this._setupTab.textContent = 'Setup Slack';

        this._holdTab = document.createElement('div');
        this._holdTab.className = 'timing-tab';
        this._holdTab.textContent = 'Hold Slack';

        tabBar.appendChild(this._setupTab);
        tabBar.appendChild(this._holdTab);
        el.appendChild(tabBar);

        // Filter row
        const filterRow = document.createElement('div');
        filterRow.className = 'charts-toolbar';

        const pgLabel = document.createElement('span');
        pgLabel.textContent = 'Path Group:';
        pgLabel.style.color = 'var(--fg-muted)';
        pgLabel.style.fontSize = '12px';
        this._pathGroupSelect = document.createElement('select');
        this._pathGroupSelect.className = 'charts-select';
        this._pathGroupSelect.innerHTML = '<option value="">All</option>';

        const clkLabel = document.createElement('span');
        clkLabel.textContent = 'Clock:';
        clkLabel.style.color = 'var(--fg-muted)';
        clkLabel.style.fontSize = '12px';
        this._clockSelect = document.createElement('select');
        this._clockSelect.className = 'charts-select';
        this._clockSelect.innerHTML = '<option value="">All</option>';

        filterRow.appendChild(pgLabel);
        filterRow.appendChild(this._pathGroupSelect);
        filterRow.appendChild(clkLabel);
        filterRow.appendChild(this._clockSelect);
        el.appendChild(filterRow);

        // Canvas
        this._canvas = document.createElement('canvas');
        this._canvas.className = 'charts-canvas';
        el.appendChild(this._canvas);

        // Tooltip
        this._tooltip = document.createElement('div');
        this._tooltip.className = 'charts-tooltip';
        this._tooltip.style.display = 'none';
        el.appendChild(this._tooltip);

        container.element.appendChild(el);
        this._el = el;

        this._ctx = this._canvas.getContext('2d');
        this._bindEvents();
    }

    _bindEvents() {
        this._updateBtn.addEventListener('click', () => this.update());

        this._setupTab.addEventListener('click', () => {
            if (this._currentTab === 'setup') return;
            this._currentTab = 'setup';
            this._setupTab.classList.add('active');
            this._holdTab.classList.remove('active');
            this.update();
        });

        this._holdTab.addEventListener('click', () => {
            if (this._currentTab === 'hold') return;
            this._currentTab = 'hold';
            this._holdTab.classList.add('active');
            this._setupTab.classList.remove('active');
            this.update();
        });

        this._pathGroupSelect.addEventListener('change', () => this._fetchHistogram());
        this._clockSelect.addEventListener('change', () => this._fetchHistogram());

        this._canvas.addEventListener('mousemove', (e) => this._handleHover(e));
        this._canvas.addEventListener('mouseleave', () => {
            this._hoveredBar = null;
            this._tooltip.style.display = 'none';
            this._render();
        });
        this._canvas.addEventListener('click', (e) => this._handleClick(e));

        // Re-render on resize
        const ro = new ResizeObserver(() => {
            this._sizeCanvas();
            this._computeLayout();
            this._render();
        });
        ro.observe(this._canvas);
    }

    _sizeCanvas() {
        const rect = this._canvas.getBoundingClientRect();
        const dpr = window.devicePixelRatio || 1;
        this._canvas.width = rect.width * dpr;
        this._canvas.height = rect.height * dpr;
        this._ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    }

    async update() {
        this._updateBtn.disabled = true;
        this._updateBtn.textContent = 'Loading...';
        this._statusLabel.textContent = '';
        try {
            await this._fetchFilters();
            await this._fetchHistogram();
        } catch (err) {
            this._statusLabel.textContent = 'Error: ' + err.message;
        }
        this._updateBtn.disabled = false;
        this._updateBtn.textContent = 'Update';
    }

    async _fetchFilters() {
        const filters = await this._app.websocketManager.request({
            type: 'chart_filters',
        });
        this._populateSelect(this._pathGroupSelect, filters.path_groups || []);
        this._populateSelect(this._clockSelect, filters.clocks || []);
    }

    _populateSelect(select, items) {
        const prev = select.value;
        select.innerHTML = '<option value="">All</option>';
        for (const name of items) {
            const opt = document.createElement('option');
            opt.value = name;
            opt.textContent = name;
            select.appendChild(opt);
        }
        // Restore previous selection if still valid
        if (items.includes(prev)) {
            select.value = prev;
        }
    }

    async _fetchHistogram() {
        try {
            const req = {
                type: 'slack_histogram',
                is_setup: this._currentTab === 'setup' ? 1 : 0,
            };
            if (this._pathGroupSelect.value) {
                req.path_group = this._pathGroupSelect.value;
            }
            if (this._clockSelect.value) {
                req.clock_name = this._clockSelect.value;
            }
            const data = await this._app.websocketManager.request(req);
            this._histogramData = data;
            this._sizeCanvas();
            this._computeLayout();
            this._render();

            const total = data.total_endpoints || 0;
            const unconstrained = data.unconstrained_count || 0;
            const constrained = total - unconstrained;
            this._statusLabel.textContent = `${constrained} endpoints` +
                (unconstrained > 0 ? `, ${unconstrained} unconstrained` : '');
        } catch (err) {
            this._statusLabel.textContent = 'Error: ' + err.message;
        }
    }

    _computeLayout() {
        if (!this._histogramData) return;
        const rect = this._canvas.getBoundingClientRect();
        const result = computeHistogramLayout(
            this._histogramData, rect.width, rect.height);
        this._bars = result.bars;
        this._yMax = result.yMax;
        this._yTicks = result.yTicks;
        this._chartArea = result.chartArea;
    }

    render() { this._render(); }

    _render() {
        const rect = this._canvas.getBoundingClientRect();
        const w = rect.width;
        const h = rect.height;
        const ctx = this._ctx;
        const tc = getThemeColors();

        ctx.clearRect(0, 0, w, h);

        // Background
        ctx.fillStyle = tc.canvasBg;
        ctx.fillRect(0, 0, w, h);

        if (!this._bars || this._bars.length === 0) {
            ctx.fillStyle = tc.canvasText;
            ctx.font = '14px monospace';
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText('Click "Update" to load histogram', w / 2, h / 2);
            return;
        }

        this._drawAxes(ctx, tc);
        this._drawBars(ctx);
        this._drawTitle(ctx, w, tc);
    }

    _drawAxes(ctx, tc) {
        const ca = this._chartArea;
        if (!ca) return;

        ctx.strokeStyle = tc.canvasAxis;
        ctx.lineWidth = 1;

        // Y axis line
        ctx.beginPath();
        ctx.moveTo(ca.left, ca.top);
        ctx.lineTo(ca.left, ca.bottom);
        ctx.stroke();

        // X axis line
        ctx.beginPath();
        ctx.moveTo(ca.left, ca.bottom);
        ctx.lineTo(ca.right, ca.bottom);
        ctx.stroke();

        // Y axis ticks and labels
        ctx.fillStyle = tc.canvasLabel;
        ctx.font = '11px monospace';
        ctx.textAlign = 'right';
        ctx.textBaseline = 'middle';
        const chartHeight = ca.bottom - ca.top;
        for (const tick of this._yTicks) {
            const y = ca.bottom - (tick / this._yMax) * chartHeight;
            ctx.fillText(String(tick), ca.left - 6, y);
            if (tick > 0) {
                ctx.strokeStyle = tc.canvasGrid;
                ctx.beginPath();
                ctx.moveTo(ca.left, y);
                ctx.lineTo(ca.right, y);
                ctx.stroke();
                ctx.strokeStyle = tc.canvasAxis;
            }
        }

        // Y axis title
        ctx.save();
        ctx.translate(14, (ca.top + ca.bottom) / 2);
        ctx.rotate(-Math.PI / 2);
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillStyle = tc.canvasTitle;
        ctx.font = '11px monospace';
        ctx.fillText('Endpoints', 0, 0);
        ctx.restore();

        // X axis labels — show bin boundaries
        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';
        ctx.fillStyle = tc.canvasLabel;
        ctx.font = '10px monospace';

        const bins = this._histogramData.bins;
        const unit = this._histogramData.time_unit || '';

        // Determine precision from bin width
        const binWidth = bins.length > 0 ? bins[0].upper - bins[0].lower : 1;
        const precision = Math.max(0, -Math.floor(Math.log10(binWidth)));

        // Label at each bar boundary
        for (let i = 0; i <= this._bars.length; i++) {
            const val = i < bins.length ? bins[i].lower : bins[bins.length - 1].upper;
            const x = i < this._bars.length
                ? this._bars[i].x
                : this._bars[this._bars.length - 1].x + this._bars[this._bars.length - 1].width;
            ctx.fillText(val.toFixed(precision), x, ca.bottom + 4);
        }

        // X axis title
        ctx.fillStyle = tc.canvasTitle;
        ctx.font = '11px monospace';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';
        ctx.fillText(`Slack [${unit}]`, (ca.left + ca.right) / 2, ca.bottom + 22);
    }

    _drawBars(ctx) {
        for (const bar of this._bars) {
            if (bar.height <= 0) continue;

            const isHovered = (this._hoveredBar === bar);

            // Fill
            ctx.fillStyle = bar.negative ? kNegativeFill : kPositiveFill;
            if (isHovered) {
                // Lighten on hover
                ctx.fillStyle = bar.negative ? '#ff9999' : '#b0ffb0';
            }
            ctx.fillRect(bar.x, bar.y, bar.width, bar.height);

            // Border
            ctx.strokeStyle = bar.negative ? kNegativeBorder : kPositiveBorder;
            ctx.lineWidth = 1;
            ctx.strokeRect(bar.x, bar.y, bar.width, bar.height);
        }
    }

    _drawTitle(ctx, canvasWidth, tc) {
        const mode = this._currentTab === 'setup' ? 'Setup' : 'Hold';
        ctx.fillStyle = tc.fgPrimary;
        ctx.font = '13px monospace';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';
        ctx.fillText(`${mode} Endpoint Slack`, canvasWidth / 2, 8);
    }

    _hitTestBar(e) {
        if (!this._bars) return null;
        const rect = this._canvas.getBoundingClientRect();
        const mx = e.clientX - rect.left;
        const my = e.clientY - rect.top;
        for (const bar of this._bars) {
            if (mx >= bar.x && mx <= bar.x + bar.width &&
                my >= bar.y && my <= bar.y + bar.height) {
                return bar;
            }
        }
        return null;
    }

    _handleHover(e) {
        const bar = this._hitTestBar(e);
        if (bar !== this._hoveredBar) {
            this._hoveredBar = bar;
            this._render();
        }

        if (bar) {
            const unit = this._histogramData?.time_unit || '';
            const binWidth = bar.upper - bar.lower;
            const precision = Math.max(0, -Math.floor(Math.log10(binWidth)));
            this._tooltip.textContent =
                `Endpoints: ${bar.count}\n` +
                `Slack: [${bar.lower.toFixed(precision)}, ${bar.upper.toFixed(precision)}) ${unit}`;
            this._tooltip.style.display = 'block';

            const rect = this._el.getBoundingClientRect();
            const tx = e.clientX - rect.left + 12;
            const ty = e.clientY - rect.top - 10;
            this._tooltip.style.left = tx + 'px';
            this._tooltip.style.top = ty + 'px';
        } else {
            this._tooltip.style.display = 'none';
        }
    }

    async _handleClick(e) {
        const bar = this._hitTestBar(e);
        if (!bar || bar.count === 0) return;

        try {
            const resp = await this._app.websocketManager.request({
                type: 'timing_report',
                is_setup: this._currentTab === 'setup' ? 1 : 0,
                max_paths: 50,
                slack_min: bar.lower,
                slack_max: bar.upper,
            });

            if (this._app.timingWidget) {
                this._app.timingWidget.showPaths(
                    this._currentTab, resp.paths || []);
                if (this._app.focusComponent) {
                    this._app.focusComponent('TimingWidget');
                }
            }
        } catch (err) {
            console.error('Charts bar click error:', err);
        }
    }
}
