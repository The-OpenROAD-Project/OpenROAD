#!/usr/bin/env node
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Assemble a single self-contained HTML file from a JSON payload.
//
// All JS is bundled into one inline <script> by stripping ES module
// import/export and converting const/let to var. GoldenLayout is
// fetched from esm.sh at build time and inlined as an IIFE.
//
// The result works from file:// with zero server.
//
// Usage:
//   node render-static-page.js payload.json -o report.html

import { readFileSync, writeFileSync, existsSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { execSync } from 'node:child_process';

const __dirname = dirname(fileURLToPath(import.meta.url));

function parseArgs(argv) {
    const args = argv.slice(2);
    let input = null;
    let output = null;
    for (let i = 0; i < args.length; i++) {
        if (args[i] === '-o' || args[i] === '--output') output = args[++i];
        else if (args[i] !== '-h' && args[i] !== '--help') input = args[i];
        else { console.log('Usage: render-static-page.js payload.json -o report.html'); process.exit(0); }
    }
    if (!input) { console.error('Error: no JSON payload file specified'); process.exit(1); }
    if (!output) { console.error('Error: -o report.html is required'); process.exit(1); }
    return { input, output };
}

// Strip ES module syntax and convert const/let to var (prevents
// duplicate declaration errors when files are concatenated).
function stripModuleSyntax(src) {
    return src
        .split('\n')
        .map(line => {
            const t = line.trimStart();
            if (t.startsWith('import ')) return '// ' + line;
            if (t.startsWith('export function ')) return line.replace('export function ', 'function ');
            if (t.startsWith('export class ')) return line.replace('export class ', 'class ');
            if (t.startsWith('export const ')) return line.replace('export const ', 'var ');
            if (t.startsWith('export let ')) return line.replace('export let ', 'var ');
            if (t.startsWith('export default ')) return line.replace('export default ', '');
            if (t.startsWith('export {')) return '// ' + line;
            return line;
        })
        .join('\n')
        .replace(/^(\s*)const /gm, '$1var ')
        .replace(/^(\s*)let /gm, '$1var ');
}

// Fetch GoldenLayout ESM bundle from esm.sh and convert to IIFE.
const GL_CACHE = '/tmp/golden-layout-2.6.0-iife.js';
const GL_URL = 'https://esm.sh/golden-layout@2.6.0/es2022/golden-layout.bundle.mjs';

function getGoldenLayoutBundle() {
    if (existsSync(GL_CACHE)) return readFileSync(GL_CACHE, 'utf-8');
    console.log('Downloading GoldenLayout bundle...');
    const esm = execSync(`curl -sL "${GL_URL}"`, { encoding: 'utf-8', maxBuffer: 1024 * 1024 });
    const exportMatch = esm.match(/export\{([^}]+)\}/);
    const stripped = esm.replace(/export\{[^}]+\}/, '');
    let glName = 'GoldenLayout', lcName = 'LayoutConfig';
    if (exportMatch) {
        const m1 = exportMatch[1].match(/([\w$]+)\s+as\s+GoldenLayout/);
        const m2 = exportMatch[1].match(/([\w$]+)\s+as\s+LayoutConfig/);
        if (m1) glName = m1[1];
        if (m2) lcName = m2[1];
    }
    // Patch the loadLayout method to auto-init if needed.
    // The IIFE build's loadLayout requires _isInitialised (set by init()).
    // We replicate the essential init() steps: create _groundItem, observe
    // resize, set flag. Then let loadLayout proceed normally.
    const iife = `(function(){\n${stripped}\n` +
        `var _origLoad = ${glName}.prototype.loadLayout;\n` +
        `${glName}.prototype.loadLayout = function(c) {\n` +
        `  if (!this._isInitialised) {\n` +
        `    var rc = ${lcName}.resolve(c);\n` +
        `    this.layoutConfig = rc;\n` +
        `    this._groundItem = new ie(this, rc.root, this._containerElement);\n` +
        `    this._groundItem.init();\n` +
        `    this._resizeObserver.observe(this._containerElement);\n` +
        `    this._isInitialised = true;\n` +
        `    this.adjustColumnsResponsive();\n` +
        `    this.emit("initialised");\n` +
        `    this._groundItem.loadRoot(rc.root);\n` +
        `    this.checkLoadedLayoutMaximiseItem();\n` +
        `    this.adjustColumnsResponsive();\n` +
        `    this.updateSizeFromContainer();\n` +
        `    return;\n` +
        `  }\n` +
        `  return _origLoad.call(this, c);\n` +
        `};\n` +
        `window.GoldenLayout = ${glName};\nwindow.LayoutConfig = ${lcName};\n})();`;
    writeFileSync(GL_CACHE, iife);
    return iife;
}

// --- Main ---

const { input, output } = parseArgs(process.argv);
const payloadJson = readFileSync(input, 'utf-8');
JSON.parse(payloadJson);

const css = readFileSync(join(__dirname, 'style.css'), 'utf-8');
const glBundle = getGoldenLayoutBundle();

const loadOrder = [
    'theme.js', 'static-data-manager.js', 'coordinates.js', 'ui-utils.js',
    'vis-tree.js', 'checkbox-tree-model.js', 'websocket-manager.js',
    'websocket-tile-layer.js', 'tcl-completer.js', 'inspector.js',
    'display-controls.js', 'menu-bar.js', 'ruler.js', 'charts-widget.js',
    'histogram-svg.js', 'timing-widget.js', 'timing-table-html.js',
    'clock-tree-widget.js', 'hierarchy-browser.js', 'schematic-widget.js',
    'main.js',
];

const jsChunks = [];
for (const name of loadOrder) {
    try {
        const src = readFileSync(join(__dirname, name), 'utf-8');
        jsChunks.push(`// ─── ${name} ───\n` + stripModuleSyntax(src));
    } catch { /* optional file */ }
}
const bundledJs = jsChunks.join('\n\n');

const html = `<!DOCTYPE html>
<html>
<head>
    <title>OpenROAD Static Viewer</title>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css"
          integrity="sha256-p4NxAoJBhIIN+hmNHrzRCf9tD/miZyoHS5obTRR9BMY=" crossorigin=""/>
    <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"
            integrity="sha256-20nQCchB9co0qIjJZRGuk2/Z9VM+kNiyxNV1lvTlZBo=" crossorigin=""></script>
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/golden-layout@2.6.0/dist/css/goldenlayout-base.css"/>
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/golden-layout@2.6.0/dist/css/themes/goldenlayout-dark-theme.css"/>
    <script src="https://nturley.github.io/netlistsvg/elk.bundled.js"></script>
    <script src="https://nturley.github.io/netlistsvg/built/netlistsvg.bundle.js"></script>
    <style>\n${css}\n
/* Fix GoldenLayout content sizing in static IIFE mode */
.lm_content { position: absolute !important; top: 0; left: 0; right: 0; bottom: 0; }
    </style>
</head>
<body>
    <div id="menu-bar"></div>
    <div id="gl-container"></div>
    <div id="websocket-status"></div>
    <div id="loading-overlay" style="display:none">
        <div class="loading-overlay-content">
            <div class="spinner"></div>
            <span>Loading shapes…</span>
        </div>
    </div>
    <script type="application/json" data-static>\n${payloadJson}\n    </script>
    <script>\n${glBundle}\n    </script>
    <script>
// Static HTML viewer: clear any stale saved layout from live viewer sessions
localStorage.removeItem('gl-layout');
localStorage.removeItem('gl-layout-version');
(function() {
    var GoldenLayout = window.GoldenLayout;
    var LayoutConfig = window.LayoutConfig;
${bundledJs}
})();
    </script>
</body>
</html>`;

writeFileSync(output, html);
console.log(`Wrote ${output} (${(html.length / 1024).toFixed(0)} KB)`);
