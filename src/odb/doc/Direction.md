# Direction of the .odb

The `.odb` is the design. What the design owns and a later stage needs is
in it. Text formats are input: a flow reads them once, after that the
design travels as a database. Restoring an `.odb` restores the design,
constrained and ready to time.

The line between in and out is structural, not "design versus PDK".
The block references tech layers, sites, vias and masters by id and
cannot be decoded without them, so they are in. Liberty is bound to the
masters by name when it is loaded, into a slot odb marks "not saved", so
it is out. A macro is the same as a standard cell here: its `.lef` is
in the `.odb` as a master, its `.lib` and `.gds` are not. Both are
inputs, both fixed for the run: change a macro's `.lef` or `.lib` and
everything from synthesis is rebuilt.

## Why

The Developer Guide says it: one tool, one process, one database, and
file-based communication between tools is strongly discouraged. Design
state in side files next to the `.odb` is the remaining exception.

Same inputs, same bytes. Build systems cache on that, so nothing
environmental goes into the file: no paths, no timestamps, no hostnames,
no tool versions.

## In the .odb

| state | text input | in the `.odb` as |
|---|---|---|
| technology, cells, netlist, placement, routing | LEF, DEF, Verilog | the core data model |
| power intent | UPF | `dbPowerDomain`, `dbIsolation`, `dbLevelShifter`, `dbPowerSwitch` |
| scan chains | scan insertion commands | `dbScanChain`, `dbScanPin` |
| global connect rules | `global_connect` | `dbGlobalConnect` |
| `dont_touch` | `set_dont_touch` | flag on `dbInst` and `dbNet` |

Text in once, database from then on. UPF is the model: `read_upf` at the
start, `dbPowerDomain` and friends thereafter, `write_upf` when a human
wants text back.

## Being added: timing constraints

The `.sdc` is input like UPF. `read_sdc` once; from then on the
constraints are design state and travel in the `.odb`. `write_db` stores
them, `read_db` restores them once the design is linked against a
Liberty. The end state has no flags. The `-sdc` opt-in in #11260 lets
ORFS move over first and is retired after.

Constraints bind to the Liberty when they are loaded, so the `.odb`
stores the constraints and needs the same Liberty back. The PDK's
Liberty does not change under a design; that is the normal case, not a
restriction. Modes are constraint sets and go in with the constraints.
Corners bind Liberty and parasitics and stay out with them.

`read_sdc` and `write_sdc` are untouched. They are the human interface,
forgiving in, canonical out.

## Out of the .odb

- Liberty, GDS, extraction rules, per-layer RC and parasitics read from
  outside. Another tool owns the model and binds it to the design by
  name at load. The flow passes these files, as it does today.
- Paths to any of the above.
- Logs, reports and metrics. Those are outputs.
- An archive of files with a loading sequence, or a dump of a tool's
  internal state. It stays one database: typed objects where odb owns
  the model, an opaque versioned payload on the block where a tool does.
  Either way an older OpenROAD reads the file unchanged.

## Welcome

Single-concern PRs that move one more piece of design state into the
`.odb`. Known candidates: the PDN grid specification (the shapes are
already stored), parasitics extracted by rcx. Anything else passes the
same two questions: does the design own it, and does a later stage need
it.

Whether tech and libs should move out of each checkpoint into one file
shared by all of them is a separate question. It trades the
self-contained file for dedup, and needs a size measurement before it
is worth discussing.
