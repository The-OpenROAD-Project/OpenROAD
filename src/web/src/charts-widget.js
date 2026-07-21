// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Canvas-based charts widget: slack (setup/hold), net fanout, net length (HPWL)
// histograms plus engine-provided line charts (GPL/MPL/DPL/PAD), all selected
// from a single dropdown.  No charting library — everything is drawn on a 2D
// canvas.

import { getThemeColors } from './theme.js';
import { isStaticMode } from './ui-utils.js';

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

// Line chart / stacked-series colors (Tableau 10)
const kLineColors = ['#4e79a7', '#f28e2b', '#e15759', '#76b7b2',
                     '#59a14f', '#edc948', '#b07aa1', '#ff9da7'];
const kNegativeHighlight = 'rgba(240,128,128,0.12)';
const kPositiveHighlight = 'rgba(144,238,144,0.12)';
const kNegativeHover = '#ff9999';
const kPositiveHover = '#b0ffb0';

// Built-in histogram chart types (value → label) shown in the dropdown.
const kBuiltinCharts = [
    { value: 'setup', label: 'Setup Slack' },
    { value: 'hold', label: 'Hold Slack' },
    { value: 'fanout', label: 'Net Fanout' },
    { value: 'netlength', label: 'Net Length (HPWL)' },
];

// Pure hit-test — returns the bar whose column contains (mx, my), or null.
// Uses the full column height (chartArea top to bottom) so that buckets with
// few paths are easy to click.
export function hitTestColumn(bars, chartArea, mx, my) {
    if (!bars || !chartArea) return null;
    if (my < chartArea.top || my > chartArea.bottom) return null;
    for (const bar of bars) {
        if (mx >= bar.x && mx <= bar.x + bar.width) {
            return bar;
        }
    }
    return null;
}

// Map a count to a [0,1] vertical fraction.  Log mode uses log10(c+1) so a
// count of 0 maps to 0 and the fraction is monotonic over all integers.
function countFraction(count, yMax, logY) {
    if (logY) {
        return Math.log10(count + 1) / Math.log10(yMax + 1);
    }
    return count / yMax;
}

// Pure layout computation — extracted for testability.
// `opts.logY` switches the Y axis to a log scale (useful for highly skewed
// distributions like net fanout).
export function computeHistogramLayout(histogramData, canvasWidth, canvasHeight,
                                       opts = {}) {
    const logY = !!opts.logY;
    const bins = histogramData?.bins;
    if (!bins || bins.length === 0) {
        return { bars: [], yMax: 0, yTicks: [], chartArea: null, logY };
    }

    const chartLeft = kLeftMargin;
    const chartRight = canvasWidth - kRightMargin;
    const chartTop = kTopMargin;
    const chartBottom = canvasHeight - kBottomMargin;
    const chartWidth = chartRight - chartLeft;
    const chartHeight = chartBottom - chartTop;

    if (chartWidth <= 0 || chartHeight <= 0) {
        return { bars: [], yMax: 0, yTicks: [], chartArea: null, logY };
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
    const { yMax, yTicks }
        = logY ? computeLogYAxis(maxCount) : computeYAxis(maxCount);

    // Compute bar rectangles
    const barWidth = (chartWidth - kBarGap * (bins.length - 1)) / bins.length;
    const bars = [];
    for (let i = 0; i < bins.length; i++) {
        const bin = bins[i];
        const barHeight = countFraction(bin.count, yMax, logY) * chartHeight;
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

    return { bars, yMax, yTicks, chartArea, logY };
}

// Compute nice Y-axis max and tick values.
// Picks a tick interval from {1, 2, 5} × 10^n targeting ~kTargetTicks ticks,
// then rounds yMax up to the nearest interval so the tallest bar fills most
// of the chart instead of leaving the upper half empty.
function computeYAxis(maxCount) {
    if (maxCount <= 10) {
        const ticks = [];
        for (let i = 0; i <= maxCount; i++) ticks.push(i);
        return { yMax: maxCount, yTicks: ticks };
    }

    const kTargetTicks = 8;
    const raw = maxCount / kTargetTicks;
    const mag = Math.pow(10, Math.floor(Math.log10(raw)));
    const residual = raw / mag;
    let niceCoeff;
    if (residual < 1.5) niceCoeff = 1;
    else if (residual < 3) niceCoeff = 2;
    else if (residual < 7) niceCoeff = 5;
    else niceCoeff = 10;
    const interval = niceCoeff * mag;
    const yMax = Math.ceil(maxCount / interval) * interval;

    const ticks = [];
    for (let v = 0; v <= yMax + interval / 2; v += interval) {
        ticks.push(v);
    }
    return { yMax, yTicks: ticks };
}

// Log-scale Y axis: ticks at powers of 10 up to the next decade above
// maxCount, plus 0 at the bottom.  The bar height mapping uses log(c+1) so 0
// counts render flat against the axis.
function computeLogYAxis(maxCount) {
    const topDecade = Math.max(1,
        Math.pow(10, Math.ceil(Math.log10(Math.max(1, maxCount + 1)))));
    const ticks = [0];
    for (let v = 1; v <= topDecade + 0.5; v *= 10) {
        ticks.push(v);
    }
    return { yMax: topDecade, yTicks: ticks };
}

// Build CSV text for a histogram (bins + optional stacked series) or a line
// chart.  Pure — extracted for testability.
export function buildChartCsv(kind, data) {
    if (!data) return '';
    if (kind === 'generic') {
        const yl = data.y_labels || [];
        const header = ['x', ...yl].join(',');
        const rows = (data.points || []).map(
            (p) => [p.x, ...(p.ys || [])].join(','));
        return [header, ...rows].join('\n');
    }
    // histogram
    const bins = data.bins || [];
    const series = Array.isArray(data.series) ? data.series : [];
    const header = ['lower', 'upper', 'count',
                    ...series.map((s) => s.name || 'group')].join(',');
    const rows = bins.map((b, i) => {
        const cols = [b.lower, b.upper, b.count];
        for (const s of series) cols.push((s.counts && s.counts[i]) || 0);
        return cols.join(',');
    });
    return [header, ...rows].join('\n');
}

export class ChartsWidget {
    constructor(app, redrawAllLayers) {
        this._app = app;
        this._redrawAllLayers = redrawAllLayers;
        // Histogram kind: 'setup'|'hold' (endpoint slack), 'fanout', 'netlength'.
        this._currentTab = 'setup';
        this._histogramData = null;
        this._bars = [];
        this._yMax = 0;
        this._yTicks = [];
        this._chartArea = null;
        this._hoveredBar = null;

        // Engine line charts (GPL HPWL, MPL area, etc.).
        this._debugCharts = [];
        this._activeDebugChart = -1;  // -1 = show the selected histogram

        this._build();
    }

    // ---- DOM construction ----

    _build() {
        const el = document.createElement('div');
        el.className = 'charts-widget';

        // Toolbar: Update + chart-type dropdown + exports + status.
        const toolbar = document.createElement('div');
        toolbar.className = 'charts-toolbar';

        this._updateBtn = document.createElement('button');
        this._updateBtn.className = 'timing-btn';
        this._updateBtn.textContent = 'Update';
        if (isStaticMode(this._app)) {
            this._updateBtn.style.display = 'none';
        }

        // Unified chart-type selector (built-ins + engine line charts).
        this._chartSelect = document.createElement('select');
        this._chartSelect.className = 'charts-select';

        this._csvBtn = document.createElement('button');
        this._csvBtn.className = 'timing-btn';
        this._csvBtn.textContent = 'CSV';
        this._csvBtn.title = 'Export the current chart data as CSV';

        this._pngBtn = document.createElement('button');
        this._pngBtn.className = 'timing-btn';
        this._pngBtn.textContent = 'PNG';
        this._pngBtn.title = 'Export the current chart as a PNG image';

        this._statusLabel = document.createElement('span');
        this._statusLabel.className = 'timing-path-count';

        toolbar.appendChild(this._updateBtn);
        toolbar.appendChild(this._chartSelect);
        toolbar.appendChild(this._csvBtn);
        toolbar.appendChild(this._pngBtn);
        toolbar.appendChild(this._statusLabel);
        el.appendChild(toolbar);

        // Filter row (path groups + clock) — only for slack histograms.
        const filterRow = document.createElement('div');
        filterRow.className = 'charts-toolbar';

        const pgLabel = document.createElement('span');
        pgLabel.textContent = 'Path Groups:';
        pgLabel.style.color = 'var(--fg-muted)';
        pgLabel.style.fontSize = '12px';
        // Multiple selection → stacked histogram (Qt parity). None → all.
        this._pathGroupSelect = document.createElement('select');
        this._pathGroupSelect.className = 'charts-select';
        this._pathGroupSelect.multiple = true;
        this._pathGroupSelect.size = 1;
        this._pathGroupSelect.title = 'Select 0 = all, or ≥1 to stack by group';

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

        // Filter row hidden for non-slack charts.
        this._filterRow = filterRow;

        this.element = el;

        this._ctx = this._canvas.getContext('2d');
        this._rebuildSelect();
        this._bindEvents();

        if (isStaticMode(this._app)) {
            setTimeout(() => this.update(), 0);
        }
    }

    _bindEvents() {
        this._updateBtn.addEventListener('click', () => this.update());
        this._chartSelect.addEventListener('change',
            () => this._onSelectChange());
        this._csvBtn.addEventListener('click', () => this._exportCsv());
        this._pngBtn.addEventListener('click', () => this._exportPng());

        this._pathGroupSelect.addEventListener('change',
            () => this._fetchHistogram());
        this._clockSelect.addEventListener('change',
            () => this._fetchHistogram());

        this._canvas.addEventListener('mousemove', (e) => this._handleHover(e));
        this._canvas.addEventListener('mouseleave', () => {
            this._hoveredBar = null;
            this._tooltip.style.display = 'none';
            this._syncView();
        });
        this._canvas.addEventListener('click', (e) => this._handleClick(e));
        this._canvas.addEventListener('dblclick',
            (e) => this._handleDblClick(e));

        const ro = new ResizeObserver(() => this._syncView());
        ro.observe(this._canvas);
    }

    _sizeCanvas() {
        const rect = this._canvas.getBoundingClientRect();
        const dpr = window.devicePixelRatio || 1;
        this._canvas.width = rect.width * dpr;
        this._canvas.height = rect.height * dpr;
        this._ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    }

    // Populate the dropdown with built-in histograms + engine line charts, and
    // set its value to the current selection.
    _rebuildSelect() {
        const sel = this._chartSelect;
        sel.innerHTML = '';
        for (const c of kBuiltinCharts) {
            const opt = document.createElement('option');
            opt.value = c.value;
            opt.textContent = c.label;
            sel.appendChild(opt);
        }
        this._debugCharts.forEach((chart, i) => {
            const opt = document.createElement('option');
            opt.value = 'gen:' + i;
            opt.textContent = chart.name || ('Chart ' + i);
            sel.appendChild(opt);
        });
        sel.value = this._activeDebugChart >= 0
            ? 'gen:' + this._activeDebugChart : this._currentTab;
    }

    _onSelectChange() {
        const v = this._chartSelect.value;
        if (v.startsWith('gen:')) {
            this._activeDebugChart = parseInt(v.slice(4), 10) || 0;
            this._hoveredBar = null;
            this._tooltip.style.display = 'none';
            this._syncView();
        } else {
            // _selectTab owns resetting _activeDebugChart to -1.
            this._selectTab(v);
        }
    }

    async update() {
        this._updateBtn.disabled = true;
        this._updateBtn.textContent = 'Loading...';
        this._statusLabel.textContent = '';
        try {
            // Surface any engine-provided line charts on-demand (not only
            // during the placement debugger pause).  The generic charts are
            // independent of the histogram, so fetch them concurrently.
            if (this._activeDebugChart < 0) {
                if (this._currentTab === 'setup'
                    || this._currentTab === 'hold') {
                    await this._fetchFilters();
                }
                await Promise.all(
                    [this._fetchHistogram(), this._fetchGenericCharts()]);
            } else {
                await this._fetchGenericCharts();
            }
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
        this._populatePathGroups(filters.path_groups || []);
        this._populateClocks(filters.clocks || []);
    }

    _populatePathGroups(items) {
        const prev = new Set(Array.from(this._pathGroupSelect.selectedOptions,
                                        (o) => o.value));
        this._pathGroupSelect.innerHTML = '';
        for (const name of items) {
            const opt = document.createElement('option');
            opt.value = name;
            opt.textContent = name;
            opt.selected = prev.has(name);
            this._pathGroupSelect.appendChild(opt);
        }
    }

    _populateClocks(items) {
        const prev = this._clockSelect.value;
        this._clockSelect.innerHTML = '<option value="">All</option>';
        for (const name of items) {
            const opt = document.createElement('option');
            opt.value = name;
            opt.textContent = name;
            this._clockSelect.appendChild(opt);
        }
        if (items.includes(prev)) this._clockSelect.value = prev;
    }

    async _fetchGenericCharts() {
        try {
            const data = await this._app.websocketManager.request(
                { type: 'debug_charts' });
            this.setDebugCharts(data.charts || []);
        } catch (err) {
            /* engine charts are optional */
        }
    }

    async _fetchHistogram() {
        try {
            // Remember which tab issued this request so a response that arrives
            // after the user switches tabs can be discarded instead of being
            // rendered/interpreted as the wrong histogram type.
            const requestedTab = this._currentTab;
            let req;
            if (requestedTab === 'fanout') {
                req = { type: 'fanout_histogram' };
            } else if (requestedTab === 'netlength') {
                req = { type: 'net_length_histogram',
                        use_dbu: !!this._app.showDbu };
            } else {
                req = {
                    type: 'slack_histogram',
                    is_setup: requestedTab === 'setup',
                };
                const groups = Array.from(
                    this._pathGroupSelect.selectedOptions, (o) => o.value)
                    .filter(Boolean);
                if (groups.length > 0) req.path_groups = groups;
                if (this._clockSelect.value) {
                    req.clock_name = this._clockSelect.value;
                }
            }
            const data = await this._app.websocketManager.request(req);
            // Stale response: the user switched tabs while this was in flight.
            if (this._currentTab !== requestedTab) {
                return;
            }
            this._histogramData = data;
            this._syncView();

            if (requestedTab === 'fanout' || requestedTab === 'netlength') {
                const total = data.total_nets || 0;
                this._statusLabel.textContent = `${total} nets`;
            } else {
                const total = data.total_endpoints || 0;
                const unconstrained = data.unconstrained_count || 0;
                const constrained = total - unconstrained;
                this._statusLabel.textContent = `${constrained} endpoints` +
                    (unconstrained > 0 ? `, ${unconstrained} unconstrained` : '');
            }
        } catch (err) {
            this._statusLabel.textContent = 'Error: ' + err.message;
        }
    }

    _selectTab(tab) {
        if (this._currentTab === tab && this._activeDebugChart < 0) return;
        this._currentTab = tab;
        this._activeDebugChart = -1;
        if (this._chartSelect) this._chartSelect.value = tab;
        // Path-group / clock filters only apply to slack histograms.
        this._filterRow.style.display =
            (tab === 'setup' || tab === 'hold') ? '' : 'none';
        this._hoveredBar = null;
        this._tooltip.style.display = 'none';
        this.update();
    }

    _computeLayout() {
        if (!this._histogramData) return;
        const rect = this._canvas.getBoundingClientRect();
        const result = computeHistogramLayout(
            this._histogramData, rect.width, rect.height,
            { logY: this._currentTab === 'fanout' });
        this._bars = result.bars;
        this._yMax = result.yMax;
        this._yTicks = result.yTicks;
        this._chartArea = result.chartArea;
        this._logY = !!result.logY;
    }

    render() { this._syncView(); }

    _stackedSeries() {
        const s = this._histogramData?.series;
        return (Array.isArray(s) && s.length > 1) ? s : null;
    }

    _render() {
        const rect = this._canvas.getBoundingClientRect();
        const w = rect.width;
        const h = rect.height;
        const ctx = this._ctx;
        const tc = getThemeColors();

        ctx.clearRect(0, 0, w, h);
        ctx.fillStyle = tc.canvasBg;
        ctx.fillRect(0, 0, w, h);

        if (!this._bars || this._bars.length === 0) {
            ctx.fillStyle = tc.canvasText;
            ctx.font = '14px monospace';
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            const msg = isStaticMode(this._app)
                ? 'No histogram data available'
                : 'Click "Update" to load histogram';
            ctx.fillText(msg, w / 2, h / 2);
            return;
        }

        this._drawAxes(ctx, tc);
        this._drawBars(ctx);
        this._drawTitle(ctx, w, tc);
        if (this._stackedSeries()) this._drawSeriesLegend(ctx, tc);
    }

    _drawAxes(ctx, tc) {
        const ca = this._chartArea;
        if (!ca) return;

        ctx.strokeStyle = tc.canvasAxis;
        ctx.lineWidth = 1;

        ctx.beginPath();
        ctx.moveTo(ca.left, ca.top);
        ctx.lineTo(ca.left, ca.bottom);
        ctx.stroke();

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
            const frac = this._logY
                ? Math.log10(tick + 1) / Math.log10(this._yMax + 1)
                : tick / this._yMax;
            const y = ca.bottom - frac * chartHeight;
            ctx.fillText(this._formatCount(tick), ca.left - 6, y);
            if (tick > 0) {
                ctx.strokeStyle = tc.canvasGrid;
                ctx.beginPath();
                ctx.moveTo(ca.left, y);
                ctx.lineTo(ca.right, y);
                ctx.stroke();
                ctx.strokeStyle = tc.canvasAxis;
            }
        }

        const tab = this._currentTab;
        const netCount = (tab === 'fanout' || tab === 'netlength');

        // Y axis title
        ctx.save();
        ctx.translate(14, (ca.top + ca.bottom) / 2);
        ctx.rotate(-Math.PI / 2);
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillStyle = tc.canvasTitle;
        ctx.font = '11px monospace';
        ctx.fillText(this._logY ? 'Nets (log)'
                                : (netCount ? 'Nets' : 'Endpoints'), 0, 0);
        ctx.restore();

        // X axis labels — show bin boundaries
        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';
        ctx.fillStyle = tc.canvasLabel;
        ctx.font = '10px monospace';

        const bins = this._histogramData.bins;
        const unit = this._histogramData.time_unit
            || this._histogramData.length_unit || '';

        const binWidth = bins.length > 0 ? bins[0].upper - bins[0].lower : 1;
        const precision = (tab === 'fanout')
            ? 0 : Math.max(0, -Math.floor(Math.log10(binWidth)));

        for (let i = 0; i <= this._bars.length; i++) {
            const val = i < bins.length
                ? bins[i].lower : bins[bins.length - 1].upper;
            const x = i < this._bars.length
                ? this._bars[i].x
                : this._bars[this._bars.length - 1].x
                  + this._bars[this._bars.length - 1].width;
            ctx.fillText(val.toFixed(precision), x, ca.bottom + 4);
        }

        // X axis title
        ctx.fillStyle = tc.canvasTitle;
        ctx.font = '11px monospace';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';
        let xTitle;
        if (tab === 'fanout') xTitle = 'Fanout (loads)';
        else if (tab === 'netlength') xTitle = `Net length [${unit}]`;
        else xTitle = `Slack [${unit}]`;
        ctx.fillText(xTitle, (ca.left + ca.right) / 2, ca.bottom + 22);
    }

    _drawBars(ctx) {
        const ca = this._chartArea;
        const series = this._stackedSeries();
        for (let i = 0; i < this._bars.length; i++) {
            const bar = this._bars[i];
            const isHovered = (this._hoveredBar === bar);

            if (isHovered && ca) {
                ctx.fillStyle = bar.negative
                    ? kNegativeHighlight : kPositiveHighlight;
                ctx.fillRect(bar.x, ca.top, bar.width, ca.bottom - ca.top);
            }

            if (bar.height <= 0 || bar.count <= 0) continue;

            if (series) {
                // Stacked: one colored segment per path group, bottom-up.
                let yBottom = bar.y + bar.height;
                for (let s = 0; s < series.length; s++) {
                    const c = (series[s].counts && series[s].counts[i]) || 0;
                    if (c <= 0) continue;
                    const segH = bar.height * (c / bar.count);
                    const segY = yBottom - segH;
                    ctx.fillStyle = kLineColors[s % kLineColors.length];
                    ctx.fillRect(bar.x, segY, bar.width, segH);
                    yBottom = segY;
                }
                ctx.strokeStyle = 'rgba(0,0,0,0.35)';
                ctx.lineWidth = 1;
                ctx.strokeRect(bar.x, bar.y, bar.width, bar.height);
                continue;
            }

            ctx.fillStyle = bar.negative ? kNegativeFill : kPositiveFill;
            if (isHovered) {
                ctx.fillStyle = bar.negative ? kNegativeHover : kPositiveHover;
            }
            ctx.fillRect(bar.x, bar.y, bar.width, bar.height);

            ctx.strokeStyle = bar.negative ? kNegativeBorder : kPositiveBorder;
            ctx.lineWidth = 1;
            ctx.strokeRect(bar.x, bar.y, bar.width, bar.height);
        }
    }

    _drawSeriesLegend(ctx, tc) {
        const series = this._stackedSeries();
        const ca = this._chartArea;
        if (!series || !ca) return;
        ctx.font = '10px monospace';
        ctx.textAlign = 'left';
        ctx.textBaseline = 'top';
        const lx = ca.right - 120;
        const ly = ca.top + 4;
        for (let s = 0; s < series.length; s++) {
            ctx.fillStyle = kLineColors[s % kLineColors.length];
            ctx.fillRect(lx, ly + s * 14, 12, 10);
            ctx.fillStyle = tc.canvasLabel;
            ctx.fillText(series[s].name || `group ${s}`, lx + 16, ly + s * 14);
        }
    }

    _drawTitle(ctx, canvasWidth, tc) {
        let title;
        if (this._currentTab === 'fanout') title = 'Net Fanout';
        else if (this._currentTab === 'netlength') title = 'Net Length (HPWL)';
        else {
            const mode = this._currentTab === 'setup' ? 'Setup' : 'Hold';
            title = `${mode} Endpoint Slack`;
        }
        ctx.fillStyle = tc.fgPrimary;
        ctx.font = '13px monospace';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';
        ctx.fillText(title, canvasWidth / 2, 8);
    }

    _hitTestBar(e) {
        const rect = this._canvas.getBoundingClientRect();
        const mx = e.clientX - rect.left;
        const my = e.clientY - rect.top;
        return hitTestColumn(this._bars, this._chartArea, mx, my);
    }

    _handleHover(e) {
        if (this._activeDebugChart >= 0) {
            this._hoveredBar = null;
            this._tooltip.style.display = 'none';
            return;
        }
        const bar = this._hitTestBar(e);
        if (bar !== this._hoveredBar) {
            this._hoveredBar = bar;
            this._render();
        }

        if (bar) {
            const tab = this._currentTab;
            const binWidth = bar.upper - bar.lower;
            if (tab === 'fanout') {
                const hiInclusive = bar.upper - 1;
                const range = (bar.lower === hiInclusive)
                    ? `${bar.lower}` : `[${bar.lower}, ${hiInclusive}]`;
                this._tooltip.textContent = `Nets: ${bar.count}\nFanout: ${range}`;
            } else if (tab === 'netlength') {
                const unit = this._histogramData?.length_unit || '';
                const prec = Math.max(0, -Math.floor(Math.log10(binWidth)));
                this._tooltip.textContent =
                    `Nets: ${bar.count}\n`
                    + `Length: [${bar.lower.toFixed(prec)}, `
                    + `${bar.upper.toFixed(prec)}) ${unit}`;
            } else {
                const unit = this._histogramData?.time_unit || '';
                const prec = Math.max(0, -Math.floor(Math.log10(binWidth)));
                this._tooltip.textContent =
                    `Endpoints: ${bar.count}\n`
                    + `Slack: [${bar.lower.toFixed(prec)}, `
                    + `${bar.upper.toFixed(prec)}) ${unit}`;
            }
            this._tooltip.style.display = 'block';

            const rect = this.element.getBoundingClientRect();
            const ttRect = this._tooltip.getBoundingClientRect();
            const margin = 4;
            const cursorX = e.clientX - rect.left;
            const cursorY = e.clientY - rect.top;
            let tx = cursorX + 12;
            if (tx + ttRect.width > rect.width - margin) {
                tx = cursorX - 12 - ttRect.width;
            }
            tx = Math.max(margin, tx);
            let ty = cursorY - 10;
            ty = Math.min(rect.height - ttRect.height - margin,
                Math.max(margin, ty));
            this._tooltip.style.left = tx + 'px';
            this._tooltip.style.top = ty + 'px';
        } else {
            this._tooltip.style.display = 'none';
        }
    }

    async _handleClick(e) {
        if (this._activeDebugChart >= 0) return;
        // Only slack histograms drill down to timing paths.
        if (this._currentTab === 'fanout' || this._currentTab === 'netlength') {
            return;
        }
        const bar = this._hitTestBar(e);
        if (!bar || bar.count === 0) return;

        try {
            const resp = await this._app.websocketManager.request({
                type: 'timing_report',
                is_setup: this._currentTab === 'setup',
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

    // Double-click on a fanout / net-length bar selects every net in that bin
    // (server-side multi-selection) and opens the first in the Inspector.
    async _handleDblClick(e) {
        if (this._activeDebugChart >= 0) return;
        let type;
        if (this._currentTab === 'fanout') type = 'select_fanout_bin';
        else if (this._currentTab === 'netlength') type = 'select_net_length_bin';
        else return;
        const bar = this._hitTestBar(e);
        if (!bar || bar.count === 0) return;
        try {
            const resp = await this._app.websocketManager.request({
                type,
                lower: bar.lower,
                upper: bar.upper,
                use_dbu: this._app.showDbu,
            });
            if (resp && resp.truncated) {
                console.warn(
                    `Bin has ${resp.count} nets; selection capped at `
                    + `${resp.selection_limit} for performance.`);
            }
            if (this._app.updateInspector) {
                this._app.updateInspector(resp);
            }
            if (this._app.focusComponent) {
                this._app.focusComponent('Inspector');
            }
            if (this._redrawAllLayers) {
                this._redrawAllLayers();
            }
        } catch (err) {
            console.error('Net bin select failed:', err);
        }
    }

    // ---- Exports ----

    _activeChartData() {
        if (this._activeDebugChart >= 0) {
            return { kind: 'generic',
                     data: this._debugCharts[this._activeDebugChart] };
        }
        return { kind: 'histogram', data: this._histogramData };
    }

    // Base filename (no extension) for the currently shown chart.
    _chartBaseName() {
        return this._activeDebugChart >= 0
            ? (this._debugCharts[this._activeDebugChart].name || 'chart')
            : this._currentTab;
    }

    // Trigger a browser download of `url` as `filename`.  Pass revoke:true for
    // object URLs (Blob) that need releasing; data URLs (canvas) don't.
    _triggerDownload(filename, url, { revoke = false } = {}) {
        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        document.body.appendChild(a);
        a.click();
        a.remove();
        if (revoke) URL.revokeObjectURL(url);
    }

    _download(filename, mime, content) {
        try {
            const url = URL.createObjectURL(new Blob([content], { type: mime }));
            this._triggerDownload(filename, url, { revoke: true });
        } catch (err) {
            console.error('Download failed:', err);
        }
    }

    _exportCsv() {
        const { kind, data } = this._activeChartData();
        if (!data) return;
        const csv = buildChartCsv(kind, data);
        this._download(this._chartBaseName() + '.csv', 'text/csv', csv);
    }

    _exportPng() {
        try {
            const url = this._canvas.toDataURL('image/png');
            this._triggerDownload(this._chartBaseName() + '.png', url);
        } catch (err) {
            console.error('PNG export failed:', err);
        }
    }

    // ---- Engine line charts (GPL HPWL, density, etc.) ----

    setDebugCharts(charts) {
        this._debugCharts = charts;
        if (charts.length === 0) {
            this._activeDebugChart = -1;
        } else if (this._activeDebugChart >= charts.length) {
            this._activeDebugChart = 0;
        }
        this._rebuildSelect();
        this._syncView();
    }

    _syncView() {
        const isGeneric = this._activeDebugChart >= 0;
        if (isGeneric) {
            this._hoveredBar = null;
            this._tooltip.style.display = 'none';
        }
        // Filters apply only to slack histograms.
        const isSlack = !isGeneric
            && (this._currentTab === 'setup' || this._currentTab === 'hold');
        if (this._filterRow) {
            this._filterRow.style.display = isSlack ? '' : 'none';
        }
        this._sizeCanvas();
        if (isGeneric) {
            this._renderLineChart(this._debugCharts[this._activeDebugChart]);
        } else {
            this._computeLayout();
            this._render();
        }
    }

    _renderLineChart(chart) {
        const rect = this._canvas.getBoundingClientRect();
        const w = rect.width;
        const h = rect.height;
        const ctx = this._ctx;
        const tc = getThemeColors();

        ctx.clearRect(0, 0, w, h);
        ctx.fillStyle = tc.canvasBg;
        ctx.fillRect(0, 0, w, h);

        const pts = chart.points;
        if (!pts || pts.length === 0) {
            ctx.fillStyle = tc.canvasText;
            ctx.font = '14px monospace';
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText('No data points yet', w / 2, h / 2);
            return;
        }

        const numSeries = chart.y_labels ? chart.y_labels.length : 1;
        const cLeft = kLeftMargin;
        const cRight = w - kRightMargin;
        const cTop = kTopMargin;
        const cBottom = h - kBottomMargin;
        const cW = cRight - cLeft;
        const cH = cBottom - cTop;
        if (cW <= 0 || cH <= 0) return;

        let xMin = pts[0].x, xMax = pts[0].x;
        let yMin = Infinity, yMax = -Infinity;
        for (const p of pts) {
            if (p.x < xMin) xMin = p.x;
            if (p.x > xMax) xMax = p.x;
            for (const v of p.ys) {
                if (v < yMin) yMin = v;
                if (v > yMax) yMax = v;
            }
        }
        if (xMin === xMax) xMax = xMin + 1;
        if (!isFinite(yMin) || !isFinite(yMax)) { yMin = 0; yMax = 1; }
        const yPad = (yMax - yMin) * 0.05 || 1;
        yMin -= yPad;
        yMax += yPad;

        const toX = (v) => cLeft + ((v - xMin) / (xMax - xMin)) * cW;
        const toY = (v) => cBottom - ((v - yMin) / (yMax - yMin)) * cH;

        ctx.strokeStyle = tc.canvasAxis;
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(cLeft, cTop);
        ctx.lineTo(cLeft, cBottom);
        ctx.lineTo(cRight, cBottom);
        ctx.stroke();

        ctx.fillStyle = tc.canvasLabel;
        ctx.font = '10px monospace';
        ctx.textAlign = 'right';
        ctx.textBaseline = 'middle';
        const nYTicks = 5;
        for (let i = 0; i <= nYTicks; i++) {
            const v = yMin + (yMax - yMin) * i / nYTicks;
            const y = toY(v);
            ctx.fillText(this._formatNum(v), cLeft - 4, y);
            if (i > 0 && i < nYTicks) {
                ctx.strokeStyle = tc.canvasGrid;
                ctx.beginPath();
                ctx.moveTo(cLeft, y);
                ctx.lineTo(cRight, y);
                ctx.stroke();
            }
        }

        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';
        if (pts.length === 1) {
            ctx.fillStyle = tc.canvasLabel;
            ctx.fillText(this._formatNum(pts[0].x), toX(pts[0].x), cBottom + 4);
        } else {
            const nXTicks = Math.min(pts.length - 1, 6);
            for (let i = 0; i <= nXTicks; i++) {
                const v = xMin + (xMax - xMin) * i / nXTicks;
                const x = toX(v);
                ctx.fillStyle = tc.canvasLabel;
                ctx.fillText(this._formatNum(v), x, cBottom + 4);
            }
        }

        ctx.fillStyle = tc.canvasTitle;
        ctx.font = '11px monospace';
        ctx.textAlign = 'center';
        ctx.fillText(chart.x_label || '', (cLeft + cRight) / 2, cBottom + 22);

        ctx.fillStyle = tc.fgPrimary;
        ctx.font = '13px monospace';
        ctx.textBaseline = 'top';
        ctx.fillText(chart.name, w / 2, 4);

        for (let s = 0; s < numSeries; s++) {
            ctx.strokeStyle = kLineColors[s % kLineColors.length];
            ctx.lineWidth = 1.5;
            ctx.beginPath();
            let first = true;
            for (const p of pts) {
                const px = toX(p.x);
                const py = toY(p.ys[s]);
                if (first) { ctx.moveTo(px, py); first = false; }
                else { ctx.lineTo(px, py); }
            }
            ctx.stroke();
        }

        if (numSeries > 1 && chart.y_labels) {
            ctx.font = '10px monospace';
            ctx.textAlign = 'left';
            ctx.textBaseline = 'top';
            const lx = cLeft + 8;
            const ly = cTop + 4;
            for (let s = 0; s < numSeries; s++) {
                const color = kLineColors[s % kLineColors.length];
                ctx.fillStyle = color;
                ctx.fillRect(lx, ly + s * 14, 12, 10);
                ctx.fillStyle = tc.canvasLabel;
                ctx.fillText(chart.y_labels[s], lx + 16, ly + s * 14);
            }
        }
    }

    _formatNum(v) {
        const abs = Math.abs(v);
        if (abs === 0) return '0';
        if (abs >= 1e6) return (v / 1e6).toFixed(1) + 'M';
        if (abs >= 1e3) return (v / 1e3).toFixed(1) + 'k';
        if (abs >= 1) return v.toFixed(1);
        return v.toPrecision(3);
    }

    _formatCount(v) {
        if (v === 0) return '0';
        if (v >= 1e6) return (v / 1e6) + 'M';
        if (v >= 1e3) return (v / 1e3) + 'k';
        return String(v);
    }
}
