# Field test — Track A on the ASAP7 F2F stack (2026-07-03)

First exercise of the 3DIC STA adapter on a **real routed design** rather than
the synthetic `3dic_cross` fixture: the ASAP7 (4×) face-to-face logic+memory
stack in `tools/OpenROAD/ASAP_3D_testcase_v1/` (logic die 524k insts, memory die
293k, 4225 F2F bumps, cross-chip nets from the top verilog). Goal: drive
`read_3dbx → create_clock → report_checks` at scale and see whether Track A's
cross-chiplet timing holds on production data.

## Result — Track A validated at scale (validation.md Bar 1)

A real constrained cross-chiplet setup path was produced:

```
Startpoint: logic_die_0/.../s2_req_addr_reg[2]     (DFF, clk)      LOGIC die
  ... ~28 ASAP7 core gates ...
  logic_die_0/FE_OFC901_dcache_addr_internal_sig_to_top_8/Y (BUFx2)   driver out
  memory_die_0/tsv_dc_addr[8].t_in/out (IOCELLBUFANTENNAIN)  +21.12ps  BOUNDARY (IO receiver)
  ... memory SRAM address path ...
Endpoint:   memory_die_0/dcache_data_addr_reg_reg[8]  (DFF, clk)   MEMORY die
Path Group: clk   slack -329.68 (VIOLATED, as expected)
```

Confirms the mission model on real data: the IO receiver times as an **interior
chiplet leaf cell** with a Liberty arc; the boundary is the **bump↔dbBTerm
bridge**; the bond is **zero-delay** (Track A, no net2 RC); Liberty delays on
both dies; capture clock carries propagated network delay. Both dies are unique
masters (single instance each) → no duplicated-master collapse exercised here.

## F1 — BLOCKER (odb, Osama), fixed locally + verified

`read_3dbx` rebuilds the unfolded model **after** notifying `postRead3Dbx`
observers, freeing the objects STA indexed in the callback → first chip-net
query dereferences freed memory (`dbUnfoldedChipNet::getConnectedBumps()` reads a
garbage `dbVector` → `reserve()` bad_alloc → abort).

- Site: `dbDatabase::triggerPostRead3Dbx` (`src/odb/src/db/dbDatabase.cpp:1210`)
  — observer loop, then `constructUnfoldedModel()`.
- This is a concrete instance of **R3(a)** (no rebuild-notification; STA's
  net-count staleness check can't see a same-count rebuild).
- **Fix (verified on the ASAP7 design):** move `constructUnfoldedModel()`
  *before* the observer loop — pure reorder, so observers consume the final
  model and nothing is freed under them. Latent on `3dic_cross` (freed slot
  happens to survive); fires on any real design.
- Reproducer: `ASAP_3D_testcase_v1/repro_bug/` (+ `ANALYSIS.md`).
- STA follow-up once landed: drop the workaround `constructUnfoldedModel()` in
  `dbSta::postRead3Dbx` (`src/dbSta/src/dbSta.cc:393`), added for the old
  "not built when callback fires" behavior.

## F2 — `write_verilog` emits an invalid flattened netlist (dbSta) → Track E6

`write_verilog` on the 3DIC model runs (exit 0) but the output is not legal
Verilog. Two dbNetwork causes; `VerilogWriter` itself is faithful:

1. **Duplicate instance names.** `DbInstanceChildIterator` surfaces both
   chiplets' interior leaf `dbInst`s as direct children of the synthesized top,
   and `VerilogWriter` writes each by its **chiplet-local** `network_->name()`
   (VerilogWriter.cc:340). The two dies share DEF-generated names
   (`Bump_1`, `WELLTAP_99999`, …) → ~290k colliding identifiers in one module.
2. **Portless top.** `makeTopCellForChip` synthesizes a top cell with **no
   ports** (the 1384 PCB signals aren't created as `Port`s), so the module is
   `logic_memory_f2f ()`.

Fix (dbSta): path-qualified/unique interior instance names (ties to the
`dbUnfoldedInst` identity work, Track A′) — or present real hierarchy so short
names are scoped — plus top-level port synthesis in `makeTopCellForChip`.

## F3 — `report_checks -through` on f2f chip-nets finds nothing (dbSta) → Track E7

`-through [get_pins -of_objects [get_nets f2f_*]]` returns "No paths found":
`DbNetPinIterator`→`getConnectedBumps` enumerates only the **cell-less bidirect
bump/BTerm bridge pins**, which are not on the timing arc. `-through` the real
interior receiver iterm (`.../t_in/in`) finds the path immediately, proving
OpenSTA `-through` is fine — the enumeration is the gap.

Fix (dbSta): surface the bridged interior iterms as the chip-net's pins (or give
the bump pin the on-arc vertex). No `src/sta/` change needed.

## F4 — 3dbv path glob is single-wildcard, silent on zero match (odb, minor)

`baseParser.cpp matchesPattern` splits on the **first** `*` only (prefix+suffix;
a second `*` is literal). A two-wildcard liberty/LEF pattern in a `.3dbv` matches
**zero** files with no warning. This silently loaded no standard-cell liberty in
the testcase → all interior cells 0-delay/non-sequential → `report_checks` "No
paths found" (a long mis-debug before the empty match was found).

- Action (odb, minor robustness): warn when a resolved glob matches 0 files;
  optionally support multiple `*`.
- Testcase fix (done): split liberty entries into single-`*`, date-grouped
  patterns (`asap7sc7p5t_*_SS_nldm_{211120,220122,220123}.lib`).

## What needs doing (summary)

| # | Item | Owner | Where it lands |
|---|------|-------|----------------|
| F1 | `triggerPostRead3Dbx` build-before-notify | Osama (odb) | issue filed; STA drops dbSta.cc:393 workaround after |
| F2 | 3DIC `write_verilog` (names + top ports) | us (dbSta) | roadmap **E6** |
| F3 | chip-net pin enumeration for `-through` | us (dbSta) | roadmap **E7** |
| F4 | glob zero-match warning / multi-`*` | Osama (odb) | requirements note (minor) |

Cross-refs: F1 ⇔ requirements **R3(a)**; F2 ⇔ Track **A′** identity work.
