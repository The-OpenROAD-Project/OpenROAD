# Plan — flat bump-pin redesign

Order chosen so the tree stays buildable/testable at each step.

## 1. Re-point enumeration (F2 first — additive, both models briefly coexist)

1.1 `DbInstancePinIterator(chip_inst)`: yield bound pad iterms instead of
    unfolded bump pins (`region->getBumps() → getChipBumpInst()->getChipBump()
    → getInst() → single iterm`; skip unbound/spare).
1.2 `DbNetPinIterator(chip_net)`: same resolution from `getConnectedBumps()`.
1.3 `visitConnectedPins(chip_net)`: visit pad iterms; descend into
    `iterm->getNet()` (inner net) directly — drop the `term()` hop.

## 2. Delete the synthetic layer (F1)

2.1 `direction()`: remove forced-BIDIRECT branch (pad iterm hits the normal
    iterm path → MTerm INOUT → bidirect derived).
2.2 `vertexId()/setVertexId()`: remove bump branches + `chip_bump_vertex_ids_`
    member, its clears and comments.
2.3 `term()/port()/instance()/net()` bump branches: remove. NB `term()`:
    decide whether the pad iterm still reports a `Term` for the bound bterm
    (boundary naming) or none — follow what `visitConnectedPins` needs after
    1.3.
2.4 `makeTopCellForChip`: stop synthesizing per-bterm Ports (keep top cell +
    per-master Cells). Confirm nothing calls `findPort` on master cells.
2.5 Remove `dbToSta(dbUnfoldedChipBumpInst*)`, `staToUnfoldedBump()`,
    `PinPointerTags::kDbChipBumpInst`, `DBUNFOLDEDCHIPBUMP_INST_ID` ObjectId
    case, and the `_dbUnfoldedChipBumpInst` pad requirement note (odb pad
    itself = odb architect's call).
2.6 `bump_to_chip_net_`: retire or re-key (iterm → chip-net) per what net
    attribution still needs.

## 3. Validation (F4)

3.1 `3dic_cross.tcl`: update pin-name expectations
    (`chipA/bump_clk/PAD`-style); assert path + slack unchanged vs the
    pre-redesign golden; clock anchored on `[get_pins -of [get_nets clk_top]]`
    still yields the constrained path.
3.2 Add `-through [get_pins -of_objects [get_nets bridge]]` assertion —
    closes E7.
3.3 `3dic_get_cells.tcl` (STA-3004 error test): unchanged.
3.4 ASAP7 `read.tcl`: clean read; spot `report_checks` cross-die path.
3.5 Full dbSta suite (CMake + Bazel), odb suite.

## 4. Wrap-up

4.1 Update roadmap flat-redesign checkboxes, tech-stack decision block if details
    shifted, this spec's validation notes.
4.2 Signed-off commit; no spec docs in the upstream PR.
