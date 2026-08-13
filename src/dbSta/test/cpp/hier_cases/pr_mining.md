# PR mining for the hierarchy conformance corpus

Per-PR audit for the history classes the conformance plan scoped in: **(A)** name
collision, **(C)** one-modnet-to-one-net reassociation, **(G)** feedthrough,
**(H)** escaped names. For each: does the fix touch the pipeline under test
(`read_verilog` -> `link_design` -> `write_verilog`), did a netlist reproducer
ever land, and is that netlist in this corpus?

## Headline result: none of the ten named PRs is reachable by this suite

| PR | class | source it fixes | in read/link/write? | netlists | verdict |
| --- | --- | --- | --- | --- | --- |
| #9675 | A | `odb/dbInsertBuffer.cpp` | no — buffer insertion | 8 (3 use, 2 quarantined) | not reproducible |
| #9761 | A | `dbEditHierarchy.cc`, `dbInsertBuffer.cpp`, `dbModNet.cpp`, `dbNet.cpp` | no — buffer insertion / edits | 5 (3 use, 2 quarantined) | not reproducible |
| #9443 | A | `dbEditHierarchy.cc`, `dbInsertBuffer.cpp`, `rsz/Resizer.hh` | no — repair-tie transform | 9 (8 quarantined) | not reproducible |
| #9213 | A | `odb/dbInsertBuffer.cpp`, `dbNet.cpp` | no — buffer insertion | 2 (both quarantined) | not reproducible |
| #8435 | A | `dbEditHierarchy.cc`, `dbModNet.cpp` | no — hierarchy edits | none | not reproducible |
| #10852 | A | `odb/dbInsertBuffer.cpp` | no — buffer insertion | none | not reproducible |
| #10343 | G | `rsz/SwapPinsMove.cc`, `dbNetwork.cc` | no — pin swap | 2 (both use) | not reproducible |
| #9877 | G | `rsz/UnbufferMove.cc` | no — buffer removal | 2 (both use) | not reproducible |
| #8772 | H | `dbSta/SpefWriter.cc`, `dbNetwork.cc` | no — SPEF writer | 2 (1 quarantined) | not reproducible |
| #6405 | H | `rsz/SpefWriter.cc` | no — SPEF writer | none | not reproducible |

Every one fixes either a **netlist transform** (buffer insert/remove, pin swap,
repair-tie, hierarchy edits) or the **SPEF writer**. The plan's own Scope section
excludes transforms ("No netlist transforms -- no `rsz`, `cts`, `gpl`, `dft`,
`odb` module swap"), and SPEF is a different emitter entirely. So the whole
"not reproducible" column is not a coverage gap this suite can close: these bug
classes are unreachable here by construction.

Their netlists are still in the corpus, and several pass, but that only means
the corpus carries their *shapes* -- not that the bugs are covered. Reverting any
of these fixes would leave this suite green, which is why the Stage 5 negative
control had to be built from an injected defect in `makeModNetsForSubmodules`
instead.

## The pipeline's actual bug history is different commits

Measured over `--since=2025-01-01`, non-merge:

| file | commits |
| --- | --- |
| `src/dbSta/src/dbNetwork.cc` | 208 |
| `src/dbSta/src/dbReadVerilog.cc` | 53 |
| `src/sta/verilog/VerilogWriter.cc` | **0** |

The writer has not been touched at all. Whatever goes wrong on the way out
therefore originates in the `dbModule`/`dbModNet` overlay the writer walks, not
in the writer -- which is where this suite should keep looking.

The genuinely in-scope fixes, and whether a reproducer landed with them:

| commit | what it fixed | class | reproducer |
| --- | --- | --- | --- |
| `793acfccc8` | missing `dbModITerm`/`dbModNet` creations (#9454) | C | `hier3.v` (in corpus, use) |
| `057aa883c4` | escaped child `moditerm` lookup | H | `modnet_port_alias.v` (in corpus, use) |
| `45f0cb9d44` | modnet connectivity / deep-descendant pin binding | A | `TestReadVerilog_DeepDescendantModBTermCollision.v`, `modnet_port_alias.v` |
| `3ddad16377` | added the `dbModNet` connection for a feedthrough port | G | **none — written here** |
| `fd258c2516` | wrong IO type on `dbModBTerm` for a bus port | — | **none — written here** |

Two of five landed with no netlist at all, confirming the plan's expectation that
"many were fixed without one". Both now have one:

- `mined_3ddad16377_feedthrough_port_modnet.v` — a single net touching two module
  ports and a child instance pin at once, which is what
  `network_->termIterator(inst_pin_net)` keys on.
- `mined_fd258c2516_bus_bit_io_type.v` — a submodule with an input and an output
  bus of different widths, so a bit that kept a default direction cannot be
  masked by a same-shaped port opposite it.

Both pass. They are regressions, not findings.

## Caveat on `45f0cb9d44`

Removing its guard by hand (the commit no longer reverts cleanly) left both this
suite **and its own unit test** `test_read_verilog` green. So either the guard has
become redundant through later refactoring, or `TestReadVerilog`'s
`DeepDescendantModBTermCollision` case no longer exercises it. Worth resolving
independently of this work; it is not evidence about the oracle.
