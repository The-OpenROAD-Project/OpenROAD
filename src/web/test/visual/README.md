# Schematic widget render preview

A headless-Chrome harness that renders the **real** `src/schematic-widget.js`
(with the custom `src/openroad_skin.svg`) against representative gate netlists
and writes a PNG + SVG for each. Use it to visually check the gate symbols
(AND/OR/XOR/inverter/buffer and the compound AOI/OAI) and to inspect the
rendered netlistsvg DOM when iterating on the skin.

The viewer renders gates natively via netlistsvg: the server tags cells with a
`gate_kind` hint, `canonicalizeForSkin` rewrites them to the canonical types the
skin draws, and netlistsvg lays them out. This tool serves `src/` directly, so
it picks up edits to `openroad_skin.svg` and `schematic-widget.js` immediately.

This is a developer tool, not part of CI: it needs a local Chrome and loads the
netlistsvg JS bundle from its CDN (same as the viewer itself).

## Setup (one-time)

```sh
cd src/web/test/visual
npm install puppeteer-core
```

You also need Google Chrome / Chromium. The default path is
`/usr/bin/google-chrome`; override with `CHROME=/path/to/chrome`.

## Run

```sh
node preview.mjs [outDir]
```

Renders `and2`, `or2`, `inv`, `buf`, `nand2`, `nor2`, `xor2`, `aoi21`, `aoi22`,
and `oai21`. Each produces `<name>.png` (screenshot) and `<name>.svg` (the
rendered SVG DOM) in `outDir` (a temp dir is created and printed if omitted).

Open the PNGs to check that:

- the generic box is replaced by the gate outline (no leftover rectangle),
- inputs meet the symbol on the left at their real port positions,
- the output point / inversion bubble sits on the output wire,
- AOI/OAI first-level gates abut the second-level gate (no connector stubs).

`node_modules/` here is git-ignored; the tool itself is just `preview.mjs`.

## Regenerating gate symbols

The compound (AOI/OAI) and wider (>2-input) gate symbols in
`../../src/openroad_skin.svg` are generated:

```sh
node gen_compound.mjs   # aoi21/aoi22/oai21/oai22 <g> symbols
node gen_multi.mjs      # and3/4, or3/4, nand3/4, nor3/4 <g> symbols
```

Paste the printed `<g>` blocks into `openroad_skin.svg`, and add the new type
names to `SKIN_COMPOUND_TYPES` / `SKIN_MULTI_TYPES` in
`../../src/schematic-widget.js` so the viewer maps cells to them.
