// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Headless render preview for the web schematic widget.
//
// Renders the real src/schematic-widget.js in headless Chrome against a set of
// representative gate netlists and writes a PNG + SVG for each, so changes to
// the gate-symbol drawing can be eyeballed (and the rendered netlistsvg DOM
// inspected) instead of guessed at.
//
// Usage:
//   cd src/web/test/visual && npm install   # one-time
//   node preview.mjs [outDir] [netlist.json ...]
//
// Requires google-chrome (or set CHROME=/path/to/chrome).  elkjs and
// netlistsvg come from this directory's node_modules; nothing is fetched.

import puppeteer from 'puppeteer-core';
import { writeFileSync, readFileSync, mkdtempSync, mkdirSync } from 'node:fs';
import { dirname, join, basename } from 'node:path';
import { fileURLToPath } from 'node:url';
import { tmpdir } from 'node:os';
import http from 'node:http';

const here = dirname(fileURLToPath(import.meta.url));
const srcDir = join(here, '..', '..', 'src');
const chrome = process.env.CHROME || '/usr/bin/google-chrome';

// Args: any *.json is a real captured netlist to render; the first other arg is
// the output directory.  Capture a real netlist from the running viewer via
// DevTools -> Network -> WS -> the schematic_cone/schematic_full response frame
// (the JSON object with a top-level "modules" key) and save it to a file.
const jsonFiles = process.argv.slice(2).filter((a) => a.endsWith('.json'));
const outDir = process.argv.slice(2).find((a) => !a.endsWith('.json'))
  || mkdtempSync(join(tmpdir(), 'schematic-preview-'));
mkdirSync(outDir, { recursive: true });

// Build a single-cell netlist with external ports so the wires are drawn.
function netlist(type, gateKind, gateTerms, inPins, outPin) {
  let bit = 2;
  const ports = {}, pd = {}, conns = {}, netnames = {};
  for (const p of inPins) {
    ports[p] = { direction: 'input', bits: [bit] };
    pd[p] = 'input'; conns[p] = [bit];
    netnames[p] = { hide_name: 0, bits: [bit], attributes: {} };
    bit++;
  }
  ports[outPin] = { direction: 'output', bits: [bit] };
  pd[outPin] = 'output'; conns[outPin] = [bit];
  netnames[outPin] = { hide_name: 0, bits: [bit], attributes: {} };
  const cell = { hide_name: 0, type, port_directions: pd, connections: conns,
                 attributes: {}, parameters: {}, gate_kind: gateKind };
  if (gateTerms) cell.gate_terms = gateTerms;
  return { modules: { top: { ports, cells: { g1: cell }, netnames } } };
}

const CASES = jsonFiles.length
  ? jsonFiles.map((f) => [basename(f).replace(/\.json$/, ''),
                          JSON.parse(readFileSync(f, 'utf8'))])
  : [
    ['and2',  netlist('AND2_X1', 'and', null, ['A1', 'A2'], 'ZN')],
    ['or2',   netlist('OR2_X1', 'or', null, ['A1', 'A2'], 'ZN')],
    ['inv',   netlist('INV_X1', 'not', null, ['A'], 'ZN')],
    ['buf',   netlist('BUF_X1', 'buf', null, ['A'], 'Z')],
    ['nand2', netlist('NAND2_X1', 'nand', null, ['A1', 'A2'], 'ZN')],
    ['nor2',  netlist('NOR2_X1', 'nor', null, ['A1', 'A2'], 'ZN')],
    ['xor2',  netlist('XOR2_X1', 'xor', null, ['A', 'B'], 'Z')],
    ['aoi21', netlist('AOI21_X1', 'aoi', [['A'], ['B1', 'B2']], ['A', 'B1', 'B2'], 'ZN')],
    ['aoi22', netlist('AOI22_X1', 'aoi', [['A1', 'A2'], ['B1', 'B2']], ['A1', 'A2', 'B1', 'B2'], 'ZN')],
    ['oai21', netlist('OAI21_X1', 'oai', [['A'], ['B1', 'B2']], ['A', 'B1', 'B2'], 'ZN')],
  ];

const HARNESS = `<!doctype html><html><head><meta charset="utf-8">
<style>:root{--bg-panel:#fff;--fg-primary:#111;--bg-header:#eee;--border:#ccc;
  --bg-main:#fff;--fg-muted:#999;--accent:#e05a00;--fg-white:#fff;}
  body{margin:0;background:#fff}#host svg{color:#111}</style>
<script src="/vendor/elk.bundled.js"></script>
<script src="/vendor/netlistsvg.bundle.js"></script>
</head><body><div id="host" style="width:760px;height:460px"></div>
<script type="module">
  // schematic-widget.js reads the skin off the global, as vendor-globals.js
  // sets it in the app.
  const skin = await fetch('/openroad_skin.svg');
  if (!skin.ok) throw new Error('openroad_skin.svg: HTTP ' + skin.status);
  window.openroadSkin = await skin.text();
  const { SchematicWidget } = await import('/schematic-widget.js');
  const widget = new SchematicWidget({ element: document.getElementById('host') }, {});
  window.__render = async (json) => {
    for (let i = 0; i < 300 && !widget._netlistsvgReady; i++)
      await new Promise((r) => setTimeout(r, 50));
    if (!widget._netlistsvgReady) throw new Error('netlistsvg not ready');
    await widget.renderNetlist(json);
    await new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(r)));
    return widget._svgEl ? widget._svgEl.outerHTML : null;
  };
  window.__ready = true;
</script></body></html>`;

const MIME = {
  '.js': 'application/javascript',
  '.html': 'text/html',
  '.svg': 'image/svg+xml',
};

// The two libraries the widget reads off the global scope.
const VENDOR = {
  '/vendor/elk.bundled.js': 'elkjs/lib/elk.bundled.js',
  '/vendor/netlistsvg.bundle.js': 'netlistsvg/built/netlistsvg.bundle.js',
};
// npm does not apply ../../patches/netlistsvg@1.0.2.patch, which the viewer's
// bundle is built with, so the harness makes the same substitution.
const NETLISTSVG_PATCH = ['de.cau.cs.kieler.portConstraints',
                          'org.eclipse.elk.portConstraints'];
const server = http.createServer((req, res) => {
  const url = req.url.split('?')[0];
  if (url === '/preview.html' || url === '/') {
    res.setHeader('Content-Type', 'text/html'); res.end(HARNESS); return;
  }
  try {
    const ext = url.slice(url.lastIndexOf('.'));
    res.setHeader('Content-Type', MIME[ext] || 'text/plain');
    const file = VENDOR[url]
      ? join(here, 'node_modules', VENDOR[url])
      : join(srcDir, url);
    const body = readFileSync(file);
    res.end(url === '/vendor/netlistsvg.bundle.js'
      ? body.toString('utf8').replaceAll(...NETLISTSVG_PATCH)
      : body);
  } catch (e) { res.statusCode = 404; res.end('not found'); }
});
await new Promise((r) => server.listen(0, r));
const port = server.address().port;

const browser = await puppeteer.launch({
  executablePath: chrome, headless: 'new',
  args: ['--no-sandbox', '--disable-gpu'],
});
try {
  for (const [name, sample] of CASES) {
    const page = await browser.newPage();
    await page.setViewport({ width: 760, height: 460, deviceScaleFactor: 2 });
    const errs = [];
    page.on('pageerror', (e) => errs.push(e.message));
    await page.goto(`http://localhost:${port}/preview.html`, { waitUntil: 'load' });
    // A harness error (say, a missing skin) would otherwise surface as a timeout.
    await page.waitForFunction('window.__ready === true', { timeout: 20000 })
      .catch((e) => { throw new Error(errs.join('; ') || e.message); });
    const svg = await page.evaluate((j) => window.__render(j), sample);
    await new Promise((r) => setTimeout(r, 250));
    await page.screenshot({ path: join(outDir, `${name}.png`) });
    writeFileSync(join(outDir, `${name}.svg`), svg || '(no svg)');
    await page.close();
    console.log(`${name}: ${join(outDir, name + '.png')}${errs.length ? '  ERRORS: ' + errs.join('; ') : ''}`);
  }
  console.log(`\nWrote ${CASES.length} previews to ${outDir}`);
} finally {
  await browser.close();
  server.close();
}
