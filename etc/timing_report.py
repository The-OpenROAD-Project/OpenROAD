# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors
#
# Generates two timing report artifacts from a post-synth ODB:
#   1_timing.html — full interactive GUI replacement (self-contained)
#   1_timing.md   — PR comment (max info, min friction, <65KB)
#
# Usage:
#   ODB_FILE=... SDC_FILE=... REPORTS_DIR=... openroad -python synth_timing_report.py
#   (or via Bazel py_binary with standalone Python)

from __future__ import annotations

import json
import math
import os
import sys

from openroad import Design, Tech, Timing


def extract_timing_data(timing: Timing, design_name: str,
                        platform: str) -> dict:
    """Extract all timing data using the Timing Python API."""
    setup_wns = timing.getWorstSlack(Timing.Max)
    setup_tns = timing.getTotalNegativeSlack(Timing.Max)
    hold_wns = timing.getWorstSlack(Timing.Min)
    hold_tns = timing.getTotalNegativeSlack(Timing.Min)
    endpoint_count = timing.getEndpointCount()

    # Endpoint slack map for histogram
    endpoint_slacks = []
    for name, slack in timing.getEndpointSlackMap(Timing.Max):
        endpoint_slacks.append({"name": name, "slack": round(slack, 6)})

    # Clock domains
    clocks = []
    for clk in timing.getClockInfo():
        clocks.append({
            "name": clk.name,
            "period": round(clk.period, 4),
            "waveform": [round(w, 4) for w in clk.waveform],
            "sources": list(clk.sources),
        })

    # Timing paths — get as many as available
    setup_paths = []
    for p in timing.getTimingPaths(Timing.Max, 1000):
        arcs = []
        for a in p.arcs:
            arcs.append({
                "from": a.from_pin, "to": a.to_pin,
                "cell": a.cell_name,
                "delay": round(a.delay, 6), "slew": round(a.slew, 6),
                "load": round(a.load, 6), "fanout": a.fanout,
                "rising": a.is_rising, "net": a.is_net,
            })
        setup_paths.append({
            "slack": round(p.slack, 6),
            "path_delay": round(p.path_delay, 6),
            "arrival": round(p.arrival, 6),
            "required": round(p.required, 6),
            "skew": round(p.skew, 6),
            "logic_delay": round(p.logic_delay, 6),
            "logic_depth": p.logic_depth,
            "fanout": p.fanout,
            "start": p.startpoint, "end": p.endpoint,
            "start_clk": p.start_clock, "end_clk": p.end_clock,
            "group": p.path_group,
            "arcs": arcs,
        })

    return {
        "design": design_name,
        "platform": platform,
        "setup_wns": round(setup_wns, 6),
        "setup_tns": round(setup_tns, 6),
        "hold_wns": round(hold_wns, 6),
        "hold_tns": round(hold_tns, 6),
        "endpoint_count": endpoint_count,
        "endpoints": endpoint_slacks,
        "clocks": clocks,
        "paths": setup_paths,
    }


# ─── HTML generation ─────────────────────────────────────────────────────────

TIMING_HTML_TEMPLATE = r"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Timing Report — {design}</title>
<style>
*{{box-sizing:border-box;margin:0;padding:0}}
body{{background:#0d1117;color:#e6edf3;font-family:system-ui,-apple-system,sans-serif;font-size:13px}}
.hdr{{background:#161b22;border-bottom:1px solid #30363d;padding:12px 20px;display:flex;align-items:center;gap:16px;flex-wrap:wrap}}
.hdr h1{{font-size:16px;font-weight:600}}
.badge{{display:inline-block;padding:2px 8px;border-radius:12px;font-weight:600;font-size:12px}}
.badge.pass{{background:#238636;color:#fff}}
.badge.fail{{background:#da3633;color:#fff}}
.metric{{font-size:12px;color:#8b949e}}
.metric b{{color:#e6edf3}}
.panels{{display:grid;grid-template-columns:1fr;gap:1px;background:#30363d}}
.panel{{background:#0d1117;padding:16px}}
.panel h2{{font-size:14px;margin-bottom:10px;color:#58a6ff}}
#histogram-wrap{{position:relative}}
canvas{{display:block;width:100%;cursor:crosshair}}
.tooltip{{position:absolute;background:#1c2128;border:1px solid #30363d;padding:6px 10px;border-radius:6px;font-size:12px;pointer-events:none;display:none;z-index:10}}
.controls{{display:flex;gap:8px;margin-bottom:8px;flex-wrap:wrap}}
.controls button,.controls select{{background:#21262d;color:#e6edf3;border:1px solid #30363d;padding:4px 10px;border-radius:6px;cursor:pointer;font-size:12px}}
.controls button:hover,.controls select:hover{{background:#30363d}}
.controls button.active{{background:#1f6feb;border-color:#1f6feb}}
table{{width:100%;border-collapse:collapse}}
th,td{{text-align:left;padding:6px 8px;border-bottom:1px solid #21262d;white-space:nowrap}}
th{{background:#161b22;color:#8b949e;font-weight:500;cursor:pointer;user-select:none;position:sticky;top:0;z-index:5}}
th:hover{{color:#e6edf3}}
th.sorted-asc::after{{content:" ▲"}}
th.sorted-desc::after{{content:" ▼"}}
.path-table-wrap{{max-height:400px;overflow-y:auto;border:1px solid #21262d;border-radius:6px}}
tr:hover{{background:#161b22}}
tr.selected{{background:#1c2128}}
.slack-neg{{color:#f85149}}
.slack-pos{{color:#3fb950}}
.arc-wrap{{background:#161b22;border:1px solid #30363d;border-radius:6px;margin:8px 0;padding:12px;display:none}}
.arc-bar{{height:20px;display:inline-block;vertical-align:middle;border-radius:2px;position:relative;cursor:pointer}}
.arc-bar.cell{{background:#58a6ff}}
.arc-bar.net{{background:#d2a8ff}}
.arc-bar.critical{{background:#f85149}}
.arc-waterfall{{margin:8px 0;overflow-x:auto}}
.arc-table td{{font-size:12px}}
.arc-table .rise{{color:#3fb950}}
.arc-table .fall{{color:#f85149}}
.clk-table td{{font-size:12px}}
.cell-chart{{display:flex;gap:2px;align-items:flex-end;height:60px;margin-top:8px}}
.cell-bar{{background:#58a6ff;min-width:4px;border-radius:2px 2px 0 0;cursor:pointer;position:relative}}
.cell-bar:hover{{opacity:0.8}}
#status-bar{{background:#161b22;border-top:1px solid #30363d;padding:6px 20px;font-size:11px;color:#8b949e;position:fixed;bottom:0;left:0;right:0}}
</style>
</head>
<body>
<div class="hdr">
  <h1>{design}</h1>
  <span class="badge {wns_class}">{wns_label}</span>
  <span class="metric">WNS <b>{setup_wns} ns</b></span>
  <span class="metric">TNS <b>{setup_tns} ns</b></span>
  <span class="metric">Hold WNS <b>{hold_wns} ns</b></span>
  <span class="metric">Endpoints <b>{endpoint_count}</b></span>
  <span class="metric" style="margin-left:auto">{platform}</span>
</div>
<div class="panels">
  <div class="panel">
    <h2>Endpoint Slack Histogram</h2>
    <div class="controls">
      <button id="btn-setup" class="active" onclick="setMode('setup')">Setup</button>
      <button id="btn-hold" onclick="setMode('hold')">Hold</button>
    </div>
    <div id="histogram-wrap">
      <canvas id="hist-canvas" height="160"></canvas>
      <div class="tooltip" id="hist-tooltip"></div>
    </div>
  </div>
  <div class="panel">
    <h2>Clock Domains</h2>
    <table class="clk-table"><thead><tr><th>Clock</th><th>Period (ns)</th><th>Waveform</th><th>Sources</th></tr></thead>
    <tbody id="clk-body"></tbody></table>
  </div>
  <div class="panel">
    <h2>Timing Paths</h2>
    <div class="controls">
      <select id="group-filter" onchange="filterPaths()"><option value="">All Groups</option></select>
    </div>
    <div class="path-table-wrap" id="path-table-wrap">
      <table id="path-table">
        <thead><tr>
          <th data-col="slack">Slack</th>
          <th data-col="start">Start</th>
          <th data-col="end">End</th>
          <th data-col="end_clk">Clock</th>
          <th data-col="arrival">Arrival</th>
          <th data-col="required">Required</th>
          <th data-col="path_delay">Path Delay</th>
          <th data-col="logic_depth">Depth</th>
          <th data-col="fanout">Fanout</th>
        </tr></thead>
        <tbody id="path-body"></tbody>
      </table>
    </div>
    <div class="arc-wrap" id="arc-detail">
      <h2 style="margin-bottom:8px">Path Detail</h2>
      <div class="arc-waterfall" id="arc-waterfall"></div>
      <table class="arc-table" id="arc-table">
        <thead><tr><th>Pin</th><th>Edge</th><th>Delay</th><th>Slew</th><th>Load</th><th>Fanout</th><th>Cell</th></tr></thead>
        <tbody id="arc-body"></tbody>
      </table>
    </div>
  </div>
  <div class="panel">
    <h2>Cell Type Breakdown</h2>
    <div class="cell-chart" id="cell-chart"></div>
    <table id="cell-table" style="margin-top:8px">
      <thead><tr><th>Cell</th><th>Count</th><th>Total Delay (ns)</th><th>Avg Slew (ns)</th></tr></thead>
      <tbody id="cell-body"></tbody>
    </table>
  </div>
</div>
<div id="status-bar">Loaded</div>

<script>
const D = {json_data};

// ─── Histogram ─────────────────────────────────────────────────
let histMode = 'setup';
function setMode(m) {{
  histMode = m;
  document.getElementById('btn-setup').classList.toggle('active', m==='setup');
  document.getElementById('btn-hold').classList.toggle('active', m==='hold');
  drawHistogram();
}}

function snapInterval(exact) {{
  const exp = Math.floor(Math.log10(exact));
  const mag = Math.pow(10, exp);
  const res = exact / mag;
  let nice;
  if (res < 1.5) nice = 1;
  else if (res < 3) nice = 2;
  else if (res < 7) nice = 5;
  else nice = 10;
  return nice * mag;
}}

let histBins = [];
function drawHistogram() {{
  const canvas = document.getElementById('hist-canvas');
  const ctx = canvas.getContext('2d');
  const dpr = window.devicePixelRatio || 1;
  const rect = canvas.getBoundingClientRect();
  canvas.width = rect.width * dpr;
  canvas.height = rect.height * dpr;
  ctx.scale(dpr, dpr);
  const W = rect.width, H = rect.height;
  ctx.clearRect(0, 0, W, H);

  const slacks = D.endpoints.map(e => e.slack);
  if (slacks.length === 0) return;

  const lo = Math.min(0, Math.min(...slacks));
  const hi = Math.max(0, Math.max(...slacks));
  if (lo === hi) return;

  const interval = snapInterval((hi - lo) / 20);
  const binMin = Math.floor(lo / interval) * interval;
  const binMax = Math.ceil(hi / interval) * interval;
  const nBins = Math.round((binMax - binMin) / interval);

  histBins = [];
  for (let i = 0; i < nBins; i++) {{
    histBins.push({{lo: binMin + i * interval, hi: binMin + (i+1) * interval, count: 0, endpoints: []}});
  }}
  for (const e of D.endpoints) {{
    const idx = Math.min(Math.floor((e.slack - binMin) / interval), nBins - 1);
    if (idx >= 0 && idx < nBins) {{
      histBins[idx].count++;
      histBins[idx].endpoints.push(e.name);
    }}
  }}

  const maxCount = Math.max(...histBins.map(b => b.count), 1);
  const pad = {{l: 50, r: 10, t: 10, b: 30}};
  const plotW = W - pad.l - pad.r;
  const plotH = H - pad.t - pad.b;
  const barW = plotW / nBins;

  // Bars
  for (let i = 0; i < nBins; i++) {{
    const b = histBins[i];
    const bh = (b.count / maxCount) * plotH;
    const x = pad.l + i * barW;
    const y = pad.t + plotH - bh;
    const mid = (b.lo + b.hi) / 2;
    ctx.fillStyle = mid < 0 ? '#f08080' : '#90ee90';
    ctx.fillRect(x + 1, y, barW - 2, bh);
  }}

  // Zero line
  const zeroX = pad.l + ((0 - binMin) / (binMax - binMin)) * plotW;
  if (zeroX > pad.l && zeroX < W - pad.r) {{
    ctx.setLineDash([4, 3]);
    ctx.strokeStyle = '#8b949e';
    ctx.lineWidth = 1;
    ctx.beginPath(); ctx.moveTo(zeroX, pad.t); ctx.lineTo(zeroX, pad.t + plotH); ctx.stroke();
    ctx.setLineDash([]);
  }}

  // Axes
  ctx.fillStyle = '#8b949e';
  ctx.font = '10px system-ui';
  ctx.textAlign = 'center';
  for (let i = 0; i <= nBins; i += Math.max(1, Math.floor(nBins / 8))) {{
    const val = binMin + i * interval;
    ctx.fillText(val.toFixed(2), pad.l + i * barW, H - 6);
  }}
  ctx.textAlign = 'right';
  for (let i = 0; i <= 4; i++) {{
    const val = Math.round(maxCount * i / 4);
    const y = pad.t + plotH - (i / 4) * plotH;
    ctx.fillText(val.toString(), pad.l - 4, y + 4);
  }}
}}

// Histogram hover/click
const histCanvas = document.getElementById('hist-canvas');
const histTooltip = document.getElementById('hist-tooltip');
histCanvas.addEventListener('mousemove', function(e) {{
  const rect = histCanvas.getBoundingClientRect();
  const x = e.clientX - rect.left;
  const pad = {{l: 50, r: 10}};
  const plotW = rect.width - pad.l - pad.r;
  const idx = Math.floor((x - pad.l) / (plotW / histBins.length));
  if (idx >= 0 && idx < histBins.length) {{
    const b = histBins[idx];
    histTooltip.style.display = 'block';
    histTooltip.style.left = (e.clientX - rect.left + 10) + 'px';
    histTooltip.style.top = (e.clientY - rect.top - 30) + 'px';
    histTooltip.innerHTML = `${{b.count}} endpoints<br>${{b.lo.toFixed(3)}} to ${{b.hi.toFixed(3)}} ns`;
  }} else {{
    histTooltip.style.display = 'none';
  }}
}});
histCanvas.addEventListener('mouseleave', () => histTooltip.style.display = 'none');
histCanvas.addEventListener('click', function(e) {{
  const rect = histCanvas.getBoundingClientRect();
  const x = e.clientX - rect.left;
  const pad = {{l: 50, r: 10}};
  const plotW = rect.width - pad.l - pad.r;
  const idx = Math.floor((x - pad.l) / (plotW / histBins.length));
  if (idx >= 0 && idx < histBins.length) {{
    const b = histBins[idx];
    slackFilter = [b.lo, b.hi];
    filterPaths();
  }}
}});

// ─── Clock table ────────────────────────────────────────────────
function renderClocks() {{
  const body = document.getElementById('clk-body');
  body.innerHTML = D.clocks.map(c =>
    `<tr><td>${{c.name}}</td><td>${{c.period}}</td><td>${{c.waveform.join(', ')}}</td><td>${{c.sources.join(', ')}}</td></tr>`
  ).join('');
}}

// ─── Path table ─────────────────────────────────────────────────
let sortCol = 'slack', sortAsc = true;
let slackFilter = null;
let filteredPaths = D.paths;

function filterPaths() {{
  const group = document.getElementById('group-filter').value;
  filteredPaths = D.paths.filter(p => {{
    if (group && p.group !== group) return false;
    if (slackFilter && (p.slack < slackFilter[0] || p.slack >= slackFilter[1])) return false;
    return true;
  }});
  sortPaths();
}}

function sortPaths() {{
  filteredPaths.sort((a, b) => {{
    let va = a[sortCol], vb = b[sortCol];
    if (typeof va === 'string') return sortAsc ? va.localeCompare(vb) : vb.localeCompare(va);
    return sortAsc ? va - vb : vb - va;
  }});
  renderPaths();
}}

function renderPaths() {{
  const body = document.getElementById('path-body');
  // Virtualised: show up to 500 rows
  const shown = filteredPaths.slice(0, 500);
  body.innerHTML = shown.map((p, i) => {{
    const cls = p.slack < 0 ? 'slack-neg' : 'slack-pos';
    return `<tr onclick="showArcDetail(${{i}})" data-idx="${{i}}">
      <td class="${{cls}}">${{p.slack.toFixed(4)}}</td>
      <td title="${{p.start}}">${{p.start.slice(-35)}}</td>
      <td title="${{p.end}}">${{p.end.slice(-35)}}</td>
      <td>${{p.end_clk}}</td>
      <td>${{p.arrival.toFixed(4)}}</td>
      <td>${{p.required.toFixed(4)}}</td>
      <td>${{p.path_delay.toFixed(4)}}</td>
      <td>${{p.logic_depth}}</td>
      <td>${{p.fanout}}</td>
    </tr>`;
  }}).join('');
  document.getElementById('status-bar').textContent =
    `Showing ${{shown.length}} of ${{filteredPaths.length}} paths (${{D.endpoint_count}} endpoints)`;
}}

// Column header sort
document.querySelectorAll('#path-table th').forEach(th => {{
  th.addEventListener('click', () => {{
    const col = th.dataset.col;
    if (sortCol === col) sortAsc = !sortAsc;
    else {{ sortCol = col; sortAsc = true; }}
    document.querySelectorAll('#path-table th').forEach(t => t.className = '');
    th.className = sortAsc ? 'sorted-asc' : 'sorted-desc';
    sortPaths();
  }});
}});

// ─── Arc detail ──────────────────────────────────────────────────
function showArcDetail(idx) {{
  const p = filteredPaths[idx];
  const wrap = document.getElementById('arc-detail');
  wrap.style.display = 'block';

  // Waterfall bar
  const totalDelay = p.arcs.reduce((s, a) => s + Math.abs(a.delay), 0) || 1;
  const wf = document.getElementById('arc-waterfall');
  wf.innerHTML = p.arcs.map(a => {{
    const w = Math.max(2, (Math.abs(a.delay) / totalDelay) * 100);
    const cls = a.net ? 'net' : (a.delay > totalDelay * 0.15 ? 'critical' : 'cell');
    return `<span class="arc-bar ${{cls}}" style="width:${{w}}%"
      title="${{a.from}} → ${{a.to}}\n${{a.cell}} delay=${{a.delay.toFixed(4)}} slew=${{a.slew.toFixed(4)}}"></span>`;
  }}).join('');

  // Arc table
  const body = document.getElementById('arc-body');
  body.innerHTML = p.arcs.map(a => {{
    const edge = a.rising ? '<span class="rise">↑</span>' : '<span class="fall">↓</span>';
    return `<tr>
      <td title="${{a.to}}">${{a.to.slice(-40)}}</td>
      <td>${{edge}}</td>
      <td>${{a.delay.toFixed(4)}}</td>
      <td>${{a.slew.toFixed(4)}}</td>
      <td>${{a.load.toFixed(4)}}</td>
      <td>${{a.fanout}}</td>
      <td>${{a.cell}}</td>
    </tr>`;
  }}).join('');

  // Highlight selected row
  document.querySelectorAll('#path-body tr').forEach(r => r.classList.remove('selected'));
  document.querySelector(`#path-body tr[data-idx="${{idx}}"]`)?.classList.add('selected');
}}

// ─── Cell breakdown ──────────────────────────────────────────────
function renderCellBreakdown() {{
  const cells = {{}};
  for (const p of D.paths) {{
    for (const a of p.arcs) {{
      if (a.net) continue;
      if (!cells[a.cell]) cells[a.cell] = {{count: 0, delay: 0, slews: []}};
      cells[a.cell].count++;
      cells[a.cell].delay += Math.abs(a.delay);
      cells[a.cell].slews.push(a.slew);
    }}
  }}
  const sorted = Object.entries(cells).sort((a, b) => b[1].delay - a[1].delay).slice(0, 20);
  if (sorted.length === 0) return;

  const maxDelay = sorted[0][1].delay || 1;
  const chart = document.getElementById('cell-chart');
  chart.innerHTML = sorted.map(([name, v]) => {{
    const h = Math.max(4, (v.delay / maxDelay) * 60);
    return `<div class="cell-bar" style="height:${{h}}px;flex:1" title="${{name}}: ${{v.delay.toFixed(3)}} ns (${{v.count}})"></div>`;
  }}).join('');

  const body = document.getElementById('cell-body');
  body.innerHTML = sorted.map(([name, v]) => {{
    const avgSlew = v.slews.reduce((s, x) => s + x, 0) / v.slews.length;
    return `<tr><td>${{name}}</td><td>${{v.count}}</td><td>${{v.delay.toFixed(4)}}</td><td>${{avgSlew.toFixed(4)}}</td></tr>`;
  }}).join('');
}}

// ─── Group filter ────────────────────────────────────────────────
function initGroupFilter() {{
  const groups = [...new Set(D.paths.map(p => p.group))].filter(Boolean);
  const sel = document.getElementById('group-filter');
  groups.forEach(g => {{
    const opt = document.createElement('option');
    opt.value = g; opt.textContent = g;
    sel.appendChild(opt);
  }});
}}

// ─── Init ────────────────────────────────────────────────────────
renderClocks();
initGroupFilter();
filterPaths();
drawHistogram();
renderCellBreakdown();
window.addEventListener('resize', drawHistogram);
</script>
</body>
</html>"""


def generate_html(data: dict, path: str):
    """Generate self-contained interactive HTML timing report."""
    json_str = json.dumps(data, separators=(",", ":"))
    wns = data["setup_wns"]
    html = TIMING_HTML_TEMPLATE.format(
        design=data["design"],
        platform=data["platform"],
        setup_wns=f"{wns:.4f}",
        setup_tns=f"{data['setup_tns']:.4f}",
        hold_wns=f"{data['hold_wns']:.4f}",
        endpoint_count=data["endpoint_count"],
        wns_class="pass" if wns >= 0 else "fail",
        wns_label="PASS" if wns >= 0 else "FAIL",
        json_data=json_str,
    )
    with open(path, "w") as f:
        f.write(html)
    size_kb = os.path.getsize(path) / 1024
    print(f"[timing_report] wrote {path} ({size_kb:.0f} KB)")


# ─── Markdown generation ─────────────────────────────────────────────────────

def _ascii_histogram(endpoints: list[dict], width: int = 40) -> str:
    """Render a compact ASCII histogram for markdown."""
    slacks = [e["slack"] for e in endpoints]
    if not slacks:
        return ""
    lo = min(min(slacks), 0)
    hi = max(max(slacks), 0)
    if lo == hi:
        return f"    All {len(slacks)} endpoints at {lo:.3f} ns"

    blocks = " ▁▂▃▄▅▆▇█"
    nbins = min(width, 40)
    bin_w = (hi - lo) / nbins
    counts = [0] * nbins
    for s in slacks:
        idx = min(int((s - lo) / bin_w), nbins - 1)
        counts[idx] += 1
    mx = max(counts) or 1
    bar = "".join(blocks[min(int(c / mx * 8), 8)] for c in counts)
    return (f"```\n"
            f"Slack distribution (setup):\n"
            f"    {bar}   {len(slacks)} endpoints\n"
            f"    {lo:.3f}{' ' * (len(bar) - 12)}{hi:.3f} ns\n"
            f"```")


def generate_markdown(data: dict, path: str):
    """Generate PR comment markdown, auto-pruned to <65KB."""
    wns = data["setup_wns"]
    status = "PASS" if wns >= 0 else "FAIL"

    lines = []
    lines.append(f"**{status}** Timing — `{data['design']}` — "
                 f"`{data['platform']}` | "
                 f"Setup WNS `{wns:.4f} ns` TNS `{data['setup_tns']:.4f} ns` | "
                 f"Hold WNS `{data['hold_wns']:.4f} ns`\n")

    lines.append("| | Setup | Hold |")
    lines.append("|---|---|---|")
    lines.append(f"| WNS | `{data['setup_wns']:.4f} ns` | "
                 f"`{data['hold_wns']:.4f} ns` |")
    lines.append(f"| TNS | `{data['setup_tns']:.4f} ns` | "
                 f"`{data['hold_tns']:.4f} ns` |")
    lines.append(f"| Endpoints | {data['endpoint_count']} | — |\n")

    # Histogram
    hist = _ascii_histogram(data["endpoints"])
    if hist:
        lines.append(hist)
        lines.append("")

    # Clock domains
    if data["clocks"]:
        lines.append("<details><summary>Clock Domains</summary>\n")
        lines.append("| Clock | Period (ns) | Sources |")
        lines.append("|---|---|---|")
        for clk in data["clocks"]:
            sources = ", ".join(clk["sources"])
            lines.append(f"| `{clk['name']}` | `{clk['period']}` | "
                         f"`{sources}` |")
        lines.append("\n</details>\n")

    # Failing paths
    failing = [p for p in data["paths"] if p["slack"] < 0]
    if failing:
        shown = failing[:20]
        lines.append(f"<details><summary>Top Failing Paths "
                     f"({len(failing)})</summary>\n")
        lines.append("| Slack | Start | End | Group |")
        lines.append("|---|---|---|---|")
        for p in shown:
            start = p["start"][-35:]
            end = p["end"][-35:]
            lines.append(f"| `{p['slack']:.4f}` | `{start}` | "
                         f"`{end}` | {p['group']} |")
        if len(failing) > 20:
            lines.append(f"\n... and {len(failing) - 20} more failing paths")
        lines.append("\n</details>\n")

    # Cell breakdown
    cells: dict = {}
    for p in data["paths"]:
        for a in p["arcs"]:
            if a["net"]:
                continue
            if a["cell"] not in cells:
                cells[a["cell"]] = {"count": 0, "delay": 0.0, "slews": []}
            cells[a["cell"]]["count"] += 1
            cells[a["cell"]]["delay"] += abs(a["delay"])
            cells[a["cell"]]["slews"].append(a["slew"])

    if cells:
        top_cells = sorted(cells.items(),
                           key=lambda x: -x[1]["delay"])[:10]
        lines.append("<details><summary>Cell Type Breakdown "
                     "(top 10 by delay)</summary>\n")
        lines.append("| Cell | Count | Total Delay (ns) | Avg Slew (ns) |")
        lines.append("|---|---|---|---|")
        for name, v in top_cells:
            avg_slew = sum(v["slews"]) / len(v["slews"])
            lines.append(f"| `{name}` | {v['count']} | "
                         f"`{v['delay']:.4f}` | `{avg_slew:.4f}` |")
        lines.append("\n</details>\n")

    lines.append("Full interactive report: see `1_timing.html` artifact")

    md = "\n".join(lines)
    # Ensure under 65KB
    if len(md) > 60000:
        md = md[:60000] + "\n\n... (truncated to fit PR comment limit)"

    with open(path, "w") as f:
        f.write(md)
    size_kb = len(md) / 1024
    print(f"[timing_report] wrote {path} ({size_kb:.1f} KB)")


# ─── Main ─────────────────────────────────────────────────────────────────────

def main():
    odb_file = os.environ.get("ODB_FILE")
    sdc_file = os.environ.get("SDC_FILE")
    reports_dir = os.environ.get("REPORTS_DIR", ".")
    design_name = os.environ.get("DESIGN_NAME", "unknown")
    platform = os.environ.get("PLATFORM", "unknown")

    if not odb_file or not sdc_file:
        print("ERROR: ODB_FILE and SDC_FILE environment variables required",
              file=sys.stderr)
        sys.exit(1)

    tech = Tech()
    design = Design(tech)
    design.readDb(odb_file)
    design.evalTclString(f"read_sdc {sdc_file}")

    timing = Timing(design)
    print(f"[timing_report] extracting timing data for {design_name}...")

    data = extract_timing_data(timing, design_name, platform)

    os.makedirs(reports_dir, exist_ok=True)
    html_path = os.path.join(reports_dir, "1_timing.html")
    md_path = os.path.join(reports_dir, "1_timing.md")

    generate_html(data, html_path)
    generate_markdown(data, md_path)

    print(f"[timing_report] done. "
          f"WNS={data['setup_wns']:.4f} TNS={data['setup_tns']:.4f} "
          f"endpoints={data['endpoint_count']} "
          f"paths={len(data['paths'])}")


if __name__ == "__main__":
    main()
else:
    # When run via openroad -python (no __name__ == "__main__")
    main()
