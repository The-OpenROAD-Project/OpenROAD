#!/usr/bin/env node
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Static HTML assembler for OpenROAD timing reports.
//
// Takes one or more JSON payloads (from web_export_json) and produces
// a self-contained HTML file with embedded SVG histograms and timing tables.
//
// Usage:
//   node render-static.js --label route route.json -o report.html
//   node render-static.js --label place place.json --label route route.json -o stages.html

import { readFileSync, writeFileSync } from 'node:fs';
import { renderHistogramSVG } from './histogram-svg.js';
import { renderTimingTableHTML } from './timing-table-html.js';

function parseArgs(argv) {
    const args = argv.slice(2);
    const payloads = [];
    let output = null;
    let currentLabel = null;

    for (let i = 0; i < args.length; i++) {
        if (args[i] === '--label' || args[i] === '-l') {
            currentLabel = args[++i];
        } else if (args[i] === '-o' || args[i] === '--output') {
            output = args[++i];
        } else if (args[i] === '-h' || args[i] === '--help') {
            console.log('Usage: render-static.js [--label NAME] file.json [...] -o output.html');
            process.exit(0);
        } else {
            // JSON file
            const file = args[i];
            const data = JSON.parse(readFileSync(file, 'utf-8'));
            const label = currentLabel
                || data.metadata?.stage
                || data.metadata?.design
                || file.replace(/.*\//, '').replace(/\.json$/, '');
            payloads.push({ label, data, file });
            currentLabel = null;
        }
    }

    if (payloads.length === 0) {
        console.error('Error: no JSON payload files specified');
        process.exit(1);
    }
    if (!output) {
        console.error('Error: -o output.html is required');
        process.exit(1);
    }

    return { payloads, output };
}

function generateHTML(payloads) {
    const multi = payloads.length > 1;
    const parts = [];

    parts.push(`<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>OpenROAD Timing Report</title>
<style>
* { box-sizing: border-box; margin: 0; padding: 0; }
body { background: #1e1e1e; color: #ccc; font-family: monospace; font-size: 13px; padding: 16px; }
h1 { font-size: 16px; margin-bottom: 12px; color: #fff; }
.tabs { display: flex; gap: 4px; margin-bottom: 16px; flex-wrap: wrap; }
.tab { padding: 6px 14px; background: #333; border: 1px solid #555; border-radius: 4px 4px 0 0;
       cursor: pointer; color: #aaa; font-family: monospace; font-size: 12px; }
.tab.active { background: #1e1e1e; border-bottom-color: #1e1e1e; color: #fff; font-weight: bold; }
.tab:hover { color: #fff; }
.payload { display: none; }
.payload.active { display: block; }
.section { margin-bottom: 24px; }
.section h2 { font-size: 14px; margin-bottom: 8px; color: #ddd; }
svg { max-width: 100%; height: auto; }
.timing-table { border-collapse: collapse; width: 100%; font-size: 12px; }
.timing-table th { background: #2a2a2a; padding: 6px 8px; text-align: left;
                   border-bottom: 2px solid #444; position: sticky; top: 0; }
.timing-table td { padding: 4px 8px; border-bottom: 1px solid #333; white-space: nowrap; }
.timing-table tr:hover { background: #2a2a2a; }
.timing-table .negative { color: #f08080; }
.meta { color: #888; font-size: 11px; margin-bottom: 16px; }
</style>
</head>
<body>
<h1>OpenROAD Timing Report</h1>`);

    // Embed all payloads as JSON
    for (let i = 0; i < payloads.length; i++) {
        parts.push(`<script type="application/json" data-payload="${i}" ` +
            `data-label="${payloads[i].label}">${JSON.stringify(payloads[i].data)}</script>`);
    }

    // Tab bar (only if multiple payloads)
    if (multi) {
        parts.push('<div class="tabs" id="tabs">');
        for (let i = 0; i < payloads.length; i++) {
            const cls = i === 0 ? ' active' : '';
            parts.push(`<div class="tab${cls}" data-index="${i}">${payloads[i].label}</div>`);
        }
        parts.push('</div>');
    }

    // Pre-render each payload's content
    for (let i = 0; i < payloads.length; i++) {
        const p = payloads[i].data;
        const cls = i === 0 ? ' active' : '';
        parts.push(`<div class="payload${cls}" data-index="${i}">`);

        // Metadata
        const meta = p.metadata || {};
        const metaParts = [];
        if (meta.design) metaParts.push(`Design: ${meta.design}`);
        if (meta.stage) metaParts.push(`Stage: ${meta.stage}`);
        if (meta.variant) metaParts.push(`Variant: ${meta.variant}`);
        if (meta.platform) metaParts.push(`PDK: ${meta.platform}`);
        if (meta.timestamp) metaParts.push(`Time: ${meta.timestamp}`);
        if (metaParts.length > 0) {
            parts.push(`<div class="meta">${metaParts.join(' | ')}</div>`);
        }

        // Histogram SVG (support both old and new key names)
        const histData = p.slack_histogram || p.histogram;
        if (histData) {
            parts.push('<div class="section">');
            parts.push('<h2>Endpoint Slack Histogram</h2>');
            parts.push(renderHistogramSVG(histData, 800, 350));
            parts.push('</div>');
        }

        // Timing paths table (support both old and new key names)
        const timingData = p.timing_report_setup || p.timing_paths;
        if (timingData) {
            parts.push('<div class="section">');
            parts.push('<h2>Timing Paths</h2>');
            parts.push(renderTimingTableHTML(timingData));
            parts.push('</div>');
        }

        parts.push('</div>');
    }

    // Tab switching JS (only if multiple payloads)
    if (multi) {
        parts.push(`<script>
document.getElementById('tabs').addEventListener('click', function(e) {
    const tab = e.target.closest('.tab');
    if (!tab) return;
    const idx = tab.dataset.index;
    document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
    document.querySelectorAll('.payload').forEach(p => p.classList.remove('active'));
    tab.classList.add('active');
    document.querySelector('.payload[data-index="' + idx + '"]').classList.add('active');
});
</script>`);
    }

    parts.push('</body></html>');
    return parts.join('\n');
}

const { payloads, output } = parseArgs(process.argv);
const html = generateHTML(payloads);
writeFileSync(output, html);
console.log(`Wrote ${output} (${payloads.length} payload${payloads.length > 1 ? 's' : ''}, ${html.length} bytes)`);
