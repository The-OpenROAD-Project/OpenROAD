// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Generate the compound AOI/OAI gate symbols used in ../../src/openroad_skin.svg.
// Output ports are placed at the gate's natural output point so netlistsvg
// routes the output wire straight to it (no jog).  Run `node gen_compound.mjs`
// and paste the printed <g> symbols into openroad_skin.svg (and add matching
// entries to SKIN_COMPOUND_TYPES in ../../src/schematic-widget.js).

const ROW = 14;           // vertical spacing between input rows
const SUBW = 16;          // first-level gate width
const FINW = 26;          // second-level gate width
const GAP = 4;            // gap between first- and second-level gate backs
const R = 3;              // inversion bubble radius

function andBody(x, y, w, h) {
  return `<path d="M${x},${y} L${x},${y + h} L${x + w / 2},${y + h} `
       + `A${w / 2} ${h / 2} 0 0 0 ${x + w / 2},${y} Z" class="$cell_id"/>`;
}
function orBody(x, y, w, h) {
  return `<path d="M${x},${y + h} L${x + w / 2},${y + h} `
       + `A${w / 2} ${h / 2} 0 0 0 ${x + w / 2},${y} L${x},${y}" class="$cell_id"/>\n`
       + `    <path d="M${x},${y} A${w} ${h} 0 0 1 ${x},${y + h}" class="$cell_id"/>`;
}
function bubble(cx, cy) { return `<circle cx="${cx}" cy="${cy}" r="${R}" class="$cell_id"/>`; }
function line(x1, y1, x2, y2) { return `<path d="M${x1},${y1} L${x2},${y2}" class="$cell_id"/>`; }
function n(v) { return Number(v.toFixed(2)); }

// terms: array of sizes (1 = literal, >=2 = sub-gate).  isAoi: AND->NOR else OR->NAND.
// originX/originY position the template in the skin; netlistsvg overwrites the
// transform per instance at layout time, so the position only matters for
// previewing the raw skin file.  Returns {svg, w, h}.
function buildSymbol(type, terms, isAoi, originX, originY) {
  const subBase = isAoi ? 'and' : 'or';
  const totalInputs = terms.reduce((a, s) => a + s, 0);
  const H = totalInputs * ROW;
  const finalBackX = SUBW + GAP;

  const draw = [];
  const ports = [];
  const pidLetters = ['A', 'B', 'C', 'D', 'E', 'F'];
  let pid = 0;
  let row = 0;                       // current input row index
  const connYs = [];                 // y where each term meets the final gate

  for (const size of terms) {
    if (size === 1) {
      const y = n((row + 0.5) * ROW);
      ports.push(`<g s:x="0" s:y="${y}" s:pid="${pidLetters[pid]}"/>`);
      draw.push(line(0, y, finalBackX, y));     // literal lead into final gate
      connYs.push(y);
      pid++; row++;
    } else {
      const yTop = n(row * ROW + 1);
      const yBot = n((row + size) * ROW - 1);
      const subH = yBot - yTop;
      // sub-gate body; nose abuts the final gate back
      const subW = finalBackX;       // stretch nose to the final back
      draw.push(subBase === 'and'
        ? andBody(0, yTop, subW, subH)
        : orBody(0, yTop, subW, subH));
      // input ports on the sub-gate, one per row it spans
      for (let k = 0; k < size; k++) {
        const y = n((row + k + 0.5) * ROW);
        ports.push(`<g s:x="0" s:y="${y}" s:pid="${pidLetters[pid]}"/>`);
        if (subBase === 'or') draw.push(line(0, y, n(subW * 0.18), y)); // OR back leads
        pid++;
      }
      connYs.push(n((yTop + yBot) / 2));
      row += size;
    }
  }

  // Final (second-level) gate spans the connection range, centred on its mid.
  const cTop = Math.min(...connYs);
  const cBot = Math.max(...connYs);
  const finMid = n((cTop + cBot) / 2);
  const finTop = n(cTop - ROW / 2);
  const finH = n((cBot - cTop) + ROW);
  const noseX = finalBackX + FINW;
  if (isAoi) {
    draw.push(orBody(finalBackX, finTop, FINW, finH));   // NOR lobe
  } else {
    draw.push(andBody(finalBackX, finTop, FINW, finH));  // NAND body
  }
  draw.push(bubble(n(noseX + R), finMid));               // inversion bubble
  const outX = n(noseX + 2 * R);
  ports.push(`<g s:x="${outX}" s:y="${finMid}" s:pid="Y"/>`);
  const W = outX;

  const lines = [];
  lines.push(`  <g s:type="${type}" transform="translate(${originX},${originY})" s:width="${W}" s:height="${H}">`);
  lines.push(`    <s:alias val="${type}"/>`);
  lines.push(`    <text x="${n(W / 2)}" y="-4" class="nodelabel $cell_id" s:attribute="ref">${type}</text>`);
  for (const d of draw) lines.push(`    ${d}`);
  lines.push('');
  for (const p of ports) lines.push(`    ${p}`);
  lines.push('  </g>');
  return { svg: lines.join('\n'), w: W, h: H };
}

// Lay the templates out in a grid so the raw skin file is inspectable (no
// overlap).  Cells are sized to the largest symbol so nothing collides.
function layoutGrid(defs, { baseX, baseY, cols, gapX, gapY }) {
  const built = defs.map(([type, terms, isAoi]) =>
    ({ type, terms, isAoi, ...buildSymbol(type, terms, isAoi, 0, 0) }));
  const colPitch = Math.max(...built.map((b) => b.w)) + gapX;
  const rowPitch = Math.max(...built.map((b) => b.h)) + gapY;
  return built.map((b, i) => {
    const x = baseX + (i % cols) * colPitch;
    const y = baseY + Math.floor(i / cols) * rowPitch;
    return buildSymbol(b.type, b.terms, b.isAoi, x, y).svg;
  });
}

// Term-size arrays are listed smallest-first because canonicalizeCell() in
// ../../src/schematic-widget.js assigns port ids (A, B, …) term-by-term in
// ascending size order; the symbol's port layout must match that mapping.  The
// type name uses the descending-size convention (e.g. terms [1,2] -> "aoi21").
const defs = [
  ['aoi21', [1, 2], true],
  ['aoi22', [2, 2], true],
  ['aoi211', [1, 1, 2], true],
  ['aoi221', [1, 2, 2], true],
  ['aoi222', [2, 2, 2], true],
  ['aoi33', [3, 3], true],
  ['oai21', [1, 2], false],
  ['oai22', [2, 2], false],
  ['oai211', [1, 1, 2], false],
  ['oai221', [1, 2, 2], false],
  ['oai222', [2, 2, 2], false],
  ['oai33', [3, 3], false],
];
const symbols = layoutGrid(defs,
  { baseX: 20, baseY: 360, cols: 6, gapX: 24, gapY: 26 });
console.log(symbols.join('\n\n'));
