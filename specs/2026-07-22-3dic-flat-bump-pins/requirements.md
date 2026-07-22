# Requirements — flat bump-pin redesign

Replace the synthetic bump `Pin*` model with the flat model: **the bump pin is
the pad inst's single `dbITerm`**. See `specs/tech-stack.md` (decision block)
and the flat bump-pin redesign section of `specs/roadmap.md`. Constitution
refs: `mission.md` principles.

Branch: continues on `3dic-sta-track-a` (or successor).

## Motivation (odb architect's consistency critique, accepted 2026-07-22)

The synthetic model is neither of OpenSTA's two boundary treatments:
- **flat** — boundary object is a real leaf pin, gets a vertex, edges leaf→leaf;
- **modular** — boundary (modITerm/modBTerm) is transparent, never a vertex.

Synthetic bump pin = invented third category. Symptoms: forced BIDIRECT,
`chip_bump_vertex_ids_` side-map (+ rebuild-reattach), forward-only bridge,
off-arc chip-net pins (`-through` E7 gap), 4-byte alignment pad requirement.

## Preconditions (all in place)

- STA-3006: every connected bump has an inst with exactly ONE iterm.
- STA-3005: every connected bump has a bound bterm.
- Spare bumps filtered out of pin enumeration.
- Pad MTerm direction INOUT (passive contract; verified on ASAP7).
- Duplicated masters / HIER masters hard-error up front (3004/3001) — raw
  iterm identity is safe (unique placement).
- Pad iterm is connected to the chiplet inner net at bmap parse (#10077 flow).

## In scope

- **F1 delete synthetic layer**: `kDbChipBumpInst` tag encode/decode
  (`dbToSta(dbUnfoldedChipBumpInst*)`, `staToUnfoldedBump`), the
  `chip_bump_vertex_ids_` side-map + its clear/reattach logic, forced-BIDIRECT
  `direction()` branch, synthetic Port creation in `makeTopCellForChip`,
  bump special-cases in `term()/port()/instance()/net()/vertexId()/
  setVertexId()`.
- **F2 re-point enumeration**: `DbNetPinIterator(chip_net)` and
  `DbInstancePinIterator(chip_inst)` yield the bound **pad iterms**
  (`getConnectedBumps → getChipBumpInst → getChipBump → getInst → the single
  iterm`). Spares stay excluded (no bterm).
- **F3 fat-net descent without the Term hop**: pad iterm sits directly on the
  inner net; `visitConnectedPins(chip_net)` visits pad iterms and descends
  into their inner nets. The bterm keeps naming/direction duty only.
- **F4 validation**: `3dic_cross` path forms; clock anchored on chip-net
  pins (now INOUT pad iterms) still propagates; ASAP7 `read.tcl` clean.
  Slack 0.83 → 0.82: pads are now real fat-net loads (physically more
  correct). NB `-through` chip-net pins still finds nothing — a passive
  Liberty-less pad cannot be a through-vertex (no internal load→drvr hop
  edge; OpenSTA adds it only for top ports). E7 stays open with that sharper
  diagnosis.

## Out of scope

- Unfolded-family migration (raw→`dbUnfoldedITerm` identity swap) — waits for
  the odb-side family; this design survives it unchanged.
- Dropping the thin per-master Cell — kept for chip-inst naming + future ETM
  (`LibertyCell`) binding.
- odb bump-elimination itself (odb-owned). This redesign is its dbSta end-state
  adopted early; expect a small identity re-key when it lands.

## Key decisions

1. **Full replacement, no parallel legacy.** The synthetic apparatus is
   deleted in the same change.
2. **`dbChipNet` remains the `Net*` handle** (cross-block scope needs one);
   only its pin set changes.
3. **Direction = pad MTerm (INOUT)** — bidirect derived from real data. Real
   per-side direction (from bterm IoType) deferred to Track C2 with the
   port-style-driver question.
4. **Vertex id = `iterm->staVertexId()`** — persistent across unfolded-model
   rebuilds; side-map and its R3 pain retired for the current (unique-master)
   scope.

## Risks / watch-items

- Clock-on-chip-net anchoring must keep working (INOUT pad iterm gives both
  vertex halves — same mechanism as today's BIDIRECT, now data-driven).
- `net(pad_iterm)` returns the inner `dbNet` (ordinary iterm path); chip-net
  membership is reached via the chip-net's own pin iterator. Verify nothing
  in graph building requires `net(pin)` == chip-net for bump pins.
- Pin names change: `chipA/clk` (synthetic, bterm-named) →
  `chipA/bump_clk/PAD` (inst/iterm). Goldens and any user-facing docs update.
- `bump_to_chip_net_` map: re-key or retire (chip-net reached via iterm →
  inst → bump → net lookup) — decide during F2.

## Implementation findings (2026-07-22)

1. **Chip-inst must be pin-less.** Listing the pad iterm under both its pad
   inst and the chip-inst makes `Graph::makeWireEdges` seed the same driver
   twice (the seed driver is never marked visited) → duplicate wire edges to
   the same load → `makeLoadPinIndexMap` overwrites the pin key while the
   index keeps counting → vector-index abort in `inputPortDelay`. A pin
   belongs to exactly one instance.
2. **The fat net needs an explicit ascent.** With `net(pad_iterm)` = the
   inner net, no pin's `net()` is the chip-net, so nothing would ever
   traverse it. `visitConnectedPins`'s `dbNet` branch crosses into the
   chip-net at each connected pad iterm (`bump_to_chip_net_` re-keyed by
   iterm); the chip-net branch descends into the other chiplets. Descent
   only — the chip-net branch must NOT visit the pad itself (its inner net's
   iterm loop does), or the pad double-lists in one traversal and duplicates
   its edge set.
3. **`ensureUnfoldedMapsFresh` rebuild guard retired.** Vertex ids live on
   persistent iterms and the maps key on iterms, so an unfolded-model rebuild
   no longer dangles the graph.
