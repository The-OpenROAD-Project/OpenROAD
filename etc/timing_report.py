# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors
#
# The Qt GUI implementation (chartsWidget.cpp, staGui.cpp,
# timingWidget.cpp) is the single source of truth for the timing
# report UI.  This script generates a practical facsimile as a
# self-contained HTML file for use outside the GUI (CI, PRs, etc).
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


def _extract_paths_tcl(design_obj: Design, max_paths: int) -> list:
    """Extract timing paths via Tcl report_checks.

    Uses report_checks in full format, parses startpoint/endpoint/slack
    and per-arc delay lines.
    """
    import tempfile
    tmp = tempfile.NamedTemporaryFile(suffix=".json", delete=False)
    tmp.close()
    try:
        design_obj.evalTclString(
            f"report_checks -path_delay max -format json "
            f"-fields {{capacitance slew fanout}} "
            f"-digits 6 "
            f"-group_path_count {min(max_paths, 100)} "
            f"> {tmp.name}")
        with open(tmp.name) as f:
            content = f.read()
        # Skip any warning lines before the JSON
        json_start = content.find("{")
        if json_start < 0:
            return []
        raw = json.loads(content[json_start:])
    except Exception:
        return []
    finally:
        os.unlink(tmp.name)

    paths = []
    checks = raw.get("checks", [raw]) if isinstance(raw, dict) else raw
    for rpt in checks:
        p = {
            "slack": rpt.get("slack", 0.0),
            "path_delay": 0.0,
            "arrival": rpt.get("data_arrival_time", 0.0),
            "required": rpt.get("required_time", 0.0),
            "skew": 0.0,
            "logic_delay": 0.0,
            "logic_depth": 0,
            "fanout": 0,
            "start": rpt.get("startpoint", ""),
            "end": rpt.get("endpoint", ""),
            "start_clk": rpt.get("source_clock", ""),
            "end_clk": rpt.get("target_clock", ""),
            "group": rpt.get("path_group", ""),
            "arcs": [],
        }
        # source_path contains the data path arcs
        prev_arrival = 0.0
        for arc in rpt.get("source_path", []):
            arrival = arc.get("arrival", 0.0)
            delay = arrival - prev_arrival
            is_net = "net" in arc
            p["arcs"].append({
                "from": "",
                "to": arc.get("pin", ""),
                "cell": arc.get("cell", "net" if is_net else ""),
                "delay": delay,
                "slew": arc.get("slew", 0.0),
                "load": arc.get("capacitance", 0.0),
                "fanout": 0,
                "rising": True,
                "net": is_net,
            })
            prev_arrival = arrival
        # Compute logic depth
        cells = set()
        for a in p["arcs"]:
            if not a["net"] and a["cell"]:
                cells.add(a["to"])
        p["logic_depth"] = len(cells)
        if p["arcs"]:
            p["fanout"] = max(
                (a["fanout"] for a in p["arcs"]), default=0)
        paths.append(p)

    paths.sort(key=lambda p: p["slack"])
    return paths


def extract_timing_data(timing: Timing, design_obj: Design,
                        design_name: str, platform: str) -> dict:
    """Extract all timing data using the Timing Python API.

    NOTE: This is a concept demo. The C++ getTimingPaths() and
    getClockInfo() methods crash when called after getWorstSlack()
    due to STA search state reuse (see docs/timing_api_bugs.md).
    We use getEndpointSlackMap() + Tcl fallbacks as a workaround.
    """
    setup_wns = timing.getWorstSlack(Timing.Max)
    setup_tns = timing.getTotalNegativeSlack(Timing.Max)
    endpoint_count = timing.getEndpointCount()

    # Endpoint slack map — filter out unconstrained (infinite slack)
    # endpoints, matching chartsWidget.cpp behavior.
    endpoint_slacks = []
    unconstrained_count = 0
    for name, slack in timing.getEndpointSlackMap(Timing.Max):
        if timing.isTimeInf(slack) or timing.isTimeInf(-slack):
            unconstrained_count += 1
        else:
            endpoint_slacks.append({"name": name, "slack": slack})

    # Timing paths via Tcl report_checks -format json
    setup_paths = _extract_paths_tcl(design_obj, endpoint_count)

    # All path group names (including empty groups) via Tcl
    path_groups = []
    try:
        pg_str = design_obj.evalTclString(
            "set _r {}; foreach g [sta::group_path_names] "
            "{lappend _r $g}; set _r")
        path_groups = pg_str.split() if pg_str.strip() else []
    except Exception:
        path_groups = list(set(p["group"] for p in setup_paths
                               if p["group"]))

    # Clock domains via Tcl
    clocks = []
    try:
        clk_names = design_obj.evalTclString(
            "set _r {}; foreach c [all_clocks] "
            "{lappend _r [get_property $c name]}; set _r"
        ).split()
        for cname in clk_names:
            period = float(design_obj.evalTclString(
                f"get_property [get_clocks {cname}] period"))
            clocks.append({
                "name": cname,
                "period": round(period, 4),
                "waveform": [],
                "sources": [],
            })
    except Exception:
        pass

    return {
        "design": design_name,
        "platform": platform,
        "setup_wns": round(setup_wns, 6),
        "setup_tns": round(setup_tns, 6),
        "hold_wns": 0.0,
        "hold_tns": 0.0,
        "endpoint_count": endpoint_count,
        "unconstrained_count": unconstrained_count,
        "endpoints": endpoint_slacks,
        "clocks": clocks,
        "paths": setup_paths,
        "path_groups": sorted(path_groups),
    }


# ─── HTML generation ─────────────────────────────────────────────────────────

TIMING_HTML_TEMPLATE = r"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Timing Report — {design}</title>
<style>
*{{box-sizing:border-box;margin:0;padding:0}}
body{{background:#fff;color:#1a1a1a;font-family:system-ui,-apple-system,sans-serif;font-size:13px}}
.hdr{{background:#f0f0f0;border-bottom:1px solid #d0d0d0;padding:8px 16px;display:flex;align-items:center;gap:12px;flex-wrap:wrap}}
.hdr h1{{font-size:15px;font-weight:600}}
.badge{{display:inline-block;padding:2px 8px;border-radius:12px;font-weight:600;font-size:12px}}
.badge.pass{{background:#238636;color:#fff}}
.badge.fail{{background:#da3633;color:#fff}}
.metric{{font-size:12px;color:#555}}
.metric b{{color:#1a1a1a}}
.panels{{display:grid;grid-template-columns:1fr;gap:1px;background:#d0d0d0}}
.panel{{background:#fff;padding:12px 16px}}
.toolbar{{display:flex;gap:8px;align-items:center;margin-bottom:8px;flex-wrap:wrap}}
.toolbar select{{background:#fff;color:#1a1a1a;border:1px solid #ccc;padding:4px 8px;border-radius:4px;font-size:12px}}
.toolbar .spacer{{flex:1}}
#histogram-wrap{{position:relative;border:1px solid #ddd}}
canvas{{display:block;width:100%;cursor:pointer}}
.tooltip{{position:absolute;background:#fff;border:1px solid #ccc;padding:6px 10px;border-radius:4px;font-size:12px;pointer-events:none;display:none;z-index:10;box-shadow:0 2px 6px rgba(0,0,0,0.15)}}
table{{width:100%;border-collapse:collapse}}
th,td{{text-align:right;padding:4px 8px;border-bottom:1px solid #e0e0e0;white-space:nowrap;font-size:12px}}
th{{background:#f5f5f5;color:#555;font-weight:600;cursor:pointer;user-select:none;position:sticky;top:0;z-index:5}}
th:hover{{color:#1a1a1a}}
th.sorted-asc::after{{content:" ▲"}}
th.sorted-desc::after{{content:" ▼"}}
td:last-child,th:last-child{{text-align:left}}
td:nth-last-child(2),th:nth-last-child(2){{text-align:left}}
.path-table-wrap{{overflow-y:auto;border:1px solid #d0d0d0;min-height:80px;height:300px}}
.sash{{height:5px;cursor:row-resize;background:#e0e0e0;border:1px solid #ccc;border-width:1px 0}}
.sash:hover{{background:#c0c0c0}}
tr:hover{{background:#dbe9f9}}
tr.selected{{background:#c0d8f0}}
.slack-neg{{color:#c00}}
.slack-pos{{color:#060}}
.detail-wrap{{border:1px solid #d0d0d0;margin:8px 0;display:none}}
.detail-tabs{{display:flex;border-bottom:1px solid #d0d0d0}}
.detail-tab{{padding:4px 12px;font-size:12px;cursor:pointer;background:#f5f5f5;border-right:1px solid #d0d0d0}}
.detail-tab.active{{background:#fff;font-weight:600}}
.detail-table-wrap{{max-height:300px;overflow-y:auto}}
.detail-table td,.detail-table th{{font-size:12px;padding:2px 8px}}
.rise{{color:#060}}
.fall{{color:#c00}}
#status-bar{{background:#f5f5f5;border-top:1px solid #d0d0d0;padding:4px 16px;font-size:11px;color:#555;position:fixed;bottom:0;left:0;right:0}}
</style>
</head>
<body>
<div class="hdr">
  <h1>Timing Report — {design}</h1>
  <span class="badge {wns_class}">{wns_label}</span>
  <span class="metric">WNS <b>{setup_wns_ps} ps</b></span>
  <span class="metric">TNS <b>{setup_tns_ps} ps</b></span>
  <span class="metric">Endpoints <b>{endpoint_count}</b></span>
  <span class="metric" style="margin-left:auto">{platform}</span>
</div>
<div class="panels">

<!-- Charts panel -->
<div class="panel">
  <div class="toolbar">
    <select id="slack-type"><option>Setup Slack</option><option>Hold Slack</option></select>
    <span class="spacer"></span>
    <select id="group-filter" onchange="filterPaths()"><option value="">No Path Group</option></select>
    <select id="clock-filter" onchange="filterPaths()"><option value="">All Clocks</option></select>
  </div>
  <div id="histogram-wrap">
    <canvas id="hist-canvas" height="160"></canvas>
    <div class="tooltip" id="hist-tooltip"></div>
  </div>
  <div id="unconstrained-label" style="padding:4px 16px;font-size:12px;color:#555"></div>
</div>

<div class="sash" id="sash-hist"></div>

<!-- Timing Report panel -->
<div class="panel">
  <div class="toolbar">
    <b>Timing Report</b>
    <span class="spacer"></span>
  </div>
  <div class="path-table-wrap" id="path-table-wrap">
    <table id="path-table">
      <thead><tr>
        <th data-col="end_clk">Capture Clock</th>
        <th data-col="required">Required</th>
        <th data-col="arrival">Arrival</th>
        <th data-col="slack">Slack ▾</th>
        <th data-col="skew">Skew</th>
        <th data-col="logic_delay">Logic Delay</th>
        <th data-col="logic_depth">Logic Depth</th>
        <th data-col="fanout">Fanout</th>
        <th data-col="start" style="text-align:left">Start</th>
        <th data-col="end" style="text-align:left">End</th>
      </tr></thead>
      <tbody id="path-body"></tbody>
    </table>
  </div>

  <div class="sash" id="sash"></div>

  <!-- Path detail -->
  <div class="detail-wrap" id="arc-detail">
    <div class="detail-tabs">
      <div class="detail-tab active">Data Path Details</div>
      <div class="detail-tab">Capture Path Details</div>
    </div>
    <div class="detail-table-wrap">
      <table class="detail-table" id="arc-table">
        <thead><tr>
          <th style="text-align:left">Pin</th>
          <th>Fanout</th>
          <th>↕</th>
          <th>Time</th>
          <th>Delay</th>
          <th>Slew</th>
          <th>Load</th>
        </tr></thead>
        <tbody id="arc-body"></tbody>
      </table>
    </div>
  </div>
</div>
</div>
<div id="status-bar">Loaded</div>

<script>
const D = {json_data};
const PS = 1e12; // seconds to picoseconds

// ─── Nice bucket algorithm (matches chartsWidget.cpp) ─────────
function snapInterval(exact) {{
  if (exact <= 0) return 1;
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

  // Convert to ps and filter by clock/group
  const clkF = document.getElementById('clock-filter').value;
  const grpF = document.getElementById('group-filter').value;
  let eps = D.endpoints;
  // Build set of filtered endpoint names from paths
  if (clkF || grpF) {{
    const names = new Set(D.paths.filter(p =>
      (!clkF || p.end_clk === clkF) && (!grpF || p.group === grpF)
    ).map(p => p.end));
    eps = eps.filter(e => names.has(e.name));
  }}

  const slacks = eps.map(e => e.slack * PS);
  if (slacks.length === 0) return;

  const dataMin = Math.min(...slacks);
  const dataMax = Math.max(...slacks);
  if (dataMin === dataMax) {{
    // All same value — show single bar
    histBins = [{{lo: dataMin - 1, hi: dataMin + 1, count: slacks.length, endpoints: eps.map(e=>e.name)}}];
  }} else {{
    // kDefaultNumberOfBuckets = 10 (matches chartsWidget.cpp)
    const interval = snapInterval((dataMax - dataMin) / 10);
    const binMin = Math.floor(dataMin / interval) * interval;
    const binMax = Math.ceil(dataMax / interval) * interval;
    const nBins = Math.max(1, Math.round((binMax - binMin) / interval));
    histBins = [];
    for (let i = 0; i < nBins; i++)
      histBins.push({{lo: binMin+i*interval, hi: binMin+(i+1)*interval, count: 0, endpoints: []}});
    for (let j = 0; j < eps.length; j++) {{
      const s = eps[j].slack * PS;
      const idx = Math.min(Math.max(0, Math.floor((s - binMin) / interval)), nBins - 1);
      histBins[idx].count++;
      histBins[idx].endpoints.push(eps[j].name);
    }}
  }}

  const maxCount = Math.max(...histBins.map(b => b.count), 1);
  const nBins = histBins.length;
  const pad = {{l: 55, r: 20, t: 25, b: 45}};
  const plotW = W - pad.l - pad.r;
  const plotH = H - pad.t - pad.b;
  const barW = plotW / nBins;

  // Y-axis nice ticks
  const yInterval = snapInterval(maxCount / 4);
  const yMax = Math.ceil(maxCount / yInterval) * yInterval;

  // Bars (red=negative, green=positive)
  for (let i = 0; i < nBins; i++) {{
    const b = histBins[i];
    if (b.count === 0) continue;
    const bh = (b.count / yMax) * plotH;
    const x = pad.l + i * barW;
    const y = pad.t + plotH - bh;
    const mid = (b.lo + b.hi) / 2;
    ctx.fillStyle = mid < 0 ? '#f08080' : '#90ee90';
    ctx.strokeStyle = mid < 0 ? '#8b0000' : '#006400';
    ctx.lineWidth = 1;
    ctx.fillRect(x + 1, y, barW - 2, bh);
    ctx.strokeRect(x + 1, y, barW - 2, bh);
  }}

  // Title
  ctx.fillStyle = '#333';
  ctx.font = '12px system-ui';
  ctx.textAlign = 'center';
  ctx.fillText('Endpoint Slack', W / 2, 16);

  // X-axis labels + ticks
  ctx.fillStyle = '#333';
  ctx.font = '11px system-ui';
  ctx.textAlign = 'center';
  const binInterval = histBins[0].hi - histBins[0].lo;
  const xLabelEvery = Math.max(1, Math.ceil(nBins / 15));
  for (let i = 0; i <= nBins; i += xLabelEvery) {{
    const val = histBins[0].lo + i * binInterval;
    const x = pad.l + i * barW;
    ctx.fillText(Math.round(val).toString(), x, pad.t + plotH + 15);
    ctx.strokeStyle = '#ccc'; ctx.lineWidth = 0.5;
    ctx.beginPath(); ctx.moveTo(x, pad.t + plotH); ctx.lineTo(x, pad.t + plotH + 4); ctx.stroke();
  }}

  // X-axis title with clock info
  ctx.fillStyle = '#555';
  ctx.font = '11px system-ui';
  const clkLabel = D.clocks.length > 0
    ? 'Clocks: ' + D.clocks.map(c => c.name + ' ' + c.period).join(', ')
    : '';
  ctx.fillText('Slack [ps]' + (clkLabel ? ', ' + clkLabel : ''), W / 2, pad.t + plotH + 35);

  // Y-axis labels
  ctx.fillStyle = '#333';
  ctx.font = '11px system-ui';
  ctx.textAlign = 'right';
  for (let v = 0; v <= yMax; v += yInterval) {{
    const y = pad.t + plotH - (v / yMax) * plotH;
    ctx.fillText(v.toString(), pad.l - 6, y + 4);
    ctx.strokeStyle = '#eee'; ctx.lineWidth = 0.5;
    ctx.beginPath(); ctx.moveTo(pad.l, y); ctx.lineTo(pad.l + plotW, y); ctx.stroke();
  }}

  // Y-axis title (rotated)
  ctx.save();
  ctx.translate(12, pad.t + plotH / 2);
  ctx.rotate(-Math.PI / 2);
  ctx.textAlign = 'center';
  ctx.fillStyle = '#555';
  ctx.font = '11px system-ui';
  ctx.fillText('Number of Endpoints', 0, 0);
  ctx.restore();
}}

// Histogram hover/click
const histCanvas = document.getElementById('hist-canvas');
const histTooltip = document.getElementById('hist-tooltip');
histCanvas.addEventListener('mousemove', function(e) {{
  const rect = histCanvas.getBoundingClientRect();
  const x = e.clientX - rect.left;
  const pad = {{l: 55, r: 20}};
  const plotW = rect.width - pad.l - pad.r;
  const idx = Math.floor((x - pad.l) / (plotW / histBins.length));
  if (idx >= 0 && idx < histBins.length) {{
    const b = histBins[idx];
    histTooltip.style.display = 'block';
    histTooltip.style.left = (e.clientX - rect.left + 10) + 'px';
    histTooltip.style.top = (e.clientY - rect.top - 30) + 'px';
    histTooltip.innerHTML = `Number of Endpoints: ${{b.count}}<br>Interval: [${{Math.round(b.lo)}}, ${{Math.round(b.hi)}}) ps`;
  }} else histTooltip.style.display = 'none';
}});
histCanvas.addEventListener('mouseleave', () => histTooltip.style.display = 'none');
histCanvas.addEventListener('click', function(e) {{
  const rect = histCanvas.getBoundingClientRect();
  const x = e.clientX - rect.left;
  const pad = {{l: 55, r: 20}};
  const plotW = rect.width - pad.l - pad.r;
  const idx = Math.floor((x - pad.l) / (plotW / histBins.length));
  if (idx >= 0 && idx < histBins.length) {{
    const b = histBins[idx];
    const eps = 1e-15;
    slackFilter = [b.lo / PS - eps, b.hi / PS + eps];
    filterPaths();
  }}
}});
histCanvas.addEventListener('dblclick', function() {{
  slackFilter = null;
  filterPaths();
}});

// ─── Path table ─────────────────────────────────────────────────
let sortCol = 'slack', sortAsc = true;
let slackFilter = null;
let filteredPaths = D.paths;

function filterPaths() {{
  const group = document.getElementById('group-filter').value;
  const clock = document.getElementById('clock-filter').value;
  filteredPaths = D.paths.filter(p => {{
    if (group && p.group !== group) return false;
    if (clock && p.end_clk !== clock) return false;
    if (slackFilter && (p.slack < slackFilter[0] || p.slack >= slackFilter[1])) return false;
    return true;
  }});
  sortPaths();
  drawHistogram();
}}

function sortPaths() {{
  filteredPaths.sort((a, b) => {{
    let va = a[sortCol], vb = b[sortCol];
    let cmp;
    if (typeof va === 'string') cmp = va.localeCompare(vb);
    else cmp = va - vb;
    if (!sortAsc) cmp = -cmp;
    if (cmp === 0) cmp = (a.end || '').localeCompare(b.end || '');
    return cmp;
  }});
  renderPaths();
}}

function ps(v) {{ return (v * PS).toFixed(3); }}

function renderPaths() {{
  const body = document.getElementById('path-body');
  const shown = filteredPaths.slice(0, 500);
  body.innerHTML = shown.map((p, i) => {{
    const cls = p.slack < 0 ? 'slack-neg' : 'slack-pos';
    return `<tr onclick="showArcDetail(${{i}})" data-idx="${{i}}">
      <td style="text-align:left">${{p.end_clk || '&lt;No clock&gt;'}}</td>
      <td>${{ps(p.required)}}</td>
      <td>${{ps(p.arrival)}}</td>
      <td class="${{cls}}">${{ps(p.slack)}}</td>
      <td>${{ps(p.skew)}}</td>
      <td>${{ps(p.logic_delay || p.arrival)}}</td>
      <td>${{p.logic_depth}}</td>
      <td>${{p.fanout}}</td>
      <td style="text-align:left" title="${{p.start}}">${{p.start}}</td>
      <td style="text-align:left" title="${{p.end}}">${{p.end}}</td>
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
    document.querySelectorAll('#path-table th').forEach(t => {{
      t.className = '';
      t.textContent = t.textContent.replace(/ [▾▴]/g, '');
    }});
    th.className = sortAsc ? 'sorted-asc' : 'sorted-desc';
    sortPaths();
  }});
}});

// ─── Path detail ─────────────────────────────────────────────────
function showArcDetail(idx) {{
  const p = filteredPaths[idx];
  const wrap = document.getElementById('arc-detail');
  wrap.style.display = 'block';

  const body = document.getElementById('arc-body');
  let cumTime = 0;
  body.innerHTML = p.arcs.map(a => {{
    cumTime += a.delay;
    const edge = a.rising ? '<span class="rise">↑</span>' : '<span class="fall">↓</span>';
    const pin = a.to + (a.cell && !a.net ? ' (' + a.cell + ')' : '');
    return `<tr>
      <td style="text-align:left" title="${{a.to}}">${{pin}}</td>
      <td>${{a.fanout || ''}}</td>
      <td>${{edge}}</td>
      <td>${{(cumTime * PS).toFixed(3)}}</td>
      <td>${{(a.delay * PS).toFixed(3)}}</td>
      <td>${{(a.slew * PS).toFixed(3)}}</td>
      <td>${{a.load > 0 ? (a.load * 1e15).toFixed(3) : ''}}</td>
    </tr>`;
  }}).join('');

  document.querySelectorAll('#path-body tr').forEach(r => r.classList.remove('selected'));
  document.querySelector(`#path-body tr[data-idx="${{idx}}"]`)?.classList.add('selected');
}}

// ─── Filters init ────────────────────────────────────────────────
function initFilters() {{
  const gsel = document.getElementById('group-filter');
  (D.path_groups || []).forEach(g => {{
    const opt = document.createElement('option');
    opt.value = g; opt.textContent = g;
    gsel.appendChild(opt);
  }});
  const clocks = [...new Set(D.paths.map(p => p.end_clk))].filter(Boolean);
  const csel = document.getElementById('clock-filter');
  clocks.forEach(c => {{
    const opt = document.createElement('option');
    opt.value = c; opt.textContent = c;
    csel.appendChild(opt);
  }});
  if (D.unconstrained_count > 0) {{
    document.getElementById('unconstrained-label').textContent =
      'Number of unconstrained pins: ' + D.unconstrained_count;
  }}
}}

// ─── Sash drag ───────────────────────────────────────────────────
function initSash(sashId, targetId) {{
  const sash = document.getElementById(sashId);
  const target = document.getElementById(targetId);
  if (!sash || !target) return;
  let startY, startH;
  sash.addEventListener('mousedown', function(e) {{
    startY = e.clientY;
    startH = target.offsetHeight;
    function onMove(e2) {{
      target.style.height = Math.max(80, startH + e2.clientY - startY) + 'px';
      if (targetId === 'hist-canvas') drawHistogram();
    }}
    function onUp() {{
      document.removeEventListener('mousemove', onMove);
      document.removeEventListener('mouseup', onUp);
    }}
    document.addEventListener('mousemove', onMove);
    document.addEventListener('mouseup', onUp);
    e.preventDefault();
  }});
}}
initSash('sash-hist', 'hist-canvas');
initSash('sash', 'path-table-wrap');

// ─── Init ────────────────────────────────────────────────────────
initFilters();
filterPaths();
drawHistogram();
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
        setup_wns_ps=f"{wns * 1e12:.3f}",
        setup_tns_ps=f"{data['setup_tns'] * 1e12:.3f}",
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

    lib_files = os.environ.get("LIB_FILES", "").split()

    tech = Tech()
    design = Design(tech)
    for lib_file in lib_files:
        tech.readLiberty(lib_file)
    design.readDb(odb_file)
    design.evalTclString(f"read_sdc {sdc_file}")

    timing = Timing(design)

    data = extract_timing_data(timing, design, design_name, platform)

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
