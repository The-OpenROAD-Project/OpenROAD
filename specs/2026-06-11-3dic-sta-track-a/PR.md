# dbSta: cross-chiplet 3DIC static timing analysis

## Summary

After `read_3dbx`, `dbNetwork` now maps the 3DBlox chip structure (`dbChip`,
`dbChipInst`, `dbChipNet`, bumps) onto OpenSTA's `Network`, so one timing graph
spans multiple chiplet `dbBlock`s and `report_checks` produces real
cross-chiplet flop-to-flop paths:

```
chipA/ff/CK -> ff/Q -> inv/ZN -> buf/Z -> chipB/buf/Z -> inv/ZN -> chipB/ff/D
constrained setup, slack 0.83 (MET).
```

Scope: single-level hierarchy, unique masters (each chiplet master placed
exactly once), zero-delay bonds. Chip-bump timing identity is keyed on the
per-unfold-path `dbUnfoldedChipBumpInst`. Designs that place a master more than
once, or use nested (HIER) masters, are **rejected with a hard error**
(`STA-3004` / `STA-3001`) rather than mistimed — real support is Track A'
(`dbUnfoldedInst`) / Track B.

## How it works

```
read_3dbx -> dbSta::postRead3Dbx(top_chip)
               constructUnfoldedModel()        (not auto-built at this callback)
               dbNetwork::setTopChip()         (maps + synth top/per-master cells)
               one dbStaCbk per chiplet dbBlock (addOwner is single-owner)
           -> OpenSTA graph: topInstance children = chip-insts + unique-master
              inner dbInsts; nets = top-level chip-nets
           -> cross-chiplet report_checks
```

Handle mapping (one level up from the 2D master/instance pattern):

| OpenSTA | 3DBlox object | encoding |
|---|---|---|
| `Pin` (chip bump) | `dbUnfoldedChipBumpInst` | low-3-bit pointer tag |
| `Net` (top net) | `dbChipNet` | cast + `getObjectType()` |
| `Instance` | `dbChipInst` | cast + `getObjectType()` |
| `Cell`/`Port` | synth per chiplet master | `makeTopCellForChip` (no Liberty) |

Boundary crossing (chip-net = "fat net"):

```
   chipA: ...buf/Z --inner net-- bumpA --- dbChipNet --- bumpB --inner net-- buf/A... :chipB
   term(bumpA) -> chipA inner dbBTerm (descend);  net(bumpA) -> dbChipNet
   makeWireEdgesFromPin aggregates across the fat net -> edge chipA/buf/Z -> chipB/buf/A
```

`blockDiscBits` stamps a per-chiplet-block discriminator into the upper
`ObjectId` bits (`[block_disc:8][db_id:20][tag:4]`) so identically-numbered
pins/nets from different chiplets don't alias in `PinSet`/`NetSet`. No-op in 2D.

## Diagnostics

`STA-3001` ERROR (HIER/nested chiplet master — unsupported this version) ·
`STA-3002` ERROR (>256 unique chiplet blocks, discriminator overflow) ·
`STA-3004` ERROR (same chiplet master placed more than once — unsupported this
version). `report_3dic_summary` prints structural counts.

Revised in PR #10664 review (Osama): duplicated and nested masters now
**hard-error** instead of warning / opaque-boxing — a partial graph is a
correctness trap since downstream code assumes single placement. (Old
`STA-3003` no-master defensive check removed; a null master now falls through
to the HIER `STA-3001` path.)

## odb

`_dbChipBumpInst` / `_dbUnfoldedChipBumpInst` get a 4-byte `no-serial` pad
(`sizeof % 8 == 0`) required by the low-3-bit `Pin*` tag, guarded by a
`static_assert`. No `.odb` format change.

## Tests

`3dic_cross.tcl` (cross-chiplet setup check) and `3dic_get_cells.tcl`
(structural iteration), registered in CMake + Bazel. Full `dbSta` suite (72)
and odb `read_3dbx`/`write_3dbx`/`write_3dbv`/`check_3dblox` pass.

## Out of scope (follow-ups)

This is the single-level, zero-delay-bond foundation. The items below are
intentional scope boundaries, not regressions — each is gated by a guard or a
loud warning where it could otherwise mislead.

| Deferred | Follow-up |
|---|---|
| Duplicated chiplet masters (same master placed >1×) — **hard-error `STA-3004`** | per-path inner-instance object (`dbUnfoldedInst`), Track A' |
| **Nested (HIER)** chiplets — **hard-error `STA-3001`** | unfolded-model callback wiring + path-aware identity, Track B |
| **Bond RC** (bonds are zero-delay) | inter-chip parasitics on the hierarchical `dbChip` via `dbChipNet` |
| **Active IO cells** — bumps report `BIDIRECT` (passive-bump model) | real port direction from the bound bterm |
| `pin(Term)` reverse bridge (forward descent works) | inner-net-origin ascent |
| Map staleness is chip-net-count based; no rebuild once the graph is live | `dbBlockCallBackObj` hook on chip-net/bump mutation |
| 20-bit per-block `ObjectId` (errors `ORD-2019` past the ceiling, never aliases) | global id allocator |

## Spec docs included

`specs/mission.md`, `specs/requirements.md`, and
`specs/2026-06-11-3dic-sta-track-a/requirements.md`.
