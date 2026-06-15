// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Generate the multi-input (>2) basic gate symbols used in
// ../../src/openroad_skin.svg: and3/4, or3/4, nand3/4, nor3/4.  Output Y is
// centered at the nose so netlistsvg routes the output wire straight to it.
// Run `node gen_multi.mjs` and paste the printed <g> symbols into
// openroad_skin.svg (and add matching entries to SKIN_MULTI_TYPES in
// ../../src/schematic-widget.js).

const W = 30;      // body width
const ROW = 12;    // input spacing
const R = 3;       // inversion bubble radius

function n(v) { return Number(v.toFixed(2)); }

function symbol(name, base, inverting, nIn) {
  const H = nIn * ROW;
  const mid = H / 2;
  const draw = [];
  if (base === 'and') {
    draw.push(`<path d="M0,0 L0,${H} L${W / 2},${H} A${W / 2} ${mid} 0 0 0 ${W / 2},0 Z" class="$cell_id"/>`);
  } else { // or
    draw.push(`<path d="M0,${H} L${W / 2},${H} A${W / 2} ${mid} 0 0 0 ${W / 2},0 L0,0" class="$cell_id"/>`);
    draw.push(`<path d="M0,0 A${W} ${H} 0 0 1 0,${H}" class="$cell_id"/>`);
  }
  const inX = base === 'and' ? 0 : 3;
  let outX = W;
  if (inverting) {
    draw.push(`<circle cx="${W + R + 1}" cy="${mid}" r="${R}" class="$cell_id"/>`);
    outX = W + 2 * R + 2;
  }

  const lines = [];
  lines.push(`  <g s:type="${name}" transform="translate(50,400)" s:width="${outX}" s:height="${H}">`);
  lines.push(`    <s:alias val="${name}"/>`);
  lines.push(`    <text x="${n(W / 2)}" y="-4" class="nodelabel $cell_id" s:attribute="ref">${name}</text>`);
  for (const d of draw) lines.push(`    ${d}`);
  lines.push('');
  const letters = ['A', 'B', 'C', 'D', 'E', 'F'];
  for (let i = 0; i < nIn; i++) {
    lines.push(`    <g s:x="${inX}" s:y="${n((i + 0.5) * ROW)}" s:pid="${letters[i]}"/>`);
  }
  lines.push(`    <g s:x="${n(outX)}" s:y="${n(mid)}" s:pid="Y"/>`);
  lines.push('  </g>');
  return lines.join('\n');
}

const out = [];
for (const nIn of [3, 4]) {
  out.push(symbol('and' + nIn, 'and', false, nIn));
  out.push(symbol('nand' + nIn, 'and', true, nIn));
  out.push(symbol('or' + nIn, 'or', false, nIn));
  out.push(symbol('nor' + nIn, 'or', true, nIn));
}
console.log(out.join('\n\n'));
