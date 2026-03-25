// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Pure SVG renderer for slack histograms.
// Takes computeHistogramLayout() output and produces an SVG string.

import { computeHistogramLayout } from './charts-widget.js';

// Colors — reuse from charts-widget.js when loaded as ES module,
// redeclare with unique names for bundled/concatenated mode.
const kSvgNegativeFill = '#f08080';    // lightcoral
const kSvgNegativeBorder = '#8b0000';  // darkred
const kSvgPositiveFill = '#90ee90';    // lightgreen
const kSvgPositiveBorder = '#006400';  // darkgreen

const kSvgAxisColor = '#888';
const kSvgTextColor = '#ccc';
const kSvgGridColor = '#333';

function escapeXml(s) {
    return String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;')
        .replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}

// Render a slack histogram as an SVG string.
//
// histogramData: { bins: [{lower, upper, count, negative},...],
//                  time_unit, total_endpoints, unconstrained_count }
// width, height: SVG dimensions in pixels
export function renderHistogramSVG(histogramData, width = 800, height = 400) {
    const layout = computeHistogramLayout(histogramData, width, height);
    if (!layout.chartArea) {
        return `<svg xmlns="http://www.w3.org/2000/svg" width="${width}" height="${height}">` +
            `<text x="${width / 2}" y="${height / 2}" text-anchor="middle" ` +
            `fill="${kSvgTextColor}" font-family="monospace" font-size="14">No data</text></svg>`;
    }

    const { bars, yMax, yTicks, chartArea } = layout;
    const parts = [];

    parts.push(`<svg xmlns="http://www.w3.org/2000/svg" width="${width}" height="${height}" ` +
        `font-family="monospace" font-size="11">`);

    // Background
    parts.push(`<rect width="${width}" height="${height}" fill="#1e1e1e"/>`);

    // Title
    const unit = histogramData.time_unit || 'ns';
    const total = histogramData.total_endpoints || 0;
    parts.push(`<text x="${width / 2}" y="18" text-anchor="middle" ` +
        `fill="${kSvgTextColor}" font-size="13">Endpoint Slack (${escapeXml(unit)}) — ` +
        `${total} endpoints</text>`);

    // Y-axis grid lines and labels
    for (const tick of yTicks) {
        const y = chartArea.bottom - (tick / yMax) * (chartArea.bottom - chartArea.top);
        parts.push(`<line x1="${chartArea.left}" y1="${y}" ` +
            `x2="${chartArea.right}" y2="${y}" stroke="${kSvgGridColor}" stroke-width="1"/>`);
        parts.push(`<text x="${chartArea.left - 8}" y="${y + 4}" ` +
            `text-anchor="end" fill="${kSvgTextColor}">${tick}</text>`);
    }

    // Bars
    for (const bar of bars) {
        if (bar.height <= 0) continue;
        const fill = bar.negative ? kSvgNegativeFill : kSvgPositiveFill;
        const stroke = bar.negative ? kSvgNegativeBorder : kSvgPositiveBorder;
        parts.push(`<rect x="${bar.x.toFixed(1)}" y="${bar.y.toFixed(1)}" ` +
            `width="${bar.width.toFixed(1)}" height="${bar.height.toFixed(1)}" ` +
            `fill="${fill}" stroke="${stroke}" stroke-width="1">`);
        parts.push(`<title>${bar.count} endpoints [${bar.lower.toFixed(4)}, ${bar.upper.toFixed(4)}] ${unit}</title>`);
        parts.push(`</rect>`);
    }

    // X-axis labels (show a subset to avoid overlap)
    const maxLabels = Math.min(bars.length, 10);
    const step = Math.max(1, Math.floor(bars.length / maxLabels));
    for (let i = 0; i < bars.length; i += step) {
        const bar = bars[i];
        const x = bar.x + bar.width / 2;
        parts.push(`<text x="${x.toFixed(1)}" y="${chartArea.bottom + 15}" ` +
            `text-anchor="middle" fill="${kSvgTextColor}" font-size="10">` +
            `${bar.lower.toFixed(2)}</text>`);
    }
    // Always label the last bar's upper bound
    if (bars.length > 0) {
        const last = bars[bars.length - 1];
        parts.push(`<text x="${(last.x + last.width).toFixed(1)}" ` +
            `y="${chartArea.bottom + 15}" text-anchor="middle" fill="${kSvgTextColor}" ` +
            `font-size="10">${last.upper.toFixed(2)}</text>`);
    }

    // X-axis label
    parts.push(`<text x="${(chartArea.left + chartArea.right) / 2}" ` +
        `y="${height - 5}" text-anchor="middle" fill="${kSvgTextColor}" ` +
        `font-size="12">Slack (${escapeXml(unit)})</text>`);

    // Axes
    parts.push(`<line x1="${chartArea.left}" y1="${chartArea.top}" ` +
        `x2="${chartArea.left}" y2="${chartArea.bottom}" ` +
        `stroke="${kSvgAxisColor}" stroke-width="1"/>`);
    parts.push(`<line x1="${chartArea.left}" y1="${chartArea.bottom}" ` +
        `x2="${chartArea.right}" y2="${chartArea.bottom}" ` +
        `stroke="${kSvgAxisColor}" stroke-width="1"/>`);

    parts.push('</svg>');
    return parts.join('\n');
}
